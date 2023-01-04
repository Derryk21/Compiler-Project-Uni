/**
 * @file    alanc.c
 *
 * A recursive-descent compiler for the ALAN-2022 language.
 *
 * All scanning errors are handled in the scanner.  Parser errors MUST be
 * handled by the <code>abort_compile</code> function.  System and environment
 * errors, for example, running out of memory, MUST be handled in the unit in
 * which they occur.  Transient errors, for example, non-existent files, MUST
 * be reported where they occur.  There are no warnings, which is to say, all
 * errors are fatal and MUST cause compilation to terminate with an abnormal
 * error code.
 *

#include "boolean.h"
#include "errmsg.h"
#include "scanner.h"
#include "stdarg.h"
#include "symboltable.h"
#include "token.h"

#include "codegen.h"
#include "error.h"
#include "valtypes.h"
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/* --- type definitions ----------------------------------------------------- */

typedef struct variable_s Variable;
struct variable_s {
	char *id;       /**< variable identifier                       */
	ValType type;   /**< variable type                             */
	SourcePos pos;  /**< variable position in the source           */
	Variable *next; /**< pointer to the next variable in the list  */
};

/* --- debugging ------------------------------------------------------------ */

#ifdef DEBUG_PARSER
void debug_start(const char *fmt, ...);
void debug_end(const char *fmt, ...);
void debug_info(const char *fmt, ...);
#define DBG_start(...) debug_start(__VA_ARGS__)
#define DBG_end(...) debug_end(__VA_ARGS__)
#define DBG_info(...) debug_info(__VA_ARGS__)
#else
#define DBG_start(...)
#define DBG_end(...)
#define DBG_info(...)
#endif /* DEBUG_PARSER */

/* --- global variables ----------------------------------------------------- */

Token token;    /**< the lookahead token.type                */
FILE *src_file; /**< the source code file                    */

ValType return_type; /**< the return type of the current function */
unsigned int off_counter = 0;
int counters = 0;

/* --- function prototypes: parser routines --------------------------------- */

void parse_source(void);
void parse_funcdef(void);
void parse_body(void);
void parse_type(ValType *type);
void parse_vardef(void);
void parse_statements(void);
void parse_statement(void);
void parse_assign(void);
void parse_call(void);
void parse_if(void);
void parse_input(void);
void parse_leave(void);
void parse_output(void);
void parse_while(void);
void parse_expr(ValType *type);
void parse_simple(ValType *type);
void parse_term(ValType *type);
void parse_factor(ValType *type);
void parse_idf(ValType *type, char *id);

/* --- helper macros -------------------------------------------------------- */

#define STARTS_FACTOR(toktype)                                                 \
	(toktype == TOKEN_ID || toktype == TOKEN_NUMBER ||                         \
	 toktype == TOKEN_OPEN_PARENTHESIS || toktype == TOKEN_NOT ||              \
	 toktype == TOKEN_TRUE || toktype == TOKEN_FALSE)

#define STARTS_EXPR(toktype) (toktype == TOKEN_MINUS || STARTS_FACTOR(toktype))

#define IS_ADDOP(toktype) (toktype >= TOKEN_MINUS && toktype <= TOKEN_PLUS)

#define IS_MULOP(toktype)                                                      \
	(toktype == TOKEN_AND || toktype == TOKEN_DIVIDE ||                        \
	 toktype == TOKEN_MULTIPLY || toktype == TOKEN_REMAINDER)

#define IS_ORDOP(toktype) (toktype == TOKEN_EQUAL || toktype == TOKEN_NOT_EQUAL)

#define IS_RELOP(toktype)                                                      \
	(toktype == TOKEN_GREATER_EQUAL || toktype == TOKEN_GREATER_THAN ||        \
	 toktype == TOKEN_LESS_EQUAL || toktype == TOKEN_LESS_THAN)

#define IS_TYPE_TOKEN(toktype)                                                 \
	(toktype == TOKEN_BOOLEAN || toktype == TOKEN_INTEGER)

/* --- function prototypes: helper routines --------------------------------- */

void check_types(ValType type1, ValType type2, SourcePos *pos, ...);

