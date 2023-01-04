/**
 * @file    codegen.c
 * @brief   The code generator for ALAN-2022.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#include "codegen.h"
#include "boolean.h"
#include "error.h"
#include "valtypes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* --- type definitions and constants --------------------------------------- */

typedef enum {
	CODE_LABEL = 0x0001,
	CODE_INSTRUCTION = 0x0002,
	CODE_OPERAND = 0x0004,
	MASK_TYPE = 0x000f,
	CODE_INTEGER = 0x0010,
	CODE_ARRAY_TYPE = 0x0020,
	CODE_STRING = 0x0040,
	CODE_REFERENCE = 0x0080,
	MASK_DATA_TYPE = 0x00f0,
	CODE_ALLOCATED = 0x0100,
	MASK_ALLOCATION = 0x0f00
} CodeType;

typedef struct {
	const char *instr;
	short pop;
	short push;
} BC;

typedef struct {
	CodeType type;
	union {
		JVMatype atype;
		Bytecode code;
		Label label;
		int num;
		char *string;
	};
} Code;

typedef struct body_s Body;
struct body_s {
	char *name;
	IDprop *idprop;
	Code *code;
	int ip;
	int max_stack_depth;
	int variables_width;
	Body *next;
	Body *prev;
};

/* --- Jasmin output string literals ---------------------------------------- */

char class_preamble[] =
	".class public %s\n"
	".super java/lang/Object\n\n"
	".field private static final charsetName Ljava/lang/String;\n"
	".field private static final usLocale Ljava/util/Locale;\n"
	".field private static final scanner Ljava/util/Scanner;\n\n"
	".method static public <clinit>()V\n"
	".limit stack 5\n"
	".limit locals 1 \n"
	"\tldc	\"UTF-8\"\n"
	"\tputstatic %s/charsetName Ljava/lang/String;\n"
	"\tnew	java/util/Locale\n"
	"\tdup\n"
	"\tldc	\"en\"\n"
	"\tldc	\"US\"\n"
	"\tinvokespecial "
	"java/util/Locale/<init>(Ljava/lang/String;Ljava/lang/String;)V\n"
	"\tputstatic %s/usLocale Ljava/util/Locale;\n"
	"\tnew	java/util/Scanner\n"
	"\tdup\n"
	"\tnew	java/io/BufferedInputStream\n"
	"\tdup\n"
	"\tgetstatic java/lang/System/in Ljava/io/InputStream;\n"
	"\tinvokespecial"
	" java/io/BufferedInputStream/<init>(Ljava/io/InputStream;)V\n"
	"\tgetstatic %s/charsetName Ljava/lang/String;\n"
	"\tinvokespecial "
	"java/util/Scanner/<init>(Ljava/io/InputStream;Ljava/lang/String;)V\n"
	"\tputstatic %s/scanner Ljava/util/Scanner;\n"
	"\tgetstatic %s/scanner Ljava/util/Scanner;\n"
	"\tgetstatic %s/usLocale Ljava/util/Locale;\n"
	"\tinvokevirtual"
	" java/util/Scanner/useLocale(Ljava/util/Locale;)Ljava/util/Scanner;\n"
	"\tpop\n"
	"\treturn\n"
	".end method\n\n";

char method_init[] = ".method public <init>()V\n"
					 // ".limit stack 2\n"
					 //".limit locals 1\n"
					 "\taload 0\n"
					 "\tinvokespecial java/lang/Object/<init>()V\n"
					 "\treturn\n"
					 ".end method\n\n";

char method_readBoolean[] =
	".method public static readBoolean()Z\n"
	".limit stack 2\n"
	".limit locals 1\n"
	"\tgetstatic %s/scanner Ljava/util/Scanner;\n"
	"\tinvokevirtual java/util/Scanner/next()Ljava/lang/String;\n"
	"\tastore 0\n"
	"\taload 0\n"
	"\tldc	\"true\"\n"
	"\tinvokevirtual "
	"java/lang/String/equalsIgnoreCase(Ljava/lang/String;)Z\n"
	"\tifeq False\n"
	"\ticonst_1\n"
	"\tireturn\n"
	"False:\n"
	"\taload 0\n"
	"\tldc	\"false\"\n"
	"\tinvokevirtual "
	"java/lang/String/equalsIgnoreCase(Ljava/lang/String;)Z\n"
	"\tifeq Exception\n"
	"\ticonst_0\n"
	"\tireturn\n"
	"Exception:\n"
	"\tnew	java/util/InputMismatchException\n"
	"\tdup\n"
	"\tinvokespecial java/util/InputMismatchException/<init>()V\n"
	"\tathrow\n"
	".end method\n\n";

