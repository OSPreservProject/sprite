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
# Fri Mar 13 15:29:49 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= args.c blok.c builtin.c cmd.c ctype.c error.c expand.c fault.c io.c macro.c main.c msg.c name.c print.c service.c setbrk.c stak.c string.c word.c xec.c
HDRS		= brkincr.h ctype.h defs.h dup.h mac.h mode.h name.h stak.h sym.h timeout.h
MDPUBHDRS	= 
OBJS		= ds3100.md/args.o ds3100.md/blok.o ds3100.md/builtin.o ds3100.md/cmd.o ds3100.md/ctype.o ds3100.md/error.o ds3100.md/expand.o ds3100.md/fault.o ds3100.md/io.o ds3100.md/macro.o ds3100.md/main.o ds3100.md/msg.o ds3100.md/name.o ds3100.md/print.o ds3100.md/service.o ds3100.md/setbrk.o ds3100.md/stak.o ds3100.md/string.o ds3100.md/word.o ds3100.md/xec.o
CLEANOBJS	= ds3100.md/args.o ds3100.md/blok.o ds3100.md/builtin.o ds3100.md/cmd.o ds3100.md/ctype.o ds3100.md/error.o ds3100.md/expand.o ds3100.md/fault.o ds3100.md/io.o ds3100.md/macro.o ds3100.md/main.o ds3100.md/msg.o ds3100.md/name.o ds3100.md/print.o ds3100.md/service.o ds3100.md/setbrk.o ds3100.md/stak.o ds3100.md/string.o ds3100.md/word.o ds3100.md/xec.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
