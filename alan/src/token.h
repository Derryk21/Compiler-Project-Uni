/**
 * @file    token.h
 * @brief   Data type definitions for the lexical analyser (scanner) of
 *          ALAN-2022.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#ifndef TOKEN_H
#define TOKEN_H

/** the maximum length of an identifier */
#define MAX_ID_LENGTH 32

/** the types of tokens that the scanner recognises */
typedef enum {

	TOKEN_EOF,     /* end-of-file    */
	TOKEN_ID,      /* identifier     */
	TOKEN_NUMBER,  /* number literal */
	TOKEN_STRING,  /* string literal */

	/* keywords: note that the boolean operators AND and OR, and the remainder
	 * operator REM, although written out as string literals, are still
	 * treated as operators
	 * */
	TOKEN_ARRAY,
	TOKEN_BEGIN,
	TOKEN_BOOLEAN,
	TOKEN_CALL,
	TOKEN_DO,
	TOKEN_ELSE,
	TOKEN_ELSIF,
	TOKEN_END,
	TOKEN_FALSE,
	TOKEN_FUNCTION,
	TOKEN_GET,
	TOKEN_IF,
	TOKEN_INTEGER,
	TOKEN_LEAVE,
	TOKEN_NOT,
	TOKEN_PUT,
	TOKEN_RELAX,
	TOKEN_SOURCE,
	TOKEN_THEN,
	TOKEN_TO,
	TOKEN_TRUE,
	TOKEN_WHILE,
	
	/* relational operators: the order of these operators is significant -- it
	 * allows us to do range checking in the parser
	 */
	TOKEN_EQUAL,
	TOKEN_GREATER_EQUAL,
	TOKEN_GREATER_THAN,
	TOKEN_LESS_EQUAL,
	TOKEN_LESS_THAN,
	TOKEN_NOT_EQUAL,

	/* additive operators */
	TOKEN_MINUS,
	TOKEN_OR,
	TOKEN_PLUS,

	/* multiplicative operators */
	TOKEN_AND,
	TOKEN_DIVIDE,
	TOKEN_MULTIPLY,
	TOKEN_REMAINDER,

	/* other non-alphabetic operators */
	TOKEN_CLOSE_BRACKET,
	TOKEN_CLOSE_PARENTHESIS,
	TOKEN_COMMA,
	TOKEN_CONCATENATE,
	TOKEN_GETS,
	TOKEN_OPEN_BRACKET,
	TOKEN_OPEN_PARENTHESIS,
	TOKEN_SEMICOLON

} TokenType;

/** the token data type */
typedef struct {
	TokenType  type;                        /**< the type of the token        */
	union {
		int    value;                      /**< numeric value (for integers) */
		char   lexeme[MAX_ID_LENGTH+1];    /**< lexeme for identifiers       */
		char  *string;                     /**< string (for write)           */
	};
} Token;

/**
 * Returns a string representation of the specified token type.
 *
 * @param[in]   type
 *     the token for which to get the string representation
 * @return      a (constant) string representation of the specified token type
 */
const char *get_token_string(TokenType type);

#endif /* TOKEN_H */
