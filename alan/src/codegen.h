/**
 * @file    codegen.h
 * @brief   Code generation functions for ALAN-2022.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "jvm.h"
#include "symboltable.h"
#include "token.h"

typedef unsigned int Label;

/**
 * Assembles a Jasmin file.  The file must first be written by calling
 * <code>make_code_file</code>.
 *
 * @param[in]   jasmin_path
 *     the path to the Jasmin JAR file
 */
void assemble(const char *jasmin_path);

/**
 * Closes the code generation for the current function or procedure.
 *
 * @param[in]   varwidth
 *     the length of the local variable array, including space for parameters;
 *     should be read from the symbol table
 */
void close_subroutine_codegen(int varwidth);

/**
 * Generates the code for an operation that does not have an operand.
 *
 * @param[in]   opcode
 *     the bytecode instruction
 */
void gen_1(Bytecode opcode);

/**
 * Generates a label.
 *
 * @param[in]   label
 *     the label
 */
void gen_label(Label label);

/**
 * Generates the code for an operation with one operand.
 *
 * @param[in]   opcode
 *     the bytecode instruction
 * @param[in]   operand
 *     the operand
 */
void gen_2(Bytecode opcode, int value);

/**
 * Generates an instruction that takes a label.
 *
 * @param[in]   opcode
 *     the bytecode instruction
 * @param[in]   label
 *     the label
 */
void gen_2_label(Bytecode opcode, Label label);

/**
 * Generates a call.
 *
 * @param[in]   fname
 *     the name of the function or procedure
 * @param[in]   idprop
 *     the properties of the function or procedure identifier
 */
void gen_call(char *fname, IDprop *idprop);

/**
 * Generates the instructions that handle comparisons, ensuring that either
 * zero or one is pushed onto the stack.
 *
 * @param[in]   opcode
 *     the false jump instruction
 */
void gen_cmp(Bytecode opcode);

/**
 * Generates the instruction that creates a new array of the specified type.
 *
 * @param[in]   atype
 *     the type of array items
 */
void gen_newarray(JVMatype atype);

/**
 * Generates the instructions for the displaying output on screen.
 *
 * @param[in]   type
 *     the operand type
 */
void gen_print(ValType type);

/**
 * Generates the instructions for displaying a string on screen.
 *
 * @param[in]   string
 *     the string to display
 */
void gen_print_string(char *string);

/**
 * Generates the instructions for reading from standard input into a variable.
 *
 * @param[in]   type
 *     the operand type
 */
void gen_read(ValType type);

/**
 * Returns the next label integer.
 *
 * @return      the next label integer.
 */
Label get_label(void);

/**
 * Gets a string representation (mnemonic) of an opcode.  It would
 * probably not be wise to pack the strings in a const char * array -- since
 * the original interpreter do not use enums, it would be stylistically
 * out-of-sync to assume that the ordinal numbering will work out
 * automatically.  <b>Note:</b>  This routine is included as an alternative to
 * the approach taken in the token files.
 *
 * @param[in]   opcode
 *     the opcode for which to get the mnemonic.
 * @return      the mnemonic string of the opcode.
 */
const char *get_opcode_string(Bytecode opcode);

/**
 * Initialises the code generation unit.
 */
void init_code_generation(void);

/**
 * Initialises the code array for a function or procedure.
 *
 * @param[in]   name
 *     the name of the function or procedure
 * @param[in]   p
 *     the properties of the function or procedure identifier
 */
void init_subroutine_codegen(const char *name, IDprop *p);

/**
 * Prints the generated code to screen; for debugging purposes.
 */
void list_code(void);

/**
 * Opens the object file, and write the generated code to it.
 */
void make_code_file(void);

/**
 * Sets the name of the class file.  This must be called after
 * <code>init_code_generation</code>, but before any other code is emitted.
 *
 * @param[in] cname the name of the class file
 */
void set_class_name(char *cname);

/**
 * Releases the resources allocated or held by the code generation unit.
 */
void release_code_generation(void);

#endif /* CODEGEN_H */
