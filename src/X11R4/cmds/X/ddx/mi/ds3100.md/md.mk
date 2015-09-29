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
# Fri May 18 15:05:32 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= miarc.c mibitblt.c mibstore.c micursor.c midash.c midispcur.c miexpose.c mifillarc.c mifillrct.c mifpolycon.c miglblt.c miinitext.c mipointer.c mipoly.c mipolycon.c mipolygen.c mipolypnt.c mipolyrect.c mipolyseg.c mipolytext.c mipolyutil.c mipushpxl.c miregion.c mispans.c misprite.c mivaltree.c miwideline.c miwindow.c mizerarc.c mizerline.c
HDRS		= mi.h mibstore.h mibstorest.h mifillarc.h mifpoly.h mipointer.h mipointrst.h mipoly.h miscanfill.h mispans.h misprite.h mispritest.h mistruct.h miwideline.h mizerarc.h
MDPUBHDRS	= 
OBJS		= ds3100.md/miarc.o ds3100.md/mibitblt.o ds3100.md/mibstore.o ds3100.md/micursor.o ds3100.md/midash.o ds3100.md/midispcur.o ds3100.md/miexpose.o ds3100.md/mifillarc.o ds3100.md/mifillrct.o ds3100.md/mifpolycon.o ds3100.md/miglblt.o ds3100.md/miinitext.o ds3100.md/mipointer.o ds3100.md/mipoly.o ds3100.md/mipolycon.o ds3100.md/mipolygen.o ds3100.md/mipolypnt.o ds3100.md/mipolyrect.o ds3100.md/mipolyseg.o ds3100.md/mipolytext.o ds3100.md/mipolyutil.o ds3100.md/mipushpxl.o ds3100.md/miregion.o ds3100.md/mispans.o ds3100.md/misprite.o ds3100.md/mivaltree.o ds3100.md/miwideline.o ds3100.md/miwindow.o ds3100.md/mizerarc.o ds3100.md/mizerline.o
CLEANOBJS	= ds3100.md/miarc.o ds3100.md/mibitblt.o ds3100.md/mibstore.o ds3100.md/micursor.o ds3100.md/midash.o ds3100.md/midispcur.o ds3100.md/miexpose.o ds3100.md/mifillarc.o ds3100.md/mifillrct.o ds3100.md/mifpolycon.o ds3100.md/miglblt.o ds3100.md/miinitext.o ds3100.md/mipointer.o ds3100.md/mipoly.o ds3100.md/mipolycon.o ds3100.md/mipolygen.o ds3100.md/mipolypnt.o ds3100.md/mipolyrect.o ds3100.md/mipolyseg.o ds3100.md/mipolytext.o ds3100.md/mipolyutil.o ds3100.md/mipushpxl.o ds3100.md/miregion.o ds3100.md/mispans.o ds3100.md/misprite.o ds3100.md/mivaltree.o ds3100.md/miwideline.o ds3100.md/miwindow.o ds3100.md/mizerarc.o ds3100.md/mizerline.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
