/*
 * sysTestCall.h --
 *
 *     The test system calls for debugging.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _SYSTESTCALL
#define _SYSTESTCALL

#include <sprite.h>
#include <sys.h>

struct test_args {
    int argArray[SYS_MAX_ARGS];
};

extern	int	Test_PrintOut _ARGS_((struct test_args args));
extern	int	Test_GetLine _ARGS_((char *string, int length));
extern	int	Test_GetChar _ARGS_((char *charPtr));

#endif /* _SYSTESTCALL */
