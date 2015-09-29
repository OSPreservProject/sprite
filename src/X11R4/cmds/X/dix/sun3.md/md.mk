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
# Fri May 18 15:11:54 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c dixutils.c events.c extension.c gc.c globals.c glyphcurs.c grabs.c initatoms.c main.c property.c resource.c swaprep.c swapreq.c tables.c window.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/atom.o sun3.md/colormap.o sun3.md/cursor.o sun3.md/devices.o sun3.md/dispatch.o sun3.md/dixfonts.o sun3.md/dixutils.o sun3.md/events.o sun3.md/extension.o sun3.md/gc.o sun3.md/globals.o sun3.md/glyphcurs.o sun3.md/grabs.o sun3.md/initatoms.o sun3.md/main.o sun3.md/property.o sun3.md/resource.o sun3.md/swaprep.o sun3.md/swapreq.o sun3.md/tables.o sun3.md/window.o
CLEANOBJS	= sun3.md/atom.o sun3.md/colormap.o sun3.md/cursor.o sun3.md/devices.o sun3.md/dispatch.o sun3.md/dixfonts.o sun3.md/dixutils.o sun3.md/events.o sun3.md/extension.o sun3.md/gc.o sun3.md/globals.o sun3.md/glyphcurs.o sun3.md/grabs.o sun3.md/initatoms.o sun3.md/main.o sun3.md/property.o sun3.md/resource.o sun3.md/swaprep.o sun3.md/swapreq.o sun3.md/tables.o sun3.md/window.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
