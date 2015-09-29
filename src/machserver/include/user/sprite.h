/*
 * sprite.h --
 *
 * Common constants and type declarations for Sprite.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/sprite.h,v 1.3 92/05/12 11:54:30 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SPRITE
#define _SPRITE

#include "cfuncproto.h"
/*
 * A boolean type is defined as an integer, not an enum. This allows a
 * boolean argument to be an expression that isn't strictly 0 or 1 valued.
 */

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#ifndef _ASM
typedef int Boolean;

/*
 * Functions that must return a status can return a ReturnStatus to
 * indicate success or type of failure.
 */

typedef int  ReturnStatus;
#endif /* _ASM */

/*
 * The following statuses overlap with the first 2 generic statuses 
 * defined in status.h:
 *
 * SUCCESS			There was no error.
 * FAILURE			There was a general error.
 */

#define	SUCCESS			0x00000000
#define	FAILURE			0x00000001


/*
 * A nil pointer must be something that will cause an exception if
 * referenced.  In native Sprite there are two nils: the kernel's nil
 * and the nil used by user processes.  In the Sprite server these two
 * are the same.  
 * XXX Unfortunately, there's some user code that knows about NIL and knows 
 * that it's set to -1.  Maybe it would be better to make NIL -1 for the 
 * server, too.
 */

#ifdef SPRITED
#define NIL 		0
#else
#define NIL		0xFFFFFFFF
#endif
#define USER_NIL 	0
#ifndef NULL
#define NULL	 	0
#endif

#ifndef _ASM
/*
 * An address is just a pointer in C.  It is defined as a character pointer
 * so that address arithmetic will work properly, a byte at a time.
 */

typedef char *Address;

/*
 * ClientData is an uninterpreted word.  It is defined as an int so that
 * kdbx will not interpret client data as a string.  Unlike an "Address",
 * client data will generally not be used in arithmetic.
 */

#ifndef _CLIENTDATA
typedef int *ClientData;
#define _CLIENTDATA
#endif

#ifndef __STDC__
#define volatile
#define const
#endif

extern void panic();

#endif /* !_ASM */

#endif /* _SPRITE */
