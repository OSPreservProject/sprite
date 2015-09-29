/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef lint
static  char sccsid[] = "@(#)kbd_mode.c 7.1 87/04/13";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 *      kbd_mode:       set keyboard encoding mode
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sundev/kbio.h>
#include <sundev/kbd.h>
#include <stdio.h>

static void         die(), usage();
static int          kbd_fd;

main(argc, argv)
    int             argc;
    char          **argv;
{
    int             code, translate;

    if ((kbd_fd = open("/dev/kbd", O_RDONLY, 0)) < 0) {
	die("Couldn't open /dev/kbd");
    }
    argc--; argv++;
    if (argc-- && **argv == '-') {
	code = *(++*argv);
    } else {
	usage();
    }
    switch (code) {
      case 'a':
      case 'A':
	translate = TR_ASCII;
	break;
      case 'e':
      case 'E':
	translate = TR_EVENT;
	break;
      case 'n':
      case 'N':
	translate = TR_NONE;
	break;
      case 'u':
      case 'U':
	translate = TR_UNTRANS_EVENT;
	break;
      default:
	usage();
    }
    if (ioctl(kbd_fd, KIOCTRANS, (caddr_t) &translate)) {
	die("Couldn't initialize translation to Event");
    }
    exit(0);
}

static void
die(msg)
    char        *msg;
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void
usage()
{
    int             translate;

    if (ioctl(kbd_fd, KIOCGTRANS, (caddr_t) &translate)) {
	die("Couldn't inquire current translation");
     }
    fprintf(stderr, "kbd_mode {-a | -e | -n | -u }\n");
    fprintf(stderr, "\tfor ascii, encoded (normal) SunView events,\n");
    fprintf(stderr, " \tnon-encoded, or unencoded SunView events, resp.\n");
    fprintf(stderr, "Current mode is %s.\n",
		(   translate == 0 ?    "n (non-translated bytes)"      :
		 (  translate == 1 ?    "a (ascii bytes)"               :
		  ( translate == 2 ?    "e (encoded events)"            :
		  /* translate == 3 */  "u (unencoded events)"))));
    exit(1);
}


