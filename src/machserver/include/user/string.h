/*
 * string.h --
 *
 *	Declarations of ANSI C library procedures for string handling.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/string.h,v 1.9 91/04/08 00:10:00 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _STRING
#define _STRING

#include <cfuncproto.h>

extern _VoidPtr	memchr _ARGS_((_CONST char *s, int c, int n));
extern int	memcmp _ARGS_((_CONST char *s1, _CONST char *s2, int n));
extern _VoidPtr	memcpy _ARGS_((char *t, _CONST char *f, int n));
extern _VoidPtr	memmove _ARGS_((char *t, _CONST char *f, int n));
extern _VoidPtr	memset _ARGS_((char *s, int c, int n));

extern int	strcasecmp _ARGS_((_CONST char *s1, _CONST char *s2));
extern char *	strcat _ARGS_((char *dst, _CONST char *src));
extern char *	strchr _ARGS_((_CONST char *string, int c));
extern int	strcmp _ARGS_((_CONST char *s1, _CONST char *s2));
extern char *	strcpy _ARGS_((char *dst, _CONST char *src));
extern int	strcspn _ARGS_((_CONST char *string, _CONST char *chars));
extern char *	strdup _ARGS_((_CONST char *string));
extern char *	strerror _ARGS_((int error));
extern int	strlen _ARGS_((_CONST char *string));
extern int	strncasecmp _ARGS_((_CONST char *s1, _CONST char *s2, int n));
extern char *	strncat _ARGS_((char *dst, _CONST char *src, int numChars));
extern int	strncmp _ARGS_((_CONST char *s1, _CONST char *s2, int nChars));
extern char *	strncpy _ARGS_((char *dst, _CONST char *src, int numChars));
extern char *	strpbrk _ARGS_((_CONST char *string, _CONST char *chars));
extern char *	strrchr _ARGS_((_CONST char *string, int c));
extern int	strspn _ARGS_((_CONST char *string, _CONST char *chars));
extern char *	strstr _ARGS_((_CONST char *string, _CONST char *substring));
extern char *	strtok _ARGS_((char *s, _CONST char *delim));

/*
 * Obsolete library procedures from BSD, supported for compatibility:
 */

extern char *	index _ARGS_((_CONST char *string, int c));
extern char *	rindex _ARGS_((_CONST char *string, int c));

#endif /* _STRING */
