/**
 * @file    error.c
 *
 * Implementations for error reporting, memory allocation, and string funtions.
 *
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

/* Adapted extensively from "The Practice of Programming" */
/* by Brian W. Kernighan and Rob Pike                     */
/* Copyright (C) 1999 Lucent Technologies                 */

#include "error.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* --- ASCII colours -------------------------------------------------------- */

#define ESC                      "\033["
#define BOLD                     "1;"
#define ASCII_RESET              ESC "m"

#define BLACK                    "30m"
#define RED                      "31m"
#define GREEN                    "32m"
#define YELLOW                   "33m"
#define BLUE                     "34m"
#define MAGENTA                  "35m"
#define CYAN                     "36m"
#define WHITE                    "37m"

#define HIGH_BLACK               "90m"
#define HIGH_RED                 "91m"
#define HIGH_GREEN               "92m"
#define HIGH_YELLOW              "93m"
#define HIGH_BLUE                "94m"
#define HIGH_MAGENTA             "95m"
#define HIGH_CYAN                "96m"
#define HIGH_WHITE               "97m"

#define ASCII_BLACK              ESC BLACK
#define ASCII_RED                ESC RED
#define ASCII_GREEN              ESC GREEN
#define ASCII_YELLOW             ESC YELLOW
#define ASCII_BLUE               ESC BLUE
#define ASCII_MAGENTA            ESC MAGENTA
#define ASCII_CYAN               ESC CYAN
#define ASCII_WHITE              ESC WHITE

#define ASCII_BOLD_BLACK         ESC BOLD BLACK
#define ASCII_BOLD_RED           ESC BOLD RED
#define ASCII_BOLD_GREEN         ESC BOLD GREEN
#define ASCII_BOLD_YELLOW        ESC BOLD YELLOW
#define ASCII_BOLD_BLUE          ESC BOLD BLUE
#define ASCII_BOLD_MAGENTA       ESC BOLD MAGENTA
#define ASCII_BOLD_CYAN          ESC BOLD CYAN
#define ASCII_BOLD_WHITE         ESC BOLD WHITE

#define ASCII_HIGH_BLACK         ESC HIGH_BLACK
#define ASCII_HIGH_RED           ESC HIGH_RED
#define ASCII_HIGH_GREEN         ESC HIGH_GREEN
#define ASCII_HIGH_YELLOW        ESC HIGH_YELLOW
#define ASCII_HIGH_BLUE          ESC HIGH_BLUE
#define ASCII_HIGH_MAGENTA       ESC HIGH_MAGENTA
#define ASCII_HIGH_CYAN          ESC HIGH_CYAN
#define ASCII_HIGH_WHITE         ESC HIGH_WHITE

#define ASCII_BOLD_HIGH_BLACK    ESC BOLD HIGH_BLACK
#define ASCII_BOLD_HIGH_RED      ESC BOLD HIGH_RED
#define ASCII_BOLD_HIGH_GREEN    ESC BOLD HIGH_GREEN
#define ASCII_BOLD_HIGH_YELLOW   ESC BOLD HIGH_YELLOW
#define ASCII_BOLD_HIGH_BLUE     ESC BOLD HIGH_BLUE
#define ASCII_BOLD_HIGH_MAGENTA  ESC BOLD HIGH_MAGENTA
#define ASCII_BOLD_HIGH_CYAN     ESC BOLD HIGH_CYAN
#define ASCII_BOLD_HIGH_WHITE    ESC BOLD HIGH_WHITE

/* --- error routines ------------------------------------------------------- */

SourcePos position;

#ifndef __APPLE__
static char *pname = NULL;
#endif
static char *sname = NULL;

static void _weprintf(const char *pre, const SourcePos *pos, const char *fmt,
		va_list args)
{
	int istty = isatty(2);
	const char *ac_end = (istty ? ASCII_RESET : "");
	const char *ac_src = (istty ? ASCII_BOLD_WHITE : "");
	const char *ac_pos = (istty ? ASCII_BOLD_WHITE : "");
	const char *progname = getprogname();
	const char *srcname = getsrcname();

	fflush(stdout);
	if (progname != NULL)
		fprintf(stderr, "%s:", progname);
	if (srcname != NULL)
		fprintf(stderr, " %s%s:%s", ac_src, srcname, ac_end);
	if (pos != NULL)
		fprintf(stderr, "%s%d:%d%s:", ac_pos, pos->line, pos->col, ac_end);
	if (pre != NULL)
		fprintf(stderr, " %s ", pre);
	else
		fprintf(stderr, " ");

	vfprintf(stderr, fmt, args);

	if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s", strerror(errno));
	fprintf(stderr, "\n");
}

void eprintf(const char *fmt, ...)
{
	int istty = isatty(2);
	va_list args;
	const char *pre =
		(istty ? ASCII_BOLD_RED "error:" ASCII_RESET : "error:");

	va_start(args, fmt);
	_weprintf(pre, NULL, fmt, args);
	va_end(args);
	exit(2);
}

void leprintf(const char *fmt, ...)
{
	int istty = isatty(2);
	va_list args;
	const char *pre =
		(istty ? ASCII_BOLD_RED "error:" ASCII_RESET : "error:");

	va_start(args, fmt);
	_weprintf(pre, &position, fmt, args);
	va_end(args);
	exit(2);
}

void weprintf(const char *fmt, ...)
{
	int istty = isatty(2);
	va_list args;
	const char *pre =
		(istty ? ASCII_BOLD_YELLOW "warning:" ASCII_RESET : "warning:");

	va_start(args, fmt);
	_weprintf(pre, NULL, fmt, args);
	va_end(args);
}

void teprintf(const char *tag, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	_weprintf(tag, &position, fmt, args);
	va_end(args);
	exit(3);
}

char *estrdup(const char *s)
{
	char *t;
	t = malloc((strlen(s) + 1) * sizeof(char));
	if (t == NULL)
		eprintf("estrdup(\"%.20s\") failed:", s);
	strcpy(t, s);
	return t;
}

char *westrdup(const char *s)
{
	char *t;
	t = malloc((strlen(s) + 1) * sizeof(char));
	if (t == NULL)
		weprintf("estrdup(\"%.20s\") failed:", s);
	strcpy(t, s);
	return t;
}

void *emalloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes failed:", n);
	return p;
}

void *wemalloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		weprintf("malloc of %u bytes failed:", n);
	return p;
}

void *erealloc(void *vp, size_t n)
{
	void *p;

	p = realloc(vp, n);
	if (p == NULL)
		eprintf("realloc of %u bytes failed:", n);
	return p;
}

void *werealloc(void *vp, size_t n)
{
	void *p;

	p = realloc(vp, n);
	if (p == NULL)
		weprintf("realloc of %u bytes failed:", n);
	return p;
}

#ifndef __APPLE__
void setprogname(char *s)
{
	char *c;

	if ((c = strrchr(s, '/')) == NULL)
		c = s;
	else
		c++;
	pname = estrdup(c);
}
#endif

void setsrcname(char *s)
{
	char *c;

	if ((c = strrchr(s, '/')) == NULL)
		c = s;
	else
		c++;
	sname = estrdup(c);
}

#ifndef __APPLE__
char *getprogname(void)
{
	return pname;
}
#endif

char *getsrcname(void)
{
	return sname;
}

void freeprogname(void)
{
#ifndef __APPLE__
	free(pname);
#endif
}

void freesrcname(void)
{
	free(sname);
}
