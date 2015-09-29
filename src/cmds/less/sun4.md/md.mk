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
# Sat Dec 21 23:06:36 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= brac.c ch.c charset.c cmdbuf.c command.c decode.c help.c ifile.c input.c linenum.c mark.c opttbl.c os.c output.c position.c prompt.c signal.c edit.c filename.c forwback.c jump.c line.c lsystem.c main.c optfunc.c option.c screen.c search.c tags.c ttyin.c version.c
HDRS		= cmd.h defines.h funcs.h less.h option.h position.h
MDPUBHDRS	= 
OBJS		= sun4.md/brac.o sun4.md/ch.o sun4.md/charset.o sun4.md/cmdbuf.o sun4.md/command.o sun4.md/decode.o sun4.md/help.o sun4.md/ifile.o sun4.md/input.o sun4.md/linenum.o sun4.md/mark.o sun4.md/opttbl.o sun4.md/os.o sun4.md/output.o sun4.md/position.o sun4.md/prompt.o sun4.md/signal.o sun4.md/edit.o sun4.md/filename.o sun4.md/forwback.o sun4.md/jump.o sun4.md/line.o sun4.md/lsystem.o sun4.md/main.o sun4.md/optfunc.o sun4.md/option.o sun4.md/screen.o sun4.md/search.o sun4.md/tags.o sun4.md/ttyin.o sun4.md/version.o
CLEANOBJS	= sun4.md/brac.o sun4.md/ch.o sun4.md/charset.o sun4.md/cmdbuf.o sun4.md/command.o sun4.md/decode.o sun4.md/help.o sun4.md/ifile.o sun4.md/input.o sun4.md/linenum.o sun4.md/mark.o sun4.md/opttbl.o sun4.md/os.o sun4.md/output.o sun4.md/position.o sun4.md/prompt.o sun4.md/signal.o sun4.md/edit.o sun4.md/filename.o sun4.md/forwback.o sun4.md/jump.o sun4.md/line.o sun4.md/lsystem.o sun4.md/main.o sun4.md/optfunc.o sun4.md/option.o sun4.md/screen.o sun4.md/search.o sun4.md/tags.o sun4.md/ttyin.o sun4.md/version.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
