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
# Fri May 18 15:12:08 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c dixutils.c events.c extension.c gc.c globals.c glyphcurs.c grabs.c initatoms.c main.c property.c resource.c swaprep.c swapreq.c tables.c window.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/atom.o sun4.md/colormap.o sun4.md/cursor.o sun4.md/devices.o sun4.md/dispatch.o sun4.md/dixfonts.o sun4.md/dixutils.o sun4.md/events.o sun4.md/extension.o sun4.md/gc.o sun4.md/globals.o sun4.md/glyphcurs.o sun4.md/grabs.o sun4.md/initatoms.o sun4.md/main.o sun4.md/property.o sun4.md/resource.o sun4.md/swaprep.o sun4.md/swapreq.o sun4.md/tables.o sun4.md/window.o
CLEANOBJS	= sun4.md/atom.o sun4.md/colormap.o sun4.md/cursor.o sun4.md/devices.o sun4.md/dispatch.o sun4.md/dixfonts.o sun4.md/dixutils.o sun4.md/events.o sun4.md/extension.o sun4.md/gc.o sun4.md/globals.o sun4.md/glyphcurs.o sun4.md/grabs.o sun4.md/initatoms.o sun4.md/main.o sun4.md/property.o sun4.md/resource.o sun4.md/swaprep.o sun4.md/swapreq.o sun4.md/tables.o sun4.md/window.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
