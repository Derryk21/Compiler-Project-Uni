/**
 * @file    testscanner.c
 * @brief   A driver program to test the scanner unit.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "scanner.h"
#include "token.h"

/* --- function prototypes -------------------------------------------------- */

void print_token(Token *token);

/* --- main routine --------------------------------------------------------- */

int main(int argc, char *argv[])
{
	Token token;
	FILE *in_file;

	/* set up program name and token */
	setprogname(argv[0]);
	token.string = NULL;

	/* check command-line argument and open file */
	if (argc != 2) {
		eprintf("usage: %s <filename>", getprogname());
	}

	setsrcname(argv[1]);

	if ((in_file = fopen(argv[1], "r")) == NULL) {
		eprintf("file '%s' could not be opened:", argv[1]);
	}

	/* initialise scanner */
	init_scanner(in_file);

	/* iterate over tokens in the input file */
	get_token(&token);
	while (token.type != TOKEN_EOF) {
		print_token(&token);
		get_token(&token);
	}

	/* free names */
	freeprogname();
	freesrcname();
	fclose(in_file);

	/* tell Linux we're happy */
	return EXIT_SUCCESS;
}

/* --- functions ------------------------------------------------------------ */

void print_token(Token *token)
{
	switch (token->type) {
		case TOKEN_ID:
			printf("Identifier: '%s'\n", token->lexeme);
			break;
		case TOKEN_NUMBER:
			printf("Number: %d\n", token->value);
			break;
		case TOKEN_STRING:
			printf("String: \"%s\"\n", token->string);
			free(token->string);
			break;
		default:
			printf("%s\n", get_token_string(token->type));
	}
}
