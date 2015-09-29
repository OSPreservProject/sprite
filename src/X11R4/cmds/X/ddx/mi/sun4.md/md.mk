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
# Fri May 18 15:06:11 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= miarc.c mibitblt.c mibstore.c micursor.c midash.c midispcur.c miexpose.c mifillarc.c mifillrct.c mifpolycon.c miglblt.c miinitext.c mipointer.c mipoly.c mipolycon.c mipolygen.c mipolypnt.c mipolyrect.c mipolyseg.c mipolytext.c mipolyutil.c mipushpxl.c miregion.c mispans.c misprite.c mivaltree.c miwideline.c miwindow.c mizerarc.c mizerline.c
HDRS		= mi.h mibstore.h mibstorest.h mifillarc.h mifpoly.h mipointer.h mipointrst.h mipoly.h miscanfill.h mispans.h misprite.h mispritest.h mistruct.h miwideline.h mizerarc.h
MDPUBHDRS	= 
OBJS		= sun4.md/miarc.o sun4.md/mibitblt.o sun4.md/mibstore.o sun4.md/micursor.o sun4.md/midash.o sun4.md/midispcur.o sun4.md/miexpose.o sun4.md/mifillarc.o sun4.md/mifillrct.o sun4.md/mifpolycon.o sun4.md/miglblt.o sun4.md/miinitext.o sun4.md/mipointer.o sun4.md/mipoly.o sun4.md/mipolycon.o sun4.md/mipolygen.o sun4.md/mipolypnt.o sun4.md/mipolyrect.o sun4.md/mipolyseg.o sun4.md/mipolytext.o sun4.md/mipolyutil.o sun4.md/mipushpxl.o sun4.md/miregion.o sun4.md/mispans.o sun4.md/misprite.o sun4.md/mivaltree.o sun4.md/miwideline.o sun4.md/miwindow.o sun4.md/mizerarc.o sun4.md/mizerline.o
CLEANOBJS	= sun4.md/miarc.o sun4.md/mibitblt.o sun4.md/mibstore.o sun4.md/micursor.o sun4.md/midash.o sun4.md/midispcur.o sun4.md/miexpose.o sun4.md/mifillarc.o sun4.md/mifillrct.o sun4.md/mifpolycon.o sun4.md/miglblt.o sun4.md/miinitext.o sun4.md/mipointer.o sun4.md/mipoly.o sun4.md/mipolycon.o sun4.md/mipolygen.o sun4.md/mipolypnt.o sun4.md/mipolyrect.o sun4.md/mipolyseg.o sun4.md/mipolytext.o sun4.md/mipolyutil.o sun4.md/mipushpxl.o sun4.md/miregion.o sun4.md/mispans.o sun4.md/misprite.o sun4.md/mivaltree.o sun4.md/miwideline.o sun4.md/miwindow.o sun4.md/mizerarc.o sun4.md/mizerline.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