char method_readInt[] = ".method public static readInt()I\n"
						".limit stack 1\n"
						".limit locals 1\n"
						"\tgetstatic %s/scanner Ljava/util/Scanner;\n"
						"\tinvokevirtual java/util/Scanner/nextInt()I\n"
						"\tireturn\n"
						".end method\n\n";

char ref_print_boolean[] = "java/io/PrintStream/print(Z)V";
char ref_print_integer[] = "java/io/PrintStream/print(I)V";
char ref_print_stream[] = "java/lang/System/out Ljava/io/PrintStream;";
char ref_print_string[] = "java/io/PrintStream/print(Ljava/lang/String;)V";
char *ref_read_boolean; /* must be set in set_class_name */
char *ref_read_integer; /* must be set in set_class_name */

#define REF_READ_BOOLEAN "/readBoolean()Z"
#define REF_READ_INTEGER "/readInt()I"

/* --- global static variables ---------------------------------------------- */

static BC instruction_set[] = {
	{"aload", 0, 1},         {"areturn", 1, 0},   {"astore", 1, 0},
	{"getstatic", 0, 1},     {"goto", 0, 0},      {"iadd", 2, 1},
	{"iaload", 2, 1},        {"iand", 2, 1},      {"iastore", 3, 0},
	{"idiv", 2, 1},          {"ifeq", 1, 0},      {"if_icmpeq", 2, 0},
	{"if_icmpge", 2, 0},     {"if_icmpgt", 2, 0}, {"if_icmple", 2, 0},
	{"if_icmplt", 2, 0},     {"if_icmpne", 2, 0}, {"iload", 0, 1},
	{"imul", 2, 1},          {"ineg", 1, 1},      {"invokestatic", 0, 1},
	{"invokevirtual", 0, 0}, {"ior", 2, 1},       {"istore", 1, 0},
	{"isub", 2, 1},          {"irem", 2, 1},      {"ireturn", 1, 0},
	{"ixor", 2, 1},          {"ldc", 0, 1},       {"newarray", 1, 1},
	{"return", 0, 0},        {"swap", 2, 2}};

static const char *java_types[] = {"boolean", "char",  "float", "double",
								   "byte",    "short", "int",   "long"};

#define NBYTECODES (sizeof(instruction_set) / sizeof(BC))
#define INITIAL_SIZE 1024
#define JASM_EXT ".jasmin"

static char *class_name;    /**< the class name                             */
static char *function_name; /**< the name of current function               */
static char *jasm_name;     /**< the jasmin file name                       */
static int code_size;       /**< the current code array size                */
static int ip;              /**< the instruction pointer                    */
static Body *bodies;        /**< list of function bodies                    */
static Code *code;          /**< the generated code                         */
static IDprop *idprop;      /**< id properties of the current function      */

int stack_depth, max_stack_depth;

/* --- function prototypes -------------------------------------------------- */

static void ensure_space(int num_instr);
static void adjust_stack(BC *instr);

/* --- code generation interface -------------------------------------------- */

void init_code_generation(void)
{
	bodies = NULL;
}

void init_subroutine_codegen(const char *name, IDprop *p)
{
	max_stack_depth = stack_depth = 0;
	ip = 0;
	code = emalloc(sizeof(Code) * INITIAL_SIZE);
	code_size = INITIAL_SIZE;
	function_name = estrdup(name);
	idprop = p;
}

void close_subroutine_codegen(int varwidth)
{
	Body *body;

	body = emalloc(sizeof(Body));

	/* populate new body */
	body->name = function_name;
	body->idprop = idprop;
	body->code = code;
	body->ip = ip;
	body->max_stack_depth = max_stack_depth;
	body->variables_width = varwidth;

	// link
	// body->next

	// old = bodies;

	while (bodies) {
		bodies = bodies->next;
	}
	bodies = body;
	bodies->next = NULL;
}

