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
# Fri May 18 15:07:13 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= maskbits.c mfbbitblt.c mfbbres.c mfbbresd.c mfbbstore.c mfbclip.c mfbcmap.c mfbfillarc.c mfbfillrct.c mfbfillsp.c mfbfont.c mfbgc.c mfbgetsp.c mfbhrzvert.c mfbimage.c mfbimggblt.c mfbline.c mfbmisc.c mfbpixmap.c mfbplygblt.c mfbpntarea.c mfbpntwin.c mfbpolypnt.c mfbpushpxl.c mfbscrclse.c mfbscrinit.c mfbsetsp.c mfbtegblt.c mfbtile.c mfbwindow.c mfbzerarc.c
HDRS		= fastblt.h maskbits.h mfb.h
MDPUBHDRS	= 
OBJS		= ds3100.md/maskbits.o ds3100.md/mfbbitblt.o ds3100.md/mfbbres.o ds3100.md/mfbbresd.o ds3100.md/mfbbstore.o ds3100.md/mfbclip.o ds3100.md/mfbcmap.o ds3100.md/mfbfillarc.o ds3100.md/mfbfillrct.o ds3100.md/mfbfillsp.o ds3100.md/mfbfont.o ds3100.md/mfbgc.o ds3100.md/mfbgetsp.o ds3100.md/mfbhrzvert.o ds3100.md/mfbimage.o ds3100.md/mfbimggblt.o ds3100.md/mfbline.o ds3100.md/mfbmisc.o ds3100.md/mfbpixmap.o ds3100.md/mfbplygblt.o ds3100.md/mfbpntarea.o ds3100.md/mfbpntwin.o ds3100.md/mfbpolypnt.o ds3100.md/mfbpushpxl.o ds3100.md/mfbscrclse.o ds3100.md/mfbscrinit.o ds3100.md/mfbsetsp.o ds3100.md/mfbtegblt.o ds3100.md/mfbtile.o ds3100.md/mfbwindow.o ds3100.md/mfbzerarc.o
CLEANOBJS	= ds3100.md/maskbits.o ds3100.md/mfbbitblt.o ds3100.md/mfbbres.o ds3100.md/mfbbresd.o ds3100.md/mfbbstore.o ds3100.md/mfbclip.o ds3100.md/mfbcmap.o ds3100.md/mfbfillarc.o ds3100.md/mfbfillrct.o ds3100.md/mfbfillsp.o ds3100.md/mfbfont.o ds3100.md/mfbgc.o ds3100.md/mfbgetsp.o ds3100.md/mfbhrzvert.o ds3100.md/mfbimage.o ds3100.md/mfbimggblt.o ds3100.md/mfbline.o ds3100.md/mfbmisc.o ds3100.md/mfbpixmap.o ds3100.md/mfbplygblt.o ds3100.md/mfbpntarea.o ds3100.md/mfbpntwin.o ds3100.md/mfbpolypnt.o ds3100.md/mfbpushpxl.o ds3100.md/mfbscrclse.o ds3100.md/mfbscrinit.o ds3100.md/mfbsetsp.o ds3100.md/mfbtegblt.o ds3100.md/mfbtile.o ds3100.md/mfbwindow.o ds3100.md/mfbzerarc.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