void expect(TokenType type);
void expect_id(char **id);

IDprop *idprop(ValType type, unsigned int offset, unsigned int nparams,
			   ValType *params);
Variable *variable(char *id, ValType type, SourcePos pos);

/* --- function prototypes: error reporting --------------------------------- */

void abort_compile(Error err, ...);
void abort_compile_pos(SourcePos *posp, Error err, ...);

/* --- main routine --------------------------------------------------------- */

int main(int argc, char *argv[])
{
	char *jasmin_path;

	/* set up global variables */
	setprogname(argv[0]);

	/* check command-line arguments and environment */
	if (argc != 2) {
		eprintf("usage: %s <filename>", getprogname());
	}

	if ((jasmin_path = getenv("JASMIN_JAR")) == NULL) {
		eprintf("JASMIN_JAR environment variable not set");
	}

	/* open the source file, and report an error if it cannot be opened */
	if ((src_file = fopen(argv[1], "r")) == NULL) {
		eprintf("file '%s' could not be opened:", argv[1]);
	}
	setsrcname(argv[1]);

	/* initialise all compiler units */
	init_scanner(src_file);
	init_symbol_table();

	// init_code_generation();
	/* compile */
	get_token(&token);
	parse_source();

	/* produce the object code, and assemble */

	make_code_file();
	assemble(jasmin_path);

	/* release allocated resources */
	release_code_generation();
	fclose(src_file);
	freeprogname();
	freesrcname();

#ifdef DEBUG_PARSER
	printf("SUCCESS!\n");
#endif

	return EXIT_SUCCESS;
}

/* --- parser routines ------------------------------------------------------ */

/*
 * <source> = "source" <id> { <funcdef> } <body>.
 */
void parse_source(void)
{
	IDprop *p;
	char *class_name, *main_name;
	main_name = "main";

	DBG_start("<source>");

	expect(TOKEN_SOURCE);
	expect_id(&class_name);
	init_code_generation();
	set_class_name(class_name);
	// printf("here: %s", class_name);
	while (token.type == TOKEN_FUNCTION) {
		parse_funcdef();
	}
	// need to initialise main
	// unnnecessary because funcdef called for main function after source
	p = idprop(TYPE_NONE, 0, 1, NULL);
	// open_subroutine(main_name, p);
	init_subroutine_codegen(main_name, p);

	parse_body();
	gen_1(JVM_RETURN);
	close_subroutine_codegen(counters + 1);
	// list_code();
	// close_subroutine();
	// list_code();
	free(class_name);

	DBG_end("</source>");
}

/*
 * funcdef = “function” id “(” [type id {“,” type id} ] “)” [“to” type] body
 */
void parse_funcdef(void)
{
	IDprop *r;
	Variable *old, *newer;

	char *fname, *save_fname = "";
	int counter = 0;
	expect(TOKEN_FUNCTION);

	expect_id(&fname);
	save_fname = fname;
	expect(TOKEN_OPEN_PARENTHESIS);
	old = NULL;
	if (IS_TYPE_TOKEN(token.type) == TRUE) {
		counter += 1;
		parse_type(&return_type);
		expect_id(&fname);

		old = variable(fname, return_type, position);

		while (token.type == TOKEN_COMMA) {
			get_token(&token);
			parse_type(&return_type);
			expect_id(&fname);
			counter += 1;
			// linked list
			newer = variable(fname, return_type, position);
			newer->next = NULL;

			while (old->next != NULL) {
				old = old->next;
			}
			old->next = newer;
		}
		// at first to end of linked list
	}
	ValType params[counter];
	int t = 0;
	while (old) {
		params[t] = old->type;
		old = old->next;
		t += 1;
	}

	// pass the first param which has a pointer to the others
	// think type is var or func

	expect(TOKEN_CLOSE_PARENTHESIS);

	while (token.type == TOKEN_TO) {
		get_token(&token);
		parse_type(&return_type);
	}
	r = idprop(TYPE_CALLABLE, 1, counter, params);
	open_subroutine(save_fname, r);
	init_subroutine_codegen(save_fname, r);
	parse_body();
	close_subroutine();
	close_subroutine_codegen(counter);
}

