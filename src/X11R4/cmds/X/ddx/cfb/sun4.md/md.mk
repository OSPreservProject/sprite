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
# Fri May 18 15:10:03 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cfb8bit.c cfbbitblt.c cfbbres.c cfbbresd.c cfbbstore.c cfbcmap.c cfbfillarc.c cfbfillrct.c cfbfillsp.c cfbgc.c cfbgetsp.c cfbglblt8.c cfbhrzvert.c cfbimage.c cfbline.c cfbmskbits.c cfbpixmap.c cfbpntwin.c cfbpolypnt.c cfbpush8.c cfbrctstp8.c cfbscrinit.c cfbseg.c cfbsetsp.c cfbteblt8.c cfbtegblt.c cfbtileodd.c cfbwindow.c cfbzerarc.c
HDRS		= cfb.h cfb8bit.h cfbmskbits.h
MDPUBHDRS	= 
OBJS		= sun4.md/cfb8bit.o sun4.md/cfbbitblt.o sun4.md/cfbbres.o sun4.md/cfbbresd.o sun4.md/cfbbstore.o sun4.md/cfbcmap.o sun4.md/cfbfillarc.o sun4.md/cfbfillrct.o sun4.md/cfbfillsp.o sun4.md/cfbgc.o sun4.md/cfbgetsp.o sun4.md/cfbglblt8.o sun4.md/cfbhrzvert.o sun4.md/cfbimage.o sun4.md/cfbline.o sun4.md/cfbmskbits.o sun4.md/cfbpixmap.o sun4.md/cfbpntwin.o sun4.md/cfbpolypnt.o sun4.md/cfbpush8.o sun4.md/cfbrctstp8.o sun4.md/cfbscrinit.o sun4.md/cfbseg.o sun4.md/cfbsetsp.o sun4.md/cfbteblt8.o sun4.md/cfbtegblt.o sun4.md/cfbtileodd.o sun4.md/cfbwindow.o sun4.md/cfbzerarc.o
CLEANOBJS	= sun4.md/cfb8bit.o sun4.md/cfbbitblt.o sun4.md/cfbbres.o sun4.md/cfbbresd.o sun4.md/cfbbstore.o sun4.md/cfbcmap.o sun4.md/cfbfillarc.o sun4.md/cfbfillrct.o sun4.md/cfbfillsp.o sun4.md/cfbgc.o sun4.md/cfbgetsp.o sun4.md/cfbglblt8.o sun4.md/cfbhrzvert.o sun4.md/cfbimage.o sun4.md/cfbline.o sun4.md/cfbmskbits.o sun4.md/cfbpixmap.o sun4.md/cfbpntwin.o sun4.md/cfbpolypnt.o sun4.md/cfbpush8.o sun4.md/cfbrctstp8.o sun4.md/cfbscrinit.o sun4.md/cfbseg.o sun4.md/cfbsetsp.o sun4.md/cfbteblt8.o sun4.md/cfbtegblt.o sun4.md/cfbtileodd.o sun4.md/cfbwindow.o sun4.md/cfbzerarc.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
