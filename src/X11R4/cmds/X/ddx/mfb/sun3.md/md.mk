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
# Fri May 18 15:07:30 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= maskbits.c mfbbitblt.c mfbbres.c mfbbresd.c mfbbstore.c mfbclip.c mfbcmap.c mfbfillarc.c mfbfillrct.c mfbfillsp.c mfbfont.c mfbgc.c mfbgetsp.c mfbhrzvert.c mfbimage.c mfbimggblt.c mfbline.c mfbmisc.c mfbpixmap.c mfbplygblt.c mfbpntarea.c mfbpntwin.c mfbpolypnt.c mfbpushpxl.c mfbscrclse.c mfbscrinit.c mfbsetsp.c mfbtegblt.c mfbtile.c mfbwindow.c mfbzerarc.c
HDRS		= fastblt.h maskbits.h mfb.h
MDPUBHDRS	= 
OBJS		= sun3.md/maskbits.o sun3.md/mfbbitblt.o sun3.md/mfbbres.o sun3.md/mfbbresd.o sun3.md/mfbbstore.o sun3.md/mfbclip.o sun3.md/mfbcmap.o sun3.md/mfbfillarc.o sun3.md/mfbfillrct.o sun3.md/mfbfillsp.o sun3.md/mfbfont.o sun3.md/mfbgc.o sun3.md/mfbgetsp.o sun3.md/mfbhrzvert.o sun3.md/mfbimage.o sun3.md/mfbimggblt.o sun3.md/mfbline.o sun3.md/mfbmisc.o sun3.md/mfbpixmap.o sun3.md/mfbplygblt.o sun3.md/mfbpntarea.o sun3.md/mfbpntwin.o sun3.md/mfbpolypnt.o sun3.md/mfbpushpxl.o sun3.md/mfbscrclse.o sun3.md/mfbscrinit.o sun3.md/mfbsetsp.o sun3.md/mfbtegblt.o sun3.md/mfbtile.o sun3.md/mfbwindow.o sun3.md/mfbzerarc.o
CLEANOBJS	= sun3.md/maskbits.o sun3.md/mfbbitblt.o sun3.md/mfbbres.o sun3.md/mfbbresd.o sun3.md/mfbbstore.o sun3.md/mfbclip.o sun3.md/mfbcmap.o sun3.md/mfbfillarc.o sun3.md/mfbfillrct.o sun3.md/mfbfillsp.o sun3.md/mfbfont.o sun3.md/mfbgc.o sun3.md/mfbgetsp.o sun3.md/mfbhrzvert.o sun3.md/mfbimage.o sun3.md/mfbimggblt.o sun3.md/mfbline.o sun3.md/mfbmisc.o sun3.md/mfbpixmap.o sun3.md/mfbplygblt.o sun3.md/mfbpntarea.o sun3.md/mfbpntwin.o sun3.md/mfbpolypnt.o sun3.md/mfbpushpxl.o sun3.md/mfbscrclse.o sun3.md/mfbscrinit.o sun3.md/mfbsetsp.o sun3.md/mfbtegblt.o sun3.md/mfbtile.o sun3.md/mfbwindow.o sun3.md/mfbzerarc.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