void set_class_name(char *cname)
{
	size_t class_name_len;

	class_name = estrdup(cname);
	class_name_len = strlen(class_name);

	jasm_name = emalloc(class_name_len + sizeof(JASM_EXT));
	strcpy(jasm_name, class_name);
	strncat(jasm_name, JASM_EXT, sizeof(JASM_EXT));

	ref_read_boolean = emalloc(class_name_len + sizeof(REF_READ_BOOLEAN));
	strcpy(ref_read_boolean, class_name);
	strncat(ref_read_boolean, REF_READ_BOOLEAN, sizeof(REF_READ_BOOLEAN));

	ref_read_integer = emalloc(class_name_len + sizeof(REF_READ_INTEGER));
	strcpy(ref_read_integer, class_name);
	strncat(ref_read_integer, REF_READ_INTEGER, sizeof(REF_READ_INTEGER));
}

void assemble(const char *jasmin_path)
{
	int status;
	pid_t pid;

	if ((pid = fork()) < 0) {
		eprintf("Could not fork a new process for assembler");
	} else if (pid == 0) {
		if (execlp("java", "java", "-jar", jasmin_path, jasm_name,
				   (char *)NULL) < 0) {
			eprintf("Could not exec Jasmin");
		}
	}

	if (waitpid(pid, &status, 0) < 0) {
		eprintf("Error waiting for Jasmin");
	} else {
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
			eprintf("Jasmin reported failure");
		} else if (WIFSIGNALED(status) || WIFSTOPPED(status)) {
			eprintf("Jasmin stopped or terminated abnormally");
		}
	}
}

void gen_1(Bytecode opcode)
{
	ensure_space(1);
	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = opcode;

	adjust_stack(&instruction_set[opcode]);
}

void gen_2(Bytecode opcode, int operand)
{
	ensure_space(2);

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = opcode;

	code[ip].type = CODE_OPERAND | CODE_INTEGER;
	code[ip++].num = operand;

	adjust_stack(&instruction_set[opcode]);
}

void gen_call(char *fname, IDprop *idprop)
{
	char *fpath;
	unsigned int i;

	ensure_space(2);

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_INVOKESTATIC;

	/* 6 + 2 * idprop->nparams:
	 *  -- 1 for '\0'
	 *  -- 2 for '(' and ')' of parameter list
	 *  -- 1 for '.' separating class from method name
	 *  -- 2 for return type, including possibility of array type
	 * the multiplier of 2 includes the possibilities of array types
	 */
	fpath = emalloc(strlen(class_name) + strlen(fname) +
					(6 + 2 * idprop->nparams) * sizeof(char));
	strcpy(fpath, class_name);
	strcat(fpath, ".");
	strcat(fpath, fname);
	strcat(fpath, "(");
	for (i = 0; i < idprop->nparams; i++) {
		if (IS_ARRAY_TYPE(idprop->params[i])) {
			strcat(fpath, "[");
		}
		strcat(fpath, "I");
	}
	strcat(fpath, ")");
	if (IS_ARRAY_TYPE(idprop->type)) {
		strcat(fpath, "[");
	}
	if (idprop->type == TYPE_CALLABLE) {
		strcat(fpath, "V");
	} else {
		strcat(fpath, "I");
	}

	code[ip].type = CODE_OPERAND | CODE_REFERENCE | CODE_ALLOCATED;
	code[ip++].string = fpath;

	adjust_stack(&instruction_set[JVM_INVOKESTATIC]);
}
// used when theres an if in expr
void gen_cmp(Bytecode opcode)
{
	int l1, l2;

	/* unnecessary to adjust stack depth or to ensure space, since both are
	 * handled in the other gen functions
	 */
	l1 = get_label();
	l2 = get_label();
	gen_2_label(opcode, l1);
	gen_2(JVM_LDC, FALSE);
	gen_2_label(JVM_GOTO, l2);
	gen_label(l1);
	gen_2(JVM_LDC, TRUE);
	gen_label(l2);
}
// a variable name
void gen_label(Label label)
{
	ensure_space(1);

	code[ip].type = CODE_LABEL;
	code[ip++].label = label;
}

