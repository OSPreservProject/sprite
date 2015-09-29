#
# Prototype Makefile for machine-dependent directories.
#
# A file of this form resides in each ".md" subdirectory of a
# command.  Its name is typically "md.mk".  During makes in the
# parent directory, this file (or a similar file in a sibling
# subdirectory) is included to define machine-specific things
# such as additional source and object files.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.md
# Fri May 18 15:07:56 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= maskbits.c mfbbitblt.c mfbbres.c mfbbresd.c mfbbstore.c mfbclip.c mfbcmap.c mfbfillarc.c mfbfillrct.c mfbfillsp.c mfbfont.c mfbgc.c mfbgetsp.c mfbhrzvert.c mfbimage.c mfbimggblt.c mfbline.c mfbmisc.c mfbpixmap.c mfbplygblt.c mfbpntarea.c mfbpntwin.c mfbpolypnt.c mfbpushpxl.c mfbscrclse.c mfbscrinit.c mfbsetsp.c mfbtegblt.c mfbtile.c mfbwindow.c mfbzerarc.c
HDRS		= fastblt.h maskbits.h mfb.h
MDPUBHDRS	= 
OBJS		= sun4.md/maskbits.o sun4.md/mfbbitblt.o sun4.md/mfbbres.o sun4.md/mfbbresd.o sun4.md/mfbbstore.o sun4.md/mfbclip.o sun4.md/mfbcmap.o sun4.md/mfbfillarc.o sun4.md/mfbfillrct.o sun4.md/mfbfillsp.o sun4.md/mfbfont.o sun4.md/mfbgc.o sun4.md/mfbgetsp.o sun4.md/mfbhrzvert.o sun4.md/mfbimage.o sun4.md/mfbimggblt.o sun4.md/mfbline.o sun4.md/mfbmisc.o sun4.md/mfbpixmap.o sun4.md/mfbplygblt.o sun4.md/mfbpntarea.o sun4.md/mfbpntwin.o sun4.md/mfbpolypnt.o sun4.md/mfbpushpxl.o sun4.md/mfbscrclse.o sun4.md/mfbscrinit.o sun4.md/mfbsetsp.o sun4.md/mfbtegblt.o sun4.md/mfbtile.o sun4.md/mfbwindow.o sun4.md/mfbzerarc.o
CLEANOBJS	= sun4.md/maskbits.o sun4.md/mfbbitblt.o sun4.md/mfbbres.o sun4.md/mfbbresd.o sun4.md/mfbbstore.o sun4.md/mfbclip.o sun4.md/mfbcmap.o sun4.md/mfbfillarc.o sun4.md/mfbfillrct.o sun4.md/mfbfillsp.o sun4.md/mfbfont.o sun4.md/mfbgc.o sun4.md/mfbgetsp.o sun4.md/mfbhrzvert.o sun4.md/mfbimage.o sun4.md/mfbimggblt.o sun4.md/mfbline.o sun4.md/mfbmisc.o sun4.md/mfbpixmap.o sun4.md/mfbplygblt.o sun4.md/mfbpntarea.o sun4.md/mfbpntwin.o sun4.md/mfbpolypnt.o sun4.md/mfbpushpxl.o sun4.md/mfbscrclse.o sun4.md/mfbscrinit.o sun4.md/mfbsetsp.o sun4.md/mfbtegblt.o sun4.md/mfbtile.o sun4.md/mfbwindow.o sun4.md/mfbzerarc.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
