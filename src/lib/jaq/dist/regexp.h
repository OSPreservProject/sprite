/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */

#include "cfuncproto.h"
#define NULL 0

#ifndef _REGEXP
#define _REGEXP 1

#define NSUBEXP  10
typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

extern regexp *regcomp _ARGS_((char *exp));
extern int regexec     _ARGS_((regexp *prog, char *string));
extern void regsub     _ARGS_((regexp *prog, char *source, char *dest));
extern void regerror   _ARGS_((char *msg));

#endif /* REGEXP */
