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
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/testStubs.s,v 1.1 90/01/19 10:19:34 fubar Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"

/*SYS_CALL(1,	Test_GetChar, 	SYS_TEST_GETCHAR)*/
/*SYS_CALL(2,	Test_GetLine, 	SYS_TEST_GETLINE)*/
SYS_CALL(10,	Test_PrintOut,	SYS_TEST_PRINTOUT)
SYS_CALL(4,	Test_Rpc, 	SYS_TEST_RPC)
/*SYS_CALL(Test_Stats, 	SYS_TEST_STATS)*/
