/*
 *
 * testStubs.s --
 *
 *     Stubs for the Test_ system calls.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/testStubs.s,v 1.1 89/07/11 19:52:21 nelson Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"

/*SYS_CALL(Test_GetChar, 	SYS_TEST_GETCHAR)*/
/*SYS_CALL(Test_GetLine, 	SYS_TEST_GETLINE)*/
SYS_CALL(Test_PrintOut, SYS_TEST_PRINTOUT)
SYS_CALL(Test_Rpc, 	SYS_TEST_RPC)
/*SYS_CALL(Test_Stats, 	SYS_TEST_STATS)*/
