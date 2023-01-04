/**
 * @file    testsymboltable.c
 * @brief   A driver program to test the symbol table implementation.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boolean.h"
#include "symboltable.h"

#define BUFFER_SIZE 1024

int main()
{
	char buffer[BUFFER_SIZE], *id;
	Boolean main_is_active;
	IDprop *propts;

	init_symbol_table();
	main_is_active = TRUE;

	printf("type \"search <Enter>\" to stop inserting and start searching.\n");
	printf("Actions\n=======\n");
	printf("insert <id> -- insert <id> into current table\n");
	printf("find <id>   -- find <id> in current table\n");
	printf("open <id>   -- open subroutine <id> table\n");
	printf("close       -- close current subroutine table\n");
	printf("print       -- print current symbol table\n");
	printf("quit        -- quit program\n");

	buffer[0] = '\0';
	while (TRUE) {
		printf(">> ");
		scanf("%s", buffer);

		if (strcmp(buffer, "open") == 0) {

			scanf("%s", buffer);

			if (!main_is_active) {
				printf("Already in subroutine ... not added.\n");
				continue;
			}

			id = strdup(buffer);
			propts = malloc(sizeof(IDprop));
			propts->type = TYPE_CALLABLE | TYPE_INTEGER;
			propts->nparams = 0;
			propts->params = NULL;

			if (open_subroutine(id, propts)) {
				main_is_active = FALSE;
			} else {
				printf("Subroutine already exists ... not added.\n");
				free(id);
				free(propts);
			}

		} else if (strcmp(buffer, "close") == 0) {

			if (main_is_active) {
				printf("Cannot close main routine.\n");
				continue;
			}

			close_subroutine();
			main_is_active = TRUE;

		} else if (strcmp(buffer, "print") == 0) {

			print_symbol_table();

		} else if (strcmp(buffer, "insert") == 0) {

			scanf("%s", buffer);
			id = strdup(buffer);
			propts = malloc(sizeof(IDprop));
			propts->type = TYPE_INTEGER;
			propts->nparams = 0;
			propts->params = NULL;

			if (!insert_name(id, propts)) {
				printf("Identifier already exists ... not added.\n");
				free(id);
				free(propts);
			}

		} else if (strcmp(buffer, "find") == 0) {

			scanf("%s", buffer);
			if (find_name(buffer, &propts)) {
				printf("\"%s\" at offset %i.\n", buffer,
						propts->offset);
			} else {
				printf("Identifier not found.\n");
			}

		} else if (strcmp(buffer, "quit") == 0) {

			if (!main_is_active) {
				close_subroutine();
				printf("Closed subroutine.\n");
			}
			break;

		} else {
			printf("Unknown command.\n");
		}
	}

	printf("Goodbye!\n");
	release_symbol_table();

	return EXIT_SUCCESS;
}
