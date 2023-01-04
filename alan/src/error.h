/**
 * @file    error.h
 * @brief   Error reporting, memory allocation, and string functions.
 *
 * This file contains prototypes for error reporting (with the program name,
 * source file position, and output tags), as well as wrapped versions of
 * allocation and string duplication functions where any error will terminate
 * the program.  Warning functions prepend the string "warning" to the output,
 * and do not terminate the program.
 *
 * Some of the functions were adapted from "The Practice of Programming" by
 * Brian W. Kernighan and Rob Pike, and they are copyright (C) 1999 Lucent
 * Technologies.
 *
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */

#ifndef ERROR_H
#define ERROR_H

/** a place (position) in the source file */
typedef struct {
	int line;  /**< the line number   */
	int col;   /**< the column number */
} SourcePos;

extern SourcePos position;

/**
 * Displays an error message on the standard error stream and exit.
 *
 * @param[in]   fmt
 *     a printf format string
 * @param[in]   ...
 *     the variable arguments to the format string
 */
void eprintf(const char *fmt, ...);

/**
 * Displays an error message on the standard error stream, with the current
 * position prepended, and exit.
 *
 * @param[in]   fmt
 *     a printf format string
 * @param[in]   ...
 *     the variable arguments to the format string
 */
void leprintf(const char *fmt, ...);

/**
 * Displays an error message on the standard error stream, with a tag prepended,
 * and exit.
 *
 * @param[in]   tag
 *     the tag to prepend to the error message
 * @param[in]   fmt
 *     a printf format string
 * @param[in]   ...
 *     the variable arguments to the format string
 */
void teprintf(const char *tag, const char *fmt, ...);

/**
 * Displays a warning message on the standard error stream.
 *
 * @param[in]   fmt
 *     a print format string
 * @param[in]   ...
 *     the variable arguments to the format string
 */
void weprintf(const char *fmt, ...);

/**
 * Duplicates a string, and terminates the program with a message on the
 * standard error stream if the duplication fails.
 *
 * @param[in]   s
 *     the string to duplicate
 * @return      a pointer to newly allocated memory that contains a copy of the
 *              string
 */
char *estrdup(const char *s);

/**
 * Duplicates a string, and displays a warning on the standard error stream if
 * the duplication fails.
 *
 * @param[in]   s
 *     the string to duplicate
 * @return      a pointer to newly allocated memory that contains a copy of the
 *              string
 */
char *westrdup(const char *s);

/**
 * Allocates memory, and terminates the program with a message on the standard
 * error stream if the allocation fails.
 *
 * @param[in]   n
 *     the number of bytes to allocate
 * @return      a pointer to the newly allocated memory
 */
void *emalloc(size_t n);

/**
 * Allocates memory, and displays a warning on the standard error stream if the
 * allocation fails.
 *
 * @param[in]   n
 *     the number of bytes to allocate
 * @return      a pointer to the newly allocated memory
 */
void *wemalloc(size_t n);

/**
 * Reallocates memory, and terminates the program with a message on the standard
 * error stream if the reallocation fails.
 *
 * @param[in]   vp
 *     a pointer to the existing memory of which to adjust the size
 * @param[in]   n
 *     the new byte size
 * @return      a pointer to the reallocated memory
 * @see         system documentation for realloc
 */
void *erealloc(void *vp, size_t n);

/**
 * Reallocates memory, and display a warning message on the standard error
 * stream if the reallocation fails.
 *
 * @param[in]   vp
 *     a pointer to the existing memory of which to adjust the size
 * @param[in]   n
 *     the new byte size
 * @return      a pointer to the reallocated memory
 * @see         system documentation for realloc
 */
void *werealloc(void *vp, size_t n);

/**
 * Frees the program name.
 */
void freeprogname(void);

/**
 * Frees the source name.
 */
void freesrcname(void);

#ifndef __APPLE__
/**
 * Returns the stored program name.
 *
 * @return      the stored program name
 */
char *getprogname(void);
#endif

/**
 * Returns the stored source name.
 *
 * @return      the stored source name
 */
char *getsrcname(void);

#ifndef __APPLE__
/**
 * Sets the program name.
 *
 * @param[in]   s
 *     the program name
 */
void setprogname(char *s);
#endif

/**
 * Sets the source name.
 *
 * @param[in]   s
 *     the source name
 */
void setsrcname(char *s);

#endif /* ERROR_H */
