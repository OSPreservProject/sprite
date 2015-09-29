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
# Fri May 18 15:09:26 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cfb8bit.c cfbbitblt.c cfbbres.c cfbbresd.c cfbbstore.c cfbcmap.c cfbfillarc.c cfbfillrct.c cfbfillsp.c cfbgc.c cfbgetsp.c cfbglblt8.c cfbhrzvert.c cfbimage.c cfbline.c cfbmskbits.c cfbpixmap.c cfbpntwin.c cfbpolypnt.c cfbpush8.c cfbrctstp8.c cfbscrinit.c cfbseg.c cfbsetsp.c cfbteblt8.c cfbtegblt.c cfbtileodd.c cfbwindow.c cfbzerarc.c
HDRS		= cfb.h cfb8bit.h cfbmskbits.h
MDPUBHDRS	= 
OBJS		= ds3100.md/cfb8bit.o ds3100.md/cfbbitblt.o ds3100.md/cfbbres.o ds3100.md/cfbbresd.o ds3100.md/cfbbstore.o ds3100.md/cfbcmap.o ds3100.md/cfbfillarc.o ds3100.md/cfbfillrct.o ds3100.md/cfbfillsp.o ds3100.md/cfbgc.o ds3100.md/cfbgetsp.o ds3100.md/cfbglblt8.o ds3100.md/cfbhrzvert.o ds3100.md/cfbimage.o ds3100.md/cfbline.o ds3100.md/cfbmskbits.o ds3100.md/cfbpixmap.o ds3100.md/cfbpntwin.o ds3100.md/cfbpolypnt.o ds3100.md/cfbpush8.o ds3100.md/cfbrctstp8.o ds3100.md/cfbscrinit.o ds3100.md/cfbseg.o ds3100.md/cfbsetsp.o ds3100.md/cfbteblt8.o ds3100.md/cfbtegblt.o ds3100.md/cfbtileodd.o ds3100.md/cfbwindow.o ds3100.md/cfbzerarc.o
CLEANOBJS	= ds3100.md/cfb8bit.o ds3100.md/cfbbitblt.o ds3100.md/cfbbres.o ds3100.md/cfbbresd.o ds3100.md/cfbbstore.o ds3100.md/cfbcmap.o ds3100.md/cfbfillarc.o ds3100.md/cfbfillrct.o ds3100.md/cfbfillsp.o ds3100.md/cfbgc.o ds3100.md/cfbgetsp.o ds3100.md/cfbglblt8.o ds3100.md/cfbhrzvert.o ds3100.md/cfbimage.o ds3100.md/cfbline.o ds3100.md/cfbmskbits.o ds3100.md/cfbpixmap.o ds3100.md/cfbpntwin.o ds3100.md/cfbpolypnt.o ds3100.md/cfbpush8.o ds3100.md/cfbrctstp8.o ds3100.md/cfbscrinit.o ds3100.md/cfbseg.o ds3100.md/cfbsetsp.o ds3100.md/cfbteblt8.o ds3100.md/cfbtegblt.o ds3100.md/cfbtileodd.o ds3100.md/cfbwindow.o ds3100.md/cfbzerarc.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
