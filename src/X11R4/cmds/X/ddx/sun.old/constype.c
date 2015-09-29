/*
 * $XConsortium: constype.c,v 1.4 89/09/26 19:57:08 jim Exp $
 * 
 * consoletype - utility to print out string identifying Sun console type
 *
 * Copyright 1988 SRI 
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SRI not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SRI makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Doug Moran, SRI
 */

/*
SUN-SPOTS DIGEST         Thursday, 17 March 1988       Volume 6 : Issue 31

Date:    Wed, 2 Mar 88 14:50:26 PST
From:    Doug Moran <moran@ai.sri.com>
Subject: Program to determine console type

There have been several requests in this digest for programs to determine
the type of the console.  Below is a program that I wrote to produce an
identifying string (I start suntools in my .login file and use this pgm to
determine which arguments to use).

Caveat:  my cluster has only a few of these monitor types, so the pgm has
not been fully tested.

Note on coding style: the function wu_fbid is actually located in a local
library, accounting for what otherwise might appear to be a strange coding
style.
*/
#include <stdio.h>
#include <strings.h>

main (argc, argv)
    int argc;
    char **argv;
{
    char *wu_fbid();
    int retval = -1;
    char *consoleid = wu_fbid("/dev/fb", &retval);
    int print_num = 0;

    if (argc > 1 && strncmp (argv[1], "-num", strlen(argv[1])) == 0)
	print_num = 1;

    printf ("%s", consoleid ? consoleid : "tty");
    if (print_num) {
	printf (" %d", retval);
    }
    putchar ('\n');
}
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sun/fbio.h>

/* decoding as of Release 3.4 : fbio.h 1.3 87/01/09 SMI */
	/* the convention for entries in this table is to translate the
	 * macros for frame buffer codes (in <sun/fbio.h>) to short names
	 * thus:
	 *	FBTYPE_SUNxBW		becomes bwx
	 *	FBTYPE_SUNxCOLOR	becomes cgx
	 *	FBTYPE_SUNxGP		becomes gpx
	 *	FBTYPE_NOTSUN[1-9]	becomes ns[A-J]
	 */
static char *decode_fb[FBTYPE_LASTPLUSONE] = {
	"bw1", "cg1",
	"bw2", "cg2",
	"gp2",
	"bw3", "cg3",
	"bw4", "cg4",
	"nsA", "nsB", "nsC", 
#if FBTYPE_LASTPLUSONE > 12
	"gx", "rop", "vid", 
	"res5", "res4", "res3", "res2", "res1"
#endif
	};

char *
wu_fbid(fbname, retval)
	char *fbname;
	int *retval;
{
	struct fbgattr fbattr;
	int fd, ioctl_ret;
	*retval = -1;
	if ( (fd = open(fbname, O_RDWR, 0)) == -1 )
		return(0);
		/* FBIOGATTR fails for early frame buffer types */
	if (ioctl_ret = ioctl(fd,FBIOGATTR,&fbattr)) {	/*success=>0(false)*/
		ioctl_ret = ioctl(fd, FBIOGTYPE, &fbattr.fbtype);
	}
	*retval = fbattr.fbtype.fb_type;
	close(fd);
	if ( ioctl_ret == -1 )
		return(0);
	    /* The binary is obsolete and needs to be re-compiled:
	     * the ioctl returned a value beyond what was possible
	     * when the program was compiled */
	if (fbattr.fbtype.fb_type>=FBTYPE_LASTPLUSONE)
		return("unk");
	    /* The source is obsolete.  The table "decode_fb" does not
	     * have entries for some of the values returned by the ioctl.
	     * Compare <sun/fbio.h> to the entries in "decode_fb" */
	if ( decode_fb[fbattr.fbtype.fb_type] == 0 )	/* decode_fb is obs */
                return("unk");
	return(decode_fb[fbattr.fbtype.fb_type]);
}
