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
# Thu Dec 17 16:39:55 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c edit.c fio.c getname.c head.c lex.c list.c main.c names.c popen.c quit.c send.c strings.c temp.c tty.c vars.c v7.local.c version.c
HDRS		= def.h glob.h local.h rcv.h v7.local.h
MDPUBHDRS	= 
OBJS		= sun3.md/aux.o sun3.md/cmd1.o sun3.md/cmd2.o sun3.md/cmd3.o sun3.md/cmdtab.o sun3.md/collect.o sun3.md/edit.o sun3.md/fio.o sun3.md/getname.o sun3.md/head.o sun3.md/lex.o sun3.md/list.o sun3.md/main.o sun3.md/names.o sun3.md/popen.o sun3.md/quit.o sun3.md/send.o sun3.md/strings.o sun3.md/temp.o sun3.md/tty.o sun3.md/vars.o sun3.md/v7.local.o sun3.md/version.o
CLEANOBJS	= sun3.md/aux.o sun3.md/cmd1.o sun3.md/cmd2.o sun3.md/cmd3.o sun3.md/cmdtab.o sun3.md/collect.o sun3.md/edit.o sun3.md/fio.o sun3.md/getname.o sun3.md/head.o sun3.md/lex.o sun3.md/list.o sun3.md/main.o sun3.md/names.o sun3.md/popen.o sun3.md/quit.o sun3.md/send.o sun3.md/strings.o sun3.md/temp.o sun3.md/tty.o sun3.md/vars.o sun3.md/v7.local.o sun3.md/version.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