void gen_2_label(Bytecode opcode, Label label)
{
	ensure_space(2);
	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = opcode;

	code[ip].type = CODE_LABEL;
	code[ip++].label = label;

	adjust_stack(&instruction_set[opcode]);
}

void gen_newarray(JVMatype atype)
{
	ensure_space(2);

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_NEWARRAY;

	code[ip].type = CODE_OPERAND | CODE_ARRAY_TYPE;
	code[ip++].atype = atype;

	adjust_stack(&instruction_set[JVM_NEWARRAY]);
}

void gen_print(ValType type)
{
	ensure_space(5);

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_GETSTATIC;

	code[ip].type = CODE_OPERAND | CODE_REFERENCE;
	code[ip++].string = ref_print_stream;

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_SWAP;

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_INVOKEVIRTUAL;

	code[ip].type = CODE_OPERAND | CODE_REFERENCE;
	if (IS_CALLABLE_TYPE(type)) {
		SET_RETURN_TYPE(type);
	}
	if (type == TYPE_BOOLEAN) {
		code[ip++].string = ref_print_boolean;
	} else if (type == TYPE_INTEGER) {
		code[ip++].string = ref_print_integer;
	} else {
		assert(FALSE);
	}

	adjust_stack(&instruction_set[JVM_GETSTATIC]);
	adjust_stack(&instruction_set[JVM_SWAP]);
	adjust_stack(&instruction_set[JVM_INVOKEVIRTUAL]);
}

void gen_print_string(char *string)
{
	ensure_space(6);

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_GETSTATIC;

	code[ip].type = CODE_OPERAND | CODE_REFERENCE;
	code[ip++].string = ref_print_stream;

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_LDC;

	code[ip].type = CODE_OPERAND | CODE_STRING | CODE_ALLOCATED;
	code[ip++].string = string;

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_INVOKEVIRTUAL;

	code[ip].type = CODE_OPERAND | CODE_REFERENCE;
	code[ip++].string = ref_print_string;

	adjust_stack(&instruction_set[JVM_GETSTATIC]);
	adjust_stack(&instruction_set[JVM_LDC]);
	adjust_stack(&instruction_set[JVM_INVOKEVIRTUAL]);
}

void gen_read(ValType type)
{
	ensure_space(2);

	code[ip].type = CODE_INSTRUCTION;
	code[ip++].code = JVM_INVOKESTATIC;

	code[ip].type = CODE_OPERAND | CODE_REFERENCE;
	if (type == TYPE_BOOLEAN) {
		code[ip++].string = ref_read_boolean;
	} else if (type == TYPE_INTEGER) {
		code[ip++].string = ref_read_integer;
	} else {
		assert(FALSE);
	}

	adjust_stack(&instruction_set[JVM_INVOKESTATIC]);
}

Label get_label(void)
{
	static Label label = 1;
	return label++;
}

const char *get_opcode_string(Bytecode opcode)
{
	if ((unsigned long)opcode < NBYTECODES) {
		return instruction_set[opcode].instr;
	} else {
		return "INVALID OPCODE";
	}
}

/* --- code dumping --------------------------------------------------------- */

static void dump_code(FILE *file);
static void dump_method(FILE *file, Body *b);
static void dump_preamble(FILE *file, char *name);

void list_code(void)
{
dump_code(stdout);
}

void dump_code(FILE *obj_file)
{
	Body *b;

	/* preamble */
	dump_preamble(obj_file, class_name);

	/* dump the methods */
	for (b = bodies; b; b = b->next) {
		dump_method(obj_file, b);
	}
}

void make_code_file(void)
{
	FILE *obj_file;

	if ((obj_file = fopen(jasm_name, "w")) == NULL) {
		eprintf("Could not open code file:");
	}

	dump_code(obj_file);

	fclose(obj_file);
}

/* --- utility functions ---------------------------------------------------- */

static void ensure_space(int num_instr)
{
	if (ip + num_instr > code_size) {
		code = erealloc(code, code_size * 2 * sizeof(Code));
		code_size *= 2;
	}
}

/**
 * Computes the net change in the stack depth caused by the instruction, and
 * updates the maximum stack depth if necessary.
 *
 * @param[in] instr the instruction for which to factor in the stack effect.
 */
