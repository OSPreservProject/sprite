/*
 * pdev.h --
 *
 * Definitions for pseudo-device library routines.  The man page
 * for pdev (or Pdev_Open) has necessary documentation.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/pdev.h,v 1.8 90/02/19 14:44:15 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _PDEVLIB
#define _PDEVLIB

#include <fs.h>
#include <dev/pdev.h>

/*
 * Boolean that can be toggled by applications command line arguments.
 * This causes print statements that trace pdev/pfs operations.
 */
extern int pdev_Trace;

/*
 * The library keeps a set of callback procedures, one for each pdev request
 * that arrives on a request stream.  Fields can be set to NULL to get
 * a default handler for the operation.  See the man page for the
 * calling sequence of each call-back procedure.
 */

typedef struct {
    int (*open)();		/* PDEV_OPEN */
    int (*read)();		/* PDEV_READ */
    int (*write)();		/* PDEV_WRITE and PDEV_WRITE_ASYNC */
    int (*ioctl)();		/* PDEV_IOCTL */
    int (*close)();		/* PDEV_CLOSE */
    /*
     * The following are only used for pseudo-device connnections
     * into a pseudo-file-system.  For regular pseudo-devices the
     * kernel completely handles attributes.
     */
    int (*getAttr)();		/* PDEV_GET_ATTR */
    int (*setAttr)();		/* PDEV_SET_ATTR */
} Pdev_CallBacks;

/*
 * A Pdev_Stream is is passed to the PDEV_OPEN call-back handler.
 * This provides a handle on the particular stream to the pseudo-device.
 * The handle for the stream is passed to each call-back procedure.
 */
typedef struct Pdev_Stream {
    unsigned int magic;		/* Either PDEV_MAGIC or PDEV_STREAM_MAGIC */
    int streamID;		/* Sprite stream identifier, either of
				 * control or server stream depending
				 * on context of the token. */
    ClientData clientData;	/* For use by the client of the Pdev package */
} Pdev_Stream;

typedef char *Pdev_Token;	/* Opaque token for the pseudo-device */

#define PDEV_MAGIC		0xabcd1234
#define PDEV_STREAM_MAGIC	0xa1b2c3d4

#ifndef max
#define max(a, b) \
    ( ((a) > (b)) ? (a) : (b) )
#endif

extern char pdev_ErrorMsg[];

extern	Pdev_Token		Pdev_Open();
extern	void			Pdev_Close();
extern	int 		      (*Pdev_SetHandler())();
extern	int			Pdev_EnumStreams();
extern	int			Pdev_GetStreamID();

extern	Pdev_Stream	       *PdevSetup();

#endif /* _PDEVLIB */