/*
 * body = “begin” {⟨vardef⟩} ⟨statements⟩ “end”
 */
void parse_body(void)
{
	expect(TOKEN_BEGIN);

	while (IS_TYPE_TOKEN(token.type) == TRUE) {
		parse_vardef();
	}
	parse_statements();
	expect(TOKEN_END);
}

/*
 * type = ("boolean” | “integer”) [“array”]
 */
void parse_type(ValType *type)
{
	if (token.type == TOKEN_BOOLEAN) {
		*type = TYPE_BOOLEAN;
		get_token(&token);
		if (token.type == TOKEN_ARRAY) {
			get_token(&token);
			gen_newarray(T_BOOLEAN);
		}
	} else if (token.type == TOKEN_INTEGER) {
		*type = TYPE_INTEGER;
		expect(TOKEN_INTEGER);

		if (token.type == TOKEN_ARRAY) {
			*type = TYPE_ARRAY;
			get_token(&token);
			gen_newarray(T_INT);
		}
	} else {
		abort_compile(ERR_TYPE_EXPECTED, token.type);
	}
}

/*
 * vardef = ⟨type⟩ ⟨id⟩ {“,” ⟨id⟩} “;”
 */
void parse_vardef(void)
{
	// type variable
	// Variable s;
	off_counter += 1;
	char *vname;
	parse_type(&return_type);
	expect_id(&vname);

	insert_name(vname, idprop(return_type, off_counter, 0, NULL));

	while (token.type == TOKEN_COMMA) {
		off_counter += 1;
		get_token(&token);
		expect_id(&vname);
		insert_name(vname, idprop(return_type, off_counter, 0, NULL));
	}
	expect(TOKEN_SEMICOLON);
}

/*
 * statements = “relax” | ⟨statement⟩ {“;” ⟨statement⟩}
 */
void parse_statements(void)
{
	if (token.type == TOKEN_RELAX) {
		expect(TOKEN_RELAX);
	} else {
		parse_statement();
		while (token.type == TOKEN_SEMICOLON) {
			get_token(&token);
			parse_statement();
		}
	}
}

/*
 * statement = ⟨assign⟩ | ⟨call⟩ | ⟨if⟩ | ⟨input⟩ | ⟨leave⟩ | ⟨output⟩ | ⟨while⟩
 */
void parse_statement(void)
{
	switch (token.type) {
		case TOKEN_ID:
			parse_assign();
			break;
		case TOKEN_CALL:
			parse_call();
			break;
		case TOKEN_IF:
			parse_if();
			break;
		case TOKEN_GET:
			parse_input();
			break;
		case TOKEN_LEAVE:
			parse_leave();
			break;
		case TOKEN_PUT:
			parse_output();
			break;
		case TOKEN_WHILE:
			parse_while();
			break;
		default:
			abort_compile(ERR_STATEMENT_EXPECTED, token.type);
	}
}

/*
 * assign = ⟨id⟩ [“[” ⟨simple⟩ “]”] “:=” (⟨expr⟩ | “array” ⟨simple⟩)
 */
void parse_assign(void)
{
	int tempo = 0;
	IDprop *temp;
	// think needa insert into symbol table here
	char *aname;
	expect_id(&aname);

	find_name(aname, &temp);

	if (token.type == TOKEN_OPEN_BRACKET) {
		get_token(&token);
		if (token.type == TOKEN_INTEGER) {
			tempo = token.value;
		}
		parse_simple(&return_type);
		expect(TOKEN_CLOSE_BRACKET);
	}
	expect(TOKEN_GETS);

	if (STARTS_EXPR(token.type) == TRUE) {
		parse_expr(&return_type);
		gen_2(JVM_ISTORE, counters);
		if (temp->type == TYPE_ARRAY) {
			gen_2(JVM_ASTORE, tempo);
		}
		counters += 1;

	} else if (token.type == TOKEN_ARRAY) {
		expect(TOKEN_ARRAY);
		parse_simple(&return_type);

	} else
		abort_compile(ERR_ARRAY_ALLOCATION_OR_EXPRESSION_EXPECTED, TOKEN_ID);
}

