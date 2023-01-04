/**
 * @file    symboltable.c
 * @brief   A symbol table for ALAN-2022.
 * @author  W. H. K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boolean.h"
#include "error.h"
#include "hashtable.h"
#include "symboltable.h"
#include "token.h"
#include "valtypes.h"

/* --- global static variables ---------------------------------------------- */

static HashTab *table, *saved_table;
static unsigned int curr_offset;

/* --- function prototypes -------------------------------------------------- */

static void valstr(void *key, void *p, char *str);
static void freeprop(void *p);
static unsigned int shift_hash(void *key, unsigned int size);
static int key_strcmp(void *val1, void *val2);

/* --- symbol table interface ----------------------------------------------- */

void init_symbol_table(void)
{
	saved_table = NULL;
	if ((table = ht_init(0.75f, shift_hash, key_strcmp)) == NULL) {
		eprintf("Symbol table could not be initialised");
	}
	curr_offset = 1;
}

Boolean open_subroutine(char *id, IDprop *prop)
{
	if (ht_insert(table, id, prop) == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
	// must insert table properties
	saved_table = table;

	table = NULL;
	if ((table = ht_init(0.75f, shift_hash, key_strcmp)) == NULL) {
		eprintf("Symbol table could not be initialised");
	}
	curr_offset = 1;
}

void close_subroutine(void)
{
	ht_free(table, free, freeprop);
	table = saved_table;
}

Boolean insert_name(char *id, IDprop *prop)
{
	if ((!find_name(id, &prop)) && (ht_insert(table, id, prop) == 0)) {
		return TRUE;
	} else {
		return FALSE;
	}
	free(id);
	free(prop);
}

Boolean find_name(char *id, IDprop **prop)
{
	Boolean found;
	found = ht_search(table, id, (void **)prop);
	if (!found && saved_table) {
		found = ht_search(saved_table, id, (void **)prop);
		if (found && !IS_CALLABLE_TYPE((*prop)->type)) {
			found = FALSE;
		}
	}

	return found;
}

int get_variables_width(void) { return curr_offset; }

void release_symbol_table(void) { ht_free(table, free, freeprop); }

void print_symbol_table(void) { ht_print(table, valstr); }

/* --- utility functions ---------------------------------------------------- */

static void valstr(void *key, void *p, char *str)
{
	char *keystr = (char *)key;
	IDprop *idpp = (IDprop *)p;

	sprintf(str, "%s.%d", keystr, idpp->offset);
	sprintf(str, "%s@%d[%s]", keystr, idpp->offset,
			get_valtype_string(idpp->type));
}

static void freeprop(void *p) { free((IDprop *)p); }

static unsigned int shift_hash(void *key, unsigned int size)
{
	char *temp;
	size = size - 1;
	unsigned int final;
	temp = (char *)key;

	int h = 0;
	for (int i = 0; i < (int)strlen(temp); i++) {
		h = (h << 5) | (h >> 27); // 5-bit cyclic shift of the running sum
		h += (int)temp[i];        // add in next character
	}
	final = h % size;
	return final;
}

static int key_strcmp(void *val1, void *val2)
{
	return strcmp((char *)val1, (char *)val2);
}

