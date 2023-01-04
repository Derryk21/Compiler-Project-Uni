/**
 * @file    scanner.c
 * @brief   The scanner for ALAN-2022.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "boolean.h"
#include "error.h"
#include "scanner.h"
#include "token.h"

/* --- type definitions and constants --------------------------------------- */

typedef struct {
	char      *word;                   /* the reserved word, i.e., the lexeme */
	TokenType  type;                   /* the associated token type           */
} ReservedWord;

/* --- global static variables ---------------------------------------------- */

static FILE *src_file;                 /* the source file pointer             */
static int   ch;                       /* the next source character           */
static int   column_number;            /* the current column number           */

static ReservedWord reserved[] = {     /* reserved words                      */

	{"and", TOKEN_AND}, {"array", TOKEN_ARRAY}, {"begin", TOKEN_BEGIN},
	{"boolean", TOKEN_BOOLEAN}, {"call", TOKEN_CALL}, {"do", TOKEN_DO},
	{"else", TOKEN_ELSE}, {"elsif", TOKEN_ELSIF}, {"end", TOKEN_END},
	{"false", TOKEN_FALSE}, {"function", TOKEN_FUNCTION}, {"get", TOKEN_GET},
	{"if", TOKEN_IF}, {"integer", TOKEN_INTEGER}, {"leave", TOKEN_LEAVE},
	{"not", TOKEN_NOT}, {"or", TOKEN_OR}, {"put", TOKEN_PUT},
	{"relax", TOKEN_RELAX}, {"rem", TOKEN_REMAINDER}, {"source", TOKEN_SOURCE},
	{"then", TOKEN_THEN}, {"to", TOKEN_TO}, {"true", TOKEN_TRUE},
	{"while", TOKEN_WHILE}

};

#define NUM_RESERVED_WORDS     (sizeof(reserved) / sizeof(ReservedWord))
#define MAX_INITIAL_STRING_LEN (1024)

/* --- function prototypes -------------------------------------------------- */

static void next_char(void);
static void process_number(Token *token);
static void process_string(Token *token);
static void process_word(Token *token);
static void skip_comment(void);

/* --- scanner interface ---------------------------------------------------- */

void init_scanner(FILE *in_file)
{
	src_file = in_file;
	position.line = 1;
	position.col = column_number = 0;
	next_char();
}


/* Retrieves the next token
 * - Provided that the character is valid
 */
void get_token(Token * token)
{
/* removes whitespace */
if (isspace(ch)) {
	while (isspace(ch))
		next_char();
}

if (ch == '\n') {
	next_char();
}

if (!isascii(ch) && ch != EOF) {
	position.col = column_number;
	leprintf("illegal character '%c' (ASCII #%d)", ch, ch);

}

/* temp int for error checking*/
int temp;

/* remember token start */
position.col = column_number;

/* get the next token */
if (ch != EOF) {
	if (isalpha(ch) || ch == '_') {

		/* process a word */
		process_word(token);

	} else if (isdigit(ch)) {

		/* process a number */
		process_number(token);

	} else switch (ch) {

	/* process a string */
	case '"':
		position.col = column_number;
		next_char();
		process_string(token);
		break;

	/* skip comments */
	case '{':
		skip_comment();
		get_token(token);
		break;

	case '=':
		token -> type = TOKEN_EQUAL;
		next_char();
		break;

	case '>':
		next_char();
		if (ch == '=') {
			token -> type = TOKEN_GREATER_EQUAL;
			next_char();
			position.col = position.col-1;
			break;

		} else
			token -> type = TOKEN_GREATER_THAN;
		break;

	case '<':
		next_char();
		if (ch == '=') {
			token -> type = TOKEN_LESS_EQUAL;
			next_char();
			position.col = position.col-1;
			break;

		} else if (ch == '>') {
			token -> type = TOKEN_NOT_EQUAL;
			next_char();
			position.col = position.col-1;
			break;

		} else
			token -> type = TOKEN_LESS_THAN;
		break;

	case '-':
		token -> type = TOKEN_MINUS;
		next_char();
		break;

	case '+':
		token -> type = TOKEN_PLUS;
		next_char();
		break;

	case '/':
		token -> type = TOKEN_DIVIDE;
		next_char();
		break;

	case '*':
		token -> type = TOKEN_MULTIPLY;
		next_char();
		break;

	case ']':
		token -> type = TOKEN_CLOSE_BRACKET;
		next_char();
		break;

	case ')':
		token -> type = TOKEN_CLOSE_PARENTHESIS;
		next_char();
		break;

	case ',':
		token -> type = TOKEN_COMMA;
		next_char();
		break;

	case '.':
		token -> type = TOKEN_CONCATENATE;
		next_char();
		break;

	case ':':
		temp=ch;
		next_char();
		if (ch == '=') {
			token -> type = TOKEN_GETS;
			next_char();
			position.col = position.col-1;
			break;

		} else {
			position.col = position.col;
			leprintf("illegal character '%c' (ASCII #%d)", temp, temp);
			break;
		}
	case '[':
		token -> type = TOKEN_OPEN_BRACKET;
		next_char();
		break;

	case '(':
		token -> type = TOKEN_OPEN_PARENTHESIS;
		next_char();
		break;

	case ';':
		token -> type = TOKEN_SEMICOLON;
		next_char();
		break;

	case '}':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '!':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '#':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '$':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '%':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '&':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '@':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '|':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '~':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;

	case '`':
		leprintf("illegal character '%c' (ASCII #%d)", ch, ch);
		break;
	}
} else {
	token -> type = TOKEN_EOF;
}
}