/*
 * call = “call” ⟨id⟩ “(” [⟨expr⟩ {“,” ⟨expr⟩}] “)”
 */
// only procedures must originate, no return func
void parse_call(void)
{
	IDprop *k;
	char *cname;
	expect(TOKEN_CALL);
	expect_id(&cname);
	find_name(cname, &k);
	gen_call(cname, k);
	if (!IS_PROCEDURE(k->type)) {
		abort_compile(ERR_NOT_A_PROCEDURE, "'%s' is not a procedure ", cname);
	}
	expect(TOKEN_OPEN_PARENTHESIS);

	if (STARTS_EXPR(token.type) == TRUE) {
		parse_expr(&return_type);

		while (token.type == TOKEN_COMMA) {
			get_token(&token);
			parse_expr(&return_type);
		}
	}
	expect(TOKEN_CLOSE_PARENTHESIS);
}

/*
 * if = if” ⟨expr⟩ “then” ⟨statements⟩ {“elsif” ⟨expr⟩ “then” ⟨statements⟩}
 * [“else” ⟨statements⟩] “end”.
 */
void parse_if(void)
{
	expect(TOKEN_IF);
	parse_expr(&return_type);

	expect(TOKEN_THEN);
	parse_statements();

	while (token.type == TOKEN_ELSIF) {
		get_token(&token);
		parse_expr(&return_type);
		expect(TOKEN_THEN);
		parse_statements();
	}

	if (token.type == TOKEN_ELSE) {
		get_token(&token);
		parse_statements();
	}
	expect(TOKEN_END);
}

/*
 * input = “get” ⟨id⟩ [“[” ⟨simple⟩ “]”]
 */
void parse_input(void)
{
	char *iname;
	expect(TOKEN_GET);
	expect_id(&iname);

	if (token.type == TOKEN_OPEN_BRACKET) {
		get_token(&token);
		parse_simple(&return_type);
		expect(TOKEN_CLOSE_BRACKET);
	}
}

/*
 * leave = “leave” [⟨expr⟩].
 */
void parse_leave(void)
{
	expect(TOKEN_LEAVE);

	if (STARTS_EXPR(token.type) == TRUE) {
		parse_expr(&return_type);
	}
}

/*
 * output = “put” (⟨string⟩ | ⟨expr⟩) {“.” (⟨string⟩ | ⟨expr⟩)}
 */
void parse_output(void)
{
	expect(TOKEN_PUT);
	if (token.type == TOKEN_STRING) {
		gen_print_string(token.string);
		get_token(&token);
		while (token.type == TOKEN_CONCATENATE) {
			get_token(&token);
			if (token.type == TOKEN_STRING) {
				gen_print_string(token.string);
				get_token(&token);
			} else if (STARTS_EXPR(token.type) == TRUE) {
				parse_expr(&return_type);
				gen_print(return_type);

			} else {
				abort_compile(ERR_EXPRESSION_OR_STRING_EXPECTED, TOKEN_ID);
			}
			// gen_print_string(token.string);
		}

	} else if (STARTS_EXPR(token.type) == TRUE) {
		parse_expr(&return_type);
		gen_print(return_type);
		while (token.type == TOKEN_CONCATENATE) {
			get_token(&token);
			if (token.type == TOKEN_STRING) {
				gen_print_string(token.string);
				get_token(&token);
			} else if (STARTS_EXPR(token.type) == TRUE) {
				parse_expr(&return_type);
				gen_print(return_type);
			} else {
				abort_compile(ERR_EXPRESSION_OR_STRING_EXPECTED, TOKEN_ID);
			}
		}

	} else
		abort_compile(ERR_EXPRESSION_OR_STRING_EXPECTED, TOKEN_ID);
}

/*
 * while = “while” ⟨expr⟩ “do” ⟨statements⟩ “end”
 */
void parse_while(void)
{
	// local var
	ValType store_type;
	expect(TOKEN_WHILE);
	parse_expr(&store_type);
	expect(TOKEN_DO);
	parse_statements();
	expect(TOKEN_END);
}

/*
 * expr = ⟨simple⟩ [⟨relop⟩ ⟨simple⟩].
 */
