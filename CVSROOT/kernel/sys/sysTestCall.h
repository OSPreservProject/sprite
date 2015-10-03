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

extern	int 	Test_PrintOut _ARGS_((int arg0, int arg1, int arg2, int arg3, 
				      int arg4, int arg5, int arg6, int arg7, 
				      int arg8, int arg9));
extern	int	Test_GetLine _ARGS_((char *string, int length));
extern	int	Test_GetChar _ARGS_((char *charPtr));

#endif /* _SYSTESTCALL */
