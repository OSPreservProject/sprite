/*
 * testRedef.h --
 *
 *	Redefinitions of "test" routines for use with /dev/console.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/testRedef.h,v 1.1 92/03/23 14:20:34 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _TESTREDEF
#define _TESTREDEF

/* 
 * This might cause some problems if this header file is included in the 
 * middle of a function, but only if stdio.h wasn't already included.
 */
#include <stdio.h>

/* 
 * There isn't a trivial equivalent for Test_PutTime.  It was a 
 * special-purpose hack, anyway, so don't bother fixing it here.
 */

#define Test_PutDecimal(val)	printf("%d", (val))
#define Test_PutHex(val)	printf("0x%x", (val))
#define Test_PutOctal(val)	printf("0%o", (val))
#define Test_PutMessage(str)	printf("%s", (str))
#define Test_PutString(str)	printf("%s", (str))
#define Test_GetString(buf, bufLen)	fgets((buf), (bufLen), stdin)

#endif /* _TESTREDEF */
