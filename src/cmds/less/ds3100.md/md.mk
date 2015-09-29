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
# Sat Dec 21 23:06:22 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= brac.c ch.c charset.c cmdbuf.c command.c decode.c help.c ifile.c input.c linenum.c mark.c opttbl.c os.c output.c position.c prompt.c signal.c edit.c filename.c forwback.c jump.c line.c lsystem.c main.c optfunc.c option.c screen.c search.c tags.c ttyin.c version.c
HDRS		= cmd.h defines.h funcs.h less.h option.h position.h
MDPUBHDRS	= 
OBJS		= ds3100.md/brac.o ds3100.md/ch.o ds3100.md/charset.o ds3100.md/cmdbuf.o ds3100.md/command.o ds3100.md/decode.o ds3100.md/help.o ds3100.md/ifile.o ds3100.md/input.o ds3100.md/linenum.o ds3100.md/mark.o ds3100.md/opttbl.o ds3100.md/os.o ds3100.md/output.o ds3100.md/position.o ds3100.md/prompt.o ds3100.md/signal.o ds3100.md/edit.o ds3100.md/filename.o ds3100.md/forwback.o ds3100.md/jump.o ds3100.md/line.o ds3100.md/lsystem.o ds3100.md/main.o ds3100.md/optfunc.o ds3100.md/option.o ds3100.md/screen.o ds3100.md/search.o ds3100.md/tags.o ds3100.md/ttyin.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/brac.o ds3100.md/ch.o ds3100.md/charset.o ds3100.md/cmdbuf.o ds3100.md/command.o ds3100.md/decode.o ds3100.md/help.o ds3100.md/ifile.o ds3100.md/input.o ds3100.md/linenum.o ds3100.md/mark.o ds3100.md/opttbl.o ds3100.md/os.o ds3100.md/output.o ds3100.md/position.o ds3100.md/prompt.o ds3100.md/signal.o ds3100.md/edit.o ds3100.md/filename.o ds3100.md/forwback.o ds3100.md/jump.o ds3100.md/line.o ds3100.md/lsystem.o ds3100.md/main.o ds3100.md/optfunc.o ds3100.md/option.o ds3100.md/screen.o ds3100.md/search.o ds3100.md/tags.o ds3100.md/ttyin.o ds3100.md/version.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
