/*
 * termio.h --
 *
 *	Declarations of structures and flags for controlling the `termio'
 *      general terminal interface.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/sys/RCS/termios.h,v 1.1 89/01/06 07:06:35 rab Exp $
 */

#ifndef _TERMIO_
#define _TERMIO_

#define	TCGETA		_IOR('T', 1, struct termio)
#define	TCSETA		_IOW('T', 2, struct termio)
#define	TCSETAW		_IOW('T', 3, struct termio)
#define	TCSETAF		_IOW('T', 4, struct termio)
#define	TCSBRK		_IO('T', 5)

#define	NCC	8

/*
 * Ioctl control packet
 */
struct termio {
	unsigned short	c_iflag;	/* input */
	unsigned short	c_oflag;	/* output */
	unsigned short	c_cflag;	/* control */
	unsigned short	c_lflag;	/* line discipline */
	char		c_line;		/* line discipline number */
	unsigned char	c_cc[NCC];	/* control characters */
};

#endif /* _TERMIO */