void parse_expr(ValType *type)
{
	int store = 0;
	const char *temp;
	ValType store_type, store_type2;
	SourcePos *te;
	Token tempo;
	// t1
	parse_simple(&return_type);
	store_type = return_type;

	if (IS_RELOP(token.type) == TRUE || IS_ORDOP(token.type) == TRUE) {
		tempo.type = token.type;
		*type = TYPE_BOOLEAN;
		if (IS_RELOP(token.type) == TRUE) {
			te = &position;
			store = 1;
			temp = get_token_string(token.type);
		}
		get_token(&token);
		parse_simple(&store_type2);
		switch (tempo.type) {
			case TOKEN_EQUAL:
				gen_cmp(JVM_IF_ICMPEQ);
				break;
			case TOKEN_GREATER_EQUAL:
				gen_cmp(JVM_IF_ICMPGE);
				break;
			case TOKEN_GREATER_THAN:
				gen_cmp(JVM_IF_ICMPGT);
				break;
			case TOKEN_LESS_EQUAL:
				gen_cmp(JVM_IF_ICMPLE);
				break;
			case TOKEN_LESS_THAN:
				gen_cmp(JVM_IF_ICMPLT);
				break;
			case TOKEN_NOT_EQUAL:
				gen_cmp(JVM_IF_ICMPNE);
				break;

			default:
				break;
		}

		if (store == 1) {
			return;
		}
	} else {
		// just a simple
		*type = return_type;
	}
}
/*
 * simple = [“-”] ⟨term⟩ {⟨addop⟩ ⟨term⟩}
 */
void parse_simple(ValType *type)
{
	ValType store_type;
	int store1 = 0;
	int store = 0;
	if (token.type == TOKEN_MINUS) {
		expect(TOKEN_MINUS);
		store1 = 1;
	}

	// t1
	parse_term(&return_type);

	if (store1 == 1) {
		gen_1(JVM_INEG);
	}

	store_type = return_type;
	// t0=t1
	*type = return_type;

	while (IS_ADDOP(token.type) == TRUE) {
		switch (token.type) {
			case TOKEN_MINUS:
				store = 1;
				break;
			case TOKEN_PLUS:
				store = 2;
				break;
			case TOKEN_OR:
				store = 3;
				break;

			default:
				break;
		}
		int temp1 = 0;
		int temp2 = 0;
		if (token.type == TOKEN_PLUS) {
			// gen_2(JVM_IADD, token.value);
		}

		if (token.type == TOKEN_MINUS || token.type == TOKEN_PLUS) {
			temp1 = 1;
		}
		if (token.type == TOKEN_PLUS) {
			temp2 = 1;
		}

		get_token(&token);
		// t2
		parse_term(&return_type);
		switch (store) {
			case 1:
				gen_1(JVM_ISUB);
				break;
			case 2:
				gen_1(JVM_IADD);
				break;
			case 3:
				gen_1(JVM_IOR);
				break;

			default:
				break;
		}
	}
}

/*
 * term = ⟨factor⟩ {⟨mulop⟩ ⟨factor⟩}
 */
void parse_term(ValType *type)
{
	ValType store;
	int temp = 0;
	int temp2 = 0;
	parse_factor(&return_type);

	//
	// t1 value
	store = return_type;
	// t0=t1
	*type = return_type;
	while (IS_MULOP(token.type) == TRUE) {
		if (token.type == TOKEN_MULTIPLY) {
			temp2 = 1;
		} else if (token.type == TOKEN_DIVIDE) {
			temp2 = 2;
		} else if (token.type == TOKEN_REMAINDER) {
			temp2 = 3;
		} else if (token.type == TOKEN_AND) {
			temp2 = 4;
		}
		if (token.type != TOKEN_AND) {
			temp = 1;
		}
		get_token(&token);

		parse_factor(&return_type);

		switch (temp2) {
			case 1:
				gen_1(JVM_IMUL);
				break;
			case 2:
				gen_1(JVM_IDIV);
				break;
			case 3:
				gen_1(JVM_IREM);
				break;
			case 4:
				gen_1(JVM_IAND);
				break;
			default:
				break;
		}

		// t1=t2
		// check_types(store, return_type, &position);
		if (temp == 1) {
			// t1
			// check_types(store, TYPE_INTEGER, &position);
		} else {
			// check_types(store, TYPE_BOOLEAN, &position);
		}
	}
}
/*
 * factor = ⟨id⟩ [“[” ⟨simple⟩ “]” | “(” [ ⟨expr⟩ {“,” ⟨expr⟩} ] “)”] | ⟨num⟩ |
 * “(” ⟨expr⟩ “)” | “not” ⟨factor⟩ | “true” | “false”
 */
