/*
 * test.h --
 *
 *	User declarations for test system calls.
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
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/test.h,v 1.5 92/07/17 16:32:34 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _TESTUSER
#define _TESTUSER

#define TEST_BUFFER_SIZE	1024

#ifndef _MiG

#include <sprite.h>

/* 
 * Definitions for MIG-generated stubs.
 */
typedef char Test_MessageBuffer[TEST_BUFFER_SIZE];

extern void	Test_PutDecimal();
extern void	Test_PutHex();
extern void	Test_PutOctal();
extern void	Test_PutMessage();
extern void	Test_PutTime();
extern void	Test_PutString();
extern void	Test_GetString();

extern void	Test_MemCheck();
extern void	Test_Return1();
extern void	Test_Return2();

#endif /* _MiG */
#endif /* _TESTUSER */
