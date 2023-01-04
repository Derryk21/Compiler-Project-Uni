/**
 * @file    token.c
 * @brief   Utility functions for ALAN-2022 tokens.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#include <assert.h>
#include "token.h"

/* the token strings */
static char *token_names[] = {
	"end-of-file", "identifier", "numeric literal", "string literal", "'array'",
	"'begin'", "'boolean'", "'call'", "'do'", "'else'", "'elsif'", "'end'",
	"'false'", "'function'", "'get'", "'if'", "'integer'", "'leave'", "'not'",
	"'put'", "'relax'", "'source'", "'then'", "'to'", "'true'", "'while'",
	"'='", "'>='", "'>'", "'<='", "'<'", "'<>'", "'-'", "'or'", "'+'", "'and'",
	"'/'", "'*'", "'rem'", "']'", "')'", "','", "'.'", "':='", "'['", "'('",
	"';'"
};

/* --- functions ------------------------------------------------------------ */

const char *get_token_string(TokenType type)
{
	assert(type >= 0 && type < (sizeof(token_names) / sizeof(char *)));
	return token_names[type];
}
