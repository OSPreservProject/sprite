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
# Fri May 18 15:09:44 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cfb8bit.c cfbbitblt.c cfbbres.c cfbbresd.c cfbbstore.c cfbcmap.c cfbfillarc.c cfbfillrct.c cfbfillsp.c cfbgc.c cfbgetsp.c cfbglblt8.c cfbhrzvert.c cfbimage.c cfbline.c cfbmskbits.c cfbpixmap.c cfbpntwin.c cfbpolypnt.c cfbpush8.c cfbrctstp8.c cfbscrinit.c cfbseg.c cfbsetsp.c cfbteblt8.c cfbtegblt.c cfbtileodd.c cfbwindow.c cfbzerarc.c
HDRS		= cfb.h cfb8bit.h cfbmskbits.h
MDPUBHDRS	= 
OBJS		= sun3.md/cfb8bit.o sun3.md/cfbbitblt.o sun3.md/cfbbres.o sun3.md/cfbbresd.o sun3.md/cfbbstore.o sun3.md/cfbcmap.o sun3.md/cfbfillarc.o sun3.md/cfbfillrct.o sun3.md/cfbfillsp.o sun3.md/cfbgc.o sun3.md/cfbgetsp.o sun3.md/cfbglblt8.o sun3.md/cfbhrzvert.o sun3.md/cfbimage.o sun3.md/cfbline.o sun3.md/cfbmskbits.o sun3.md/cfbpixmap.o sun3.md/cfbpntwin.o sun3.md/cfbpolypnt.o sun3.md/cfbpush8.o sun3.md/cfbrctstp8.o sun3.md/cfbscrinit.o sun3.md/cfbseg.o sun3.md/cfbsetsp.o sun3.md/cfbteblt8.o sun3.md/cfbtegblt.o sun3.md/cfbtileodd.o sun3.md/cfbwindow.o sun3.md/cfbzerarc.o
CLEANOBJS	= sun3.md/cfb8bit.o sun3.md/cfbbitblt.o sun3.md/cfbbres.o sun3.md/cfbbresd.o sun3.md/cfbbstore.o sun3.md/cfbcmap.o sun3.md/cfbfillarc.o sun3.md/cfbfillrct.o sun3.md/cfbfillsp.o sun3.md/cfbgc.o sun3.md/cfbgetsp.o sun3.md/cfbglblt8.o sun3.md/cfbhrzvert.o sun3.md/cfbimage.o sun3.md/cfbline.o sun3.md/cfbmskbits.o sun3.md/cfbpixmap.o sun3.md/cfbpntwin.o sun3.md/cfbpolypnt.o sun3.md/cfbpush8.o sun3.md/cfbrctstp8.o sun3.md/cfbscrinit.o sun3.md/cfbseg.o sun3.md/cfbsetsp.o sun3.md/cfbteblt8.o sun3.md/cfbtegblt.o sun3.md/cfbtileodd.o sun3.md/cfbwindow.o sun3.md/cfbzerarc.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