void parse_factor(ValType *type)
{
	SourcePos *temp;
	IDprop *store;
	char *fname;
	unsigned int count_param = 0;
	if (token.type == TOKEN_ID) {
		expect_id(&fname);

		// use to check types
		find_name(fname, &store);
		gen_2(JVM_ILOAD, store->offset - 1);
		if (token.type == TOKEN_OPEN_BRACKET) {
			get_token(&token);
			gen_2(JVM_ALOAD, token.value);

			parse_simple(&return_type);
			// check_types(return_type, TYPE_INTEGER, &position);
			// setasarray
			// SET_AS_ARRAY(return_type);
			expect(TOKEN_CLOSE_BRACKET);
			*type = TYPE_INTEGER;
		} else if (token.type == TOKEN_OPEN_PARENTHESIS) {
			get_token(&token);
			if (STARTS_EXPR(token.type) == TRUE) {
				parse_expr(&return_type);

				// check_types(return_type, store->type, &position);
				while (token.type == TOKEN_COMMA) {
					temp = &position;

					count_param += 1;
					get_token(&token);
					parse_expr(&return_type);
				}
			}
			expect(TOKEN_CLOSE_PARENTHESIS);
		}
	} else if (token.type == TOKEN_NUMBER) {
		gen_2(JVM_LDC, token.value);
		// expect t0 = int
		get_token(&token);
		return_type = TYPE_INTEGER;
	} else if (token.type == TOKEN_OPEN_PARENTHESIS) {
		get_token(&token);
		// t1
		parse_expr(&return_type);
		*type = return_type;
		expect(TOKEN_CLOSE_PARENTHESIS);
	} else if (token.type == TOKEN_NOT) {
		get_token(&token);
		// t1
		parse_factor(&return_type);
		*type = return_type;
		// t0=t1

		// t1=bool
		// check_types(return_type, TYPE_BOOLEAN, &position, "for 'not'");
	} else if (token.type == TOKEN_TRUE) {
		*type = TYPE_BOOLEAN;
		gen_2(JVM_LDC, 1);
		get_token(&token);
	} else if (token.type == TOKEN_FALSE) {
		*type = TYPE_BOOLEAN;
		gen_2(JVM_LDC, 0);
		expect(TOKEN_FALSE);
	} else
		abort_compile(ERR_FACTOR_EXPECTED, token.type);
}

/* --- helper routines
 * ------------------------------------------------------ */

#define MAX_MESSAGE_LENGTH 256

void check_types(ValType found, ValType expected, SourcePos *pos, ...)
{
	// disabled errors
	exit(0);
	char buf[MAX_MESSAGE_LENGTH], *s;
	va_list ap;

	if (found != expected) {
		buf[0] = '\0';
		va_start(ap, pos);
		s = va_arg(ap, char *);
		vsnprintf(buf, MAX_MESSAGE_LENGTH, s, ap);
		va_end(ap);
		if (pos != NULL) {
			position = *pos;
		}
		leprintf("incompatible types (expected %s, found %s) %s",
				 get_valtype_string(expected), get_valtype_string(found), buf);
	}
}

void expect(TokenType type)
{
	if (token.type == type) {
		get_token(&token);
	} else {
		abort_compile(ERR_EXPECT, type);
	}
}

void expect_id(char **id)
{
	if (token.type == TOKEN_ID) {
		*id = strdup(token.lexeme);
		get_token(&token);
	} else {
		abort_compile(ERR_EXPECT, TOKEN_ID);
	}
}

