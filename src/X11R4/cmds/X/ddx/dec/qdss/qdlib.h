/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef QDLIB_H
#define QDLIB_H

/*
 * THIS FILE IS A GROSS HACK AND SHOULD ONLY BE USED BY .C FILES WHICH ARE
 * GOING TO GO AWAY EVENTUALLY		XXX 
 */
#include <sys/types.h>
#include <stdio.h>	/* debug */

#include "X.h"
#include "windowstr.h"
#include "regionstr.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "Xproto.h"
#include "Xprotostr.h"
#include "mi.h"
#include "Xmd.h"

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include "qduser.h"
#include "qdioctl.h"
#include "qdreg.h"

#include "qd.h"

#endif
