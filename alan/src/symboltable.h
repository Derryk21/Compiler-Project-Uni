/**
 * @file    symboltable.h
 * @brief   A symbol table for ALAN-2022.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "boolean.h"
#include "token.h"
#include "valtypes.h"

typedef struct {
	ValType       type;     /*<< variable type or function return type     */
	unsigned int  offset;   /*<< local variable offset for code generation */
	unsigned int  nparams;  /*<< number of parameters; 0 for variables     */
	ValType      *params;   /*<< array of parameter types; NULL for vars   */
} IDprop;

/**
 * Initialises the global symbol table.
 */
void init_symbol_table(void);

/**
 * Opens a new function or procedure (subroutine) context by (1) inserting the
 * subroutine name and properties into the global symbol table, (2) preserving
 * the global symbol table for later re-use, and (3) initialising a new local
 * symbol table for the subroutine as current symbol table.
 *
 * @param[in]   id
 *     the identifier of the new function or procedure
 * @param[in]   prop
 *     the identifier properties of the new function or procedure
 * @return      <code>TRUE</code> if the local subroutine context was set up
 *              successfully, or <code>FALSE</code> otherwise
 */
Boolean open_subroutine(char *id, IDprop *prop);

/**
 * Closes the current subroutine context by (1) releasing memory resources
 * associated with the current local symbol table, and (2) setting the preserved
 * global symbol table as the current symbol table.
 */
void close_subroutine(void);

/**
 * Inserts the specified identifier with the specified properties into the
 * current symbol table.  This function "steals" the <code>id</code> and
 * <code>prop</code> pointers, and assumes responsibility for their
 * deallocation.
 *
 * @param[in]   id
 *     the identifier to insert
 * @param[in]   prop
 *     the properties to be associated with the new identifier
 * @return      <code>FALSE</code> if the identifier is already in the current
 *              symbol table, or if there was not enough space for a new entry,
 *              or <code>TRUE</code> otherwise
 */
Boolean insert_name(char *id, IDprop *prop);

/**
 * Retrieves the properties associated with the specified identifier from the
 * current symbol table.
 *
 * @param[in]   id
 *     the identifier to look up in the current symbol table
 * @param[out]  prop
 *     the pointer to the pointer to which the pointer to the properties
 *     structure, associated with the identifier, will be copied
 * @return      <code>TRUE</code> if the identifier exists in the current symbol
 *              table, or <code>FALSE</code> otherwise
 */
Boolean find_name(char *id, IDprop **prop);

/**
 * Returns the number of the identifiers stored in the current symbol table.
 */
int get_variables_width(void);

/**
 * Releases the memory resources associated with the global symbol table.
 */
void release_symbol_table(void);

/**
 * Prints the current symbol table to the standard output stream.
 */
void print_symbol_table(void);

#endif /* SYMBOLTABLE_H */
