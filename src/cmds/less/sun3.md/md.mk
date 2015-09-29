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
# Sat Dec 21 23:06:29 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= brac.c ch.c charset.c cmdbuf.c command.c decode.c help.c ifile.c input.c linenum.c mark.c opttbl.c os.c output.c position.c prompt.c signal.c edit.c filename.c forwback.c jump.c line.c lsystem.c main.c optfunc.c option.c screen.c search.c tags.c ttyin.c version.c
HDRS		= cmd.h defines.h funcs.h less.h option.h position.h
MDPUBHDRS	= 
OBJS		= sun3.md/brac.o sun3.md/ch.o sun3.md/charset.o sun3.md/cmdbuf.o sun3.md/command.o sun3.md/decode.o sun3.md/help.o sun3.md/ifile.o sun3.md/input.o sun3.md/linenum.o sun3.md/mark.o sun3.md/opttbl.o sun3.md/os.o sun3.md/output.o sun3.md/position.o sun3.md/prompt.o sun3.md/signal.o sun3.md/edit.o sun3.md/filename.o sun3.md/forwback.o sun3.md/jump.o sun3.md/line.o sun3.md/lsystem.o sun3.md/main.o sun3.md/optfunc.o sun3.md/option.o sun3.md/screen.o sun3.md/search.o sun3.md/tags.o sun3.md/ttyin.o sun3.md/version.o
CLEANOBJS	= sun3.md/brac.o sun3.md/ch.o sun3.md/charset.o sun3.md/cmdbuf.o sun3.md/command.o sun3.md/decode.o sun3.md/help.o sun3.md/ifile.o sun3.md/input.o sun3.md/linenum.o sun3.md/mark.o sun3.md/opttbl.o sun3.md/os.o sun3.md/output.o sun3.md/position.o sun3.md/prompt.o sun3.md/signal.o sun3.md/edit.o sun3.md/filename.o sun3.md/forwback.o sun3.md/jump.o sun3.md/line.o sun3.md/lsystem.o sun3.md/main.o sun3.md/optfunc.o sun3.md/option.o sun3.md/screen.o sun3.md/search.o sun3.md/tags.o sun3.md/ttyin.o sun3.md/version.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