/* --- utility functions ---------------------------------------------------- */

/* Fetches the next char
 * - Next char fetched from source file
 * - Sets position numbers
 */

void next_char(void)
{
	position.col = column_number;
	static char last_read = '\0';

	 if (ch == EOF) {
		return;
	}

	ch = fgetc(src_file);

	  if (ch == EOF) {
		return;
	}

	   if (last_read == '\n') {
		column_number = 0;
		position.line += 1;
		last_read = '\0';
	}
	column_number += 1;

	 if (ch == '\n') {
		last_read = '\n';
	}
}

/* Process numbers
 * - Builds number up to the specified maximum magnitude.
 * - Stores the value in the number token field.
 */

void process_number(Token * token)
{
	SourcePos start_pos;
	start_pos.col = column_number;

	int k;
	k = column_number;
	int number = ch - '0';
	int newnum;
	while (isdigit(ch)) {
		next_char();

		if (!isdigit(ch)) {
			break;
		}
		newnum = ch - '0';
		if (INT_MAX / 10 < number ||
			(INT_MAX / 10 == number && newnum > INT_MAX % 10)) {
			//integer overflow error
			position.col = k;
			leprintf("number too large");
		}
		number = 10 * number + newnum;
	}
	token -> type = TOKEN_NUMBER;
	token -> value = number;
	position.col = start_pos.col;
}

/* Process string literals
 * - Allocate heap space of the size of the maximum initial string length.
 * - If a string is *about* to overflow while scanning it, double the amount
 *   of space available.
 */

void process_string(Token * token)
{
	SourcePos start_pos;
	start_pos.col = column_number;
	int count = 1;
	int k;
	char add;
	k = column_number;
	size_t i, nstring = MAX_INITIAL_STRING_LEN;
	i = 0;

	//allocate initial heap space
	char * line = (char *) malloc(MAX_INITIAL_STRING_LEN);

	while (ch != '"') {
		add = ch;

		if (ch == '\\') {
			strncat(line, & add, 1);
			next_char();
			if (ch != 'n' && ch != 't' && ch != '"' && ch != '\\') {
				position.col = column_number - 1;
				leprintf("illegal escape code '\\%c' in string", ch);
			} else {
				add = ch;
			}
		}
		strncat(line, & add, 1);
		next_char();
		count++;
		if (ch == EOF) {
			position.col = k - 1;
			leprintf("string not closed");
		}

		if (!isascii(ch) || ch =='\n' || !isprint(ch)) {
			position.col = position.col+1;
			leprintf("non-printable character (ASCII #%d) in string", ch);
		}

		if (count > 1023) {
			count = 0;
			nstring = nstring * 2;
			line = realloc(line, nstring);
		}
	}
	token -> type = TOKEN_STRING;
	token -> string = line;
   // free(line);

	next_char();
	position.col = start_pos.col-1;
}

/* Process words
 * - Uses binary search to determine if word is reserved
 */

void process_word(Token * token)
{
	SourcePos start_pos;
	start_pos.col = column_number;

	char lexeme[MAX_ID_LENGTH + 1] = "";
	char add;
	int i, cmp, low, mid, high;
	i = 0;
	cmp = 0;
	int k;

	k = column_number;
	high = NUM_RESERVED_WORDS;

	while (isalpha(ch) || ch == '_' || isdigit(ch)) {
		add = ch;
		i += 1;

		/* check that the id length is less than the maximum */
		if (i > MAX_ID_LENGTH) {
			position.col = k;
			leprintf("identifier too long");
			break;
		}

		strncat(lexeme, & add, 1);
		next_char();
	}
	strncat(lexeme, & add, '\0');

	low = 0;
	high = NUM_RESERVED_WORDS - 1;
	do {
		//binary search
		mid = (low + high) / 2; //finding the mid of the array
		if ((strcmp(lexeme, reserved[mid].word))<0) //compare the word with mid
			high = mid - 1; //if small then decrement high
		else if ((strcmp(lexeme, reserved[mid].word))>0)
			low = mid + 1; //if greater then increment low
	/*repeat the process till low doesn't becomes high and string is found */
	} while ((strcmp(lexeme, reserved[mid].word)!= 0) && low <= high);
	if ((strcmp(lexeme, reserved[mid].word))==0) { //if string is found
		token -> type = reserved[mid].type;
		position.col = start_pos.col;
	}
/* if id was not recognised as a reserved word, it is an identifier */
	else {
		token -> type = TOKEN_ID;
		strcpy(token -> lexeme, lexeme);
		position.col = start_pos.col;
	}
}

/* Skip nested comments
 * - Skip nested comments recursively strategies
 *   are not allowed.
 * - Terminates with an error if comments are not nested properly.
 */

void skip_comment(void)
{
	SourcePos start_pos;
	start_pos.col = column_number;
	start_pos.line=position.line;
	//int line_num = position.line;

	/* temp int for error checking*/
	int temp1;
	int temp2;
	temp1=column_number;
	temp2=position.line;
	next_char();

	if (ch == EOF) {
		position.col = start_pos.col;
		leprintf("comment not closed");
	}

	while (ch != '}') {
		if (ch == '{') {
			start_pos.col = column_number;
			skip_comment();
		}
		if (ch=='}') {
			break;
		}
		if (ch == EOF) {
			position.col = temp1;
			position.line=temp2;
			leprintf("comment not closed");
		}
		next_char();
	}
	next_char();
	return;
}

