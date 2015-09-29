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


#include 	<sys/types.h>
#include	"miscstruct.h"
#include	"Ultrix2.0inc.h"
#include	<vaxuba/qduser.h>
#include	<vaxuba/qdreg.h>
#include	"tlsg.h"
#include	"tl.h"

int	fd_qdss;     /* global qdss file descriptor, same for sg and qdss */
int	fdqz;	     /* as above, for X */
/* uVaxII/GPX */
struct qdmap		Qdss;
struct dga *		Dga;
VOLATILE struct adder *		Adder; /* use for both */
struct duart * 		Duart;
VOLATILE struct DMAreq_header *  DMAheader;
char *			Template;
char *			Memcsr;
char *			Redmap;   /* Use for both uVaxII and Vaxstar */
char *			Bluemap;  /* Use for both uVaxII and Vaxstar */
char *                  Greenmap; /* Use for both uVaxII and Vaxstar */
/* endif uVaxII/GPX */

/* Vaxstar */
struct sgmap            Sg;
struct color_cursor *   cursor_addr[2];
struct color_buf *      color_buf;
struct FIFOreq_header * FIFOheader;
char *                  Fcc;
char *			Vdac;
char *			Vrback;
char *			Fiforam;
char *			Cur;
struct color_buf *      Co[2];
short *			VDIdev_reg;
short *			DMAdev_reg;
int			FIFOflag;
int			Logic_reg [2];
int			Fore_color [2];
int			Back_color [2];
int			Source [2];
int			QILmode [2];
int			QDSSplanes [2];
int			QDSSall_planes [2];
/* endif Vaxstar */

int cookie;
int nOpen;           /* incremented by dcENABLE
                        decremented by dcDISABLE */
short oldwakeflags;  /* select type flags from before llge started
		        restored at last dcDISABLE */

DDXPointRec _tlTranslate = {0, 0};      /* hardware translation point	 */

unsigned int tlMask = (unsigned)-1; /* 0 bit for planes currently masked */

int umtable[16] =
{
	LF_0    | FULL_SRC_RESOLUTION,     LF_DSA  | FULL_SRC_RESOLUTION,
	LF_DNSA | FULL_SRC_RESOLUTION,     LF_S    | FULL_SRC_RESOLUTION,
	LF_DSNA | FULL_SRC_RESOLUTION,     LF_D    | FULL_SRC_RESOLUTION,
	LF_DSX  | FULL_SRC_RESOLUTION,     LF_DSO  | FULL_SRC_RESOLUTION,
	LF_DSON | FULL_SRC_RESOLUTION,     LF_DSXN | FULL_SRC_RESOLUTION,
	LF_DN   | FULL_SRC_RESOLUTION,     LF_DNSO | FULL_SRC_RESOLUTION,
	LF_SN   | FULL_SRC_RESOLUTION,     LF_DSNO | FULL_SRC_RESOLUTION,
	LF_DSAN | FULL_SRC_RESOLUTION,     LF_1    | FULL_SRC_RESOLUTION
};

int	DragonPix = DRAGONPIX;	/* max y size for offscreen pixmap */