IDprop *idprop(ValType type, unsigned int offset, unsigned int nparams,
			   ValType *params)
{
	IDprop *ip;

	ip = emalloc(sizeof(IDprop));
	ip->type = type;
	ip->offset = offset;
	ip->nparams = nparams;
	ip->params = params;

	return ip;
}
//
Variable *variable(char *id, ValType type, SourcePos pos)
{
	Variable *vp;

	vp = emalloc(sizeof(Variable));
	vp->id = id;
	vp->type = type;
	vp->pos = pos;
	vp->next = NULL;

	return vp;
}

/* --- error handling routine
 * ----------------------------------------------- */

void _abort_compile(SourcePos *posp, Error err, va_list args);

void abort_compile(Error err, ...)
{
	exit(0);
	va_list args;

	va_start(args, err);
	_abort_compile(NULL, err, args);
	va_end(args);

	// disabled errors
	// exit(0);
}

void abort_compile_pos(SourcePos *posp, Error err, ...)
{
	// disabled errors
	exit(0);
	va_list args;

	va_start(args, err);
	_abort_compile(posp, err, args);
	va_end(args);
}

void _abort_compile(SourcePos *posp, Error err, va_list args)
{
	// disabled errors
	exit(0);
	char expstr[MAX_MESSAGE_LENGTH], *s;
	int t;

	if (posp) {
		position = *posp;
	}

	snprintf(expstr, MAX_MESSAGE_LENGTH, "expected %%s, but found %s",
			 get_token_string(token.type));

	switch (err) {
		case ERR_ILLEGAL_ARRAY_OPERATION:
		case ERR_MULTIPLE_DEFINITION:
		case ERR_NOT_A_FUNCTION:
		case ERR_NOT_A_PROCEDURE:
		case ERR_NOT_A_VARIABLE:
		case ERR_NOT_AN_ARRAY:
		case ERR_SCALAR_EXPECTED:
		case ERR_TOO_FEW_ARGUMENTS:
		case ERR_TOO_MANY_ARGUMENTS:
		case ERR_UNREACHABLE:
		case ERR_UNKNOWN_IDENTIFIER:
			s = va_arg(args, char *);
			break;
		default:
			break;
	}

	switch (err) {
		case ERR_EXPECT:
			t = va_arg(args, int);
			leprintf(expstr, get_token_string(t));
			break;

		case ERR_FACTOR_EXPECTED:
			leprintf(expstr, "factor");
			break;

		case ERR_UNREACHABLE:
			leprintf("unreachable: %s", s);
			break;

		case ERR_TYPE_EXPECTED:
			leprintf(expstr, "type");
			break;

		case ERR_STATEMENT_EXPECTED:
			leprintf(expstr, "statement");
			break;

		case ERR_ARRAY_ALLOCATION_OR_EXPRESSION_EXPECTED:
			leprintf(expstr, "array allocation or expression");
			break;

		case ERR_EXPRESSION_OR_STRING_EXPECTED:
			leprintf(expstr, "expression or string");
			break;

		default:
			s = va_arg(args, char *);
			leprintf("unreachable: %s", s);
			break;
	}
}

/* --- debugging output routines
 * -------------------------------------------- */

#ifdef DEBUG_PARSER

static int indent = 0;

void debug_start(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	debug_info(fmt, ap);
	va_end(ap);
	indent += 2;
}

void debug_end(const char *fmt, ...)
{
	va_list ap;

	indent -= 2;
	va_start(ap, fmt);
	debug_info(fmt, ap);
	va_end(ap);
}

void debug_info(const char *fmt, ...)
{
	int i;
	char buf[MAX_MESSAGE_LENGTH], *buf_ptr;
	va_list ap;

	buf_ptr = buf;

	va_start(ap, fmt);

	for (i = 0; i < indent; i++) {
		*buf_ptr++ = ' ';
	}
	vsprintf(buf_ptr, fmt, ap);

	buf_ptr += strlen(buf_ptr);
	snprintf(buf_ptr, MAX_MESSAGE_LENGTH, " in line %d.\n", position.line);
	fflush(stdout);
	fputs(buf, stdout);
	fflush(NULL);

	va_end(ap);
}

#endif /* DEBUG_PARSER */

