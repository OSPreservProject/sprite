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
# Fri May 18 15:05:53 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= miarc.c mibitblt.c mibstore.c micursor.c midash.c midispcur.c miexpose.c mifillarc.c mifillrct.c mifpolycon.c miglblt.c miinitext.c mipointer.c mipoly.c mipolycon.c mipolygen.c mipolypnt.c mipolyrect.c mipolyseg.c mipolytext.c mipolyutil.c mipushpxl.c miregion.c mispans.c misprite.c mivaltree.c miwideline.c miwindow.c mizerarc.c mizerline.c
HDRS		= mi.h mibstore.h mibstorest.h mifillarc.h mifpoly.h mipointer.h mipointrst.h mipoly.h miscanfill.h mispans.h misprite.h mispritest.h mistruct.h miwideline.h mizerarc.h
MDPUBHDRS	= 
OBJS		= sun3.md/miarc.o sun3.md/mibitblt.o sun3.md/mibstore.o sun3.md/micursor.o sun3.md/midash.o sun3.md/midispcur.o sun3.md/miexpose.o sun3.md/mifillarc.o sun3.md/mifillrct.o sun3.md/mifpolycon.o sun3.md/miglblt.o sun3.md/miinitext.o sun3.md/mipointer.o sun3.md/mipoly.o sun3.md/mipolycon.o sun3.md/mipolygen.o sun3.md/mipolypnt.o sun3.md/mipolyrect.o sun3.md/mipolyseg.o sun3.md/mipolytext.o sun3.md/mipolyutil.o sun3.md/mipushpxl.o sun3.md/miregion.o sun3.md/mispans.o sun3.md/misprite.o sun3.md/mivaltree.o sun3.md/miwideline.o sun3.md/miwindow.o sun3.md/mizerarc.o sun3.md/mizerline.o
CLEANOBJS	= sun3.md/miarc.o sun3.md/mibitblt.o sun3.md/mibstore.o sun3.md/micursor.o sun3.md/midash.o sun3.md/midispcur.o sun3.md/miexpose.o sun3.md/mifillarc.o sun3.md/mifillrct.o sun3.md/mifpolycon.o sun3.md/miglblt.o sun3.md/miinitext.o sun3.md/mipointer.o sun3.md/mipoly.o sun3.md/mipolycon.o sun3.md/mipolygen.o sun3.md/mipolypnt.o sun3.md/mipolyrect.o sun3.md/mipolyseg.o sun3.md/mipolytext.o sun3.md/mipolyutil.o sun3.md/mipushpxl.o sun3.md/miregion.o sun3.md/mispans.o sun3.md/misprite.o sun3.md/mivaltree.o sun3.md/miwideline.o sun3.md/miwindow.o sun3.md/mizerarc.o sun3.md/mizerline.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
