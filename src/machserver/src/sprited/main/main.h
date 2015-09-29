/*
 * main.h --
 *
 *	External definitions for system startup and shutdown.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /r3/kupfer/spriteserver/src/sprited/main/RCS/main.h,v 1.7 91/11/14 10:02:09 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _MAIN
#define _MAIN

#include <mach.h>

/* 
 * This is the maximum number of arguments that can be passed to the init 
 * program (not counting the trailing nil string pointer).  I suppose this 
 * array should be allocated dynamically...
 */
#define MAIN_MAX_INIT_ARGS	10

extern Boolean main_DebugFlag;
extern Boolean main_MultiThreaded;
extern int main_NumRpcServers;  /* # of rpc servers to spawn off */
extern char *main_InitArgArray[];
extern char *main_InitPath;

/* procedures */

#endif /* _MAIN */