static void adjust_stack(BC *instr)
{
	stack_depth += instr->push;
	if (stack_depth > max_stack_depth) {
		max_stack_depth = stack_depth;
	}
	stack_depth -= instr->pop;
}

/**
 * Writes a method to the Jasmin output file.
 *
 * @param[in] file the output file.
 * @param[in] b    the body of the method
 */
static void dump_method(FILE *file, Body *b)
{
	int i;
	unsigned int k;

	if (strcmp(b->name, "main") == 0) {
		fprintf(file, ".method public static main([Ljava/lang/String;)V\n");

	} else {
		fprintf(file, ".method public static %s(", b->name);
		for (k = 0; k < b->idprop->nparams; k++) {
			if (IS_ARRAY(b->idprop->params[k])) {
				fputs("[", file);
			}
			fputs("I", file);
		}
		fprintf(file, ")%s%s\n", (IS_ARRAY_TYPE(b->idprop->type) ? "[" : ""),
				(b->idprop->type == TYPE_CALLABLE ? "V" : "I"));
	}
	fprintf(file, ".limit stack %d\n", b->max_stack_depth);
	fprintf(file, ".limit locals %d\n", b->variables_width);

	for (i = 0; i < b->ip; i++) {
		Code c = b->code[i];

		switch (c.type & MASK_TYPE) {
			case CODE_LABEL:
				fprintf(file, "L%d:\n", c.label);
				break;
			case CODE_LABEL | CODE_OPERAND:
				fprintf(file, " L%d\n", c.label);
				break;
			case CODE_INSTRUCTION:
				fprintf(file, "\t%s", get_opcode_string(c.code));
				switch (c.code) {
					case JVM_ARETURN:
					case JVM_IADD:
					case JVM_IALOAD:
					case JVM_IAND:
					case JVM_IASTORE:
					case JVM_IDIV:
					case JVM_IMUL:
					case JVM_INEG:
					case JVM_IOR:
					case JVM_ISUB:
					case JVM_IREM:
					case JVM_IRETURN:
					case JVM_IXOR:
					case JVM_RETURN:
					case JVM_SWAP:
						/* emit linefeed */
						fprintf(file, "\n");
						break;
					default:
						/* no linefeed */
						break;
				}
				break;
			case CODE_OPERAND:
				switch (c.type & MASK_DATA_TYPE) {
					case CODE_ARRAY_TYPE:
						fprintf(file, " %s\n", java_types[c.atype - T_BOOLEAN]);
						break;
					case CODE_INTEGER:
						fprintf(file, " %d\n", c.num);
						break;
					case CODE_REFERENCE:
						fprintf(file, " %s\n", c.string);
						break;
					case CODE_STRING:
						fprintf(file, " \"%s\"\n", c.string);
						break;
					default:
						weprintf("Unknown data type for bytecode: %x\n",
								 (unsigned int)c.type);
				}
				break;
			default:
				weprintf("Unknown operator for bytecode: %x\n",
						 (unsigned int)c.type);
		}
	}

	/* guard against a dangling label at the end of the code stream */
	if ((b->code[b->ip - 1].type & MASK_TYPE) == CODE_LABEL) {
		fprintf(file, "\tnop\n");
	}

	fprintf(file, ".end method\n\n");
}

/**
 * Writes the preamble to the Jasmin output file.  The preamble consists of (i)
 * the class name and visibility specifier, (ii) the superclass, and (iii) the
 * default initialiser (constructor).
 *
 * @param[in] file the output file.
 * @param[in] name the name of the class.
 */
static void dump_preamble(FILE *file, char *name)
{
	fprintf(file, class_preamble, name, name, name, name, name, name, name);
	fputs(method_init, file);
	fprintf(file, method_readInt, name);
	fprintf(file, method_readBoolean, name);
}

void release_code_generation(void)
{
	Body *b;

	/* remove Jasmin file */

	unlink(jasm_name);

	/* free bodies */
	for (b = bodies; b; b = b->next) {
		free(b->code);
		free(b);
	}
	/* free strings */
	free(jasm_name);
	free(function_name);
	free(class_name);
	free(ref_read_integer);
	free(ref_read_boolean);
}

