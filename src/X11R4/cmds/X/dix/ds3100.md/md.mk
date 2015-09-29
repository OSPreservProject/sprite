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
# Fri May 18 15:11:38 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c dixutils.c events.c extension.c gc.c globals.c glyphcurs.c grabs.c initatoms.c main.c property.c resource.c swaprep.c swapreq.c tables.c window.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/atom.o ds3100.md/colormap.o ds3100.md/cursor.o ds3100.md/devices.o ds3100.md/dispatch.o ds3100.md/dixfonts.o ds3100.md/dixutils.o ds3100.md/events.o ds3100.md/extension.o ds3100.md/gc.o ds3100.md/globals.o ds3100.md/glyphcurs.o ds3100.md/grabs.o ds3100.md/initatoms.o ds3100.md/main.o ds3100.md/property.o ds3100.md/resource.o ds3100.md/swaprep.o ds3100.md/swapreq.o ds3100.md/tables.o ds3100.md/window.o
CLEANOBJS	= ds3100.md/atom.o ds3100.md/colormap.o ds3100.md/cursor.o ds3100.md/devices.o ds3100.md/dispatch.o ds3100.md/dixfonts.o ds3100.md/dixutils.o ds3100.md/events.o ds3100.md/extension.o ds3100.md/gc.o ds3100.md/globals.o ds3100.md/glyphcurs.o ds3100.md/grabs.o ds3100.md/initatoms.o ds3100.md/main.o ds3100.md/property.o ds3100.md/resource.o ds3100.md/swaprep.o ds3100.md/swapreq.o ds3100.md/tables.o ds3100.md/window.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
