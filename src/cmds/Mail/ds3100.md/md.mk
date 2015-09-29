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
# Thu Dec 17 16:39:49 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c edit.c fio.c getname.c head.c lex.c list.c main.c names.c popen.c quit.c send.c strings.c temp.c tty.c vars.c v7.local.c version.c
HDRS		= def.h glob.h local.h rcv.h v7.local.h
MDPUBHDRS	= 
OBJS		= ds3100.md/aux.o ds3100.md/cmd1.o ds3100.md/cmd2.o ds3100.md/cmd3.o ds3100.md/cmdtab.o ds3100.md/collect.o ds3100.md/edit.o ds3100.md/fio.o ds3100.md/getname.o ds3100.md/head.o ds3100.md/lex.o ds3100.md/list.o ds3100.md/main.o ds3100.md/names.o ds3100.md/popen.o ds3100.md/quit.o ds3100.md/send.o ds3100.md/strings.o ds3100.md/temp.o ds3100.md/tty.o ds3100.md/v7.local.o ds3100.md/vars.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/aux.o ds3100.md/cmd1.o ds3100.md/cmd2.o ds3100.md/cmd3.o ds3100.md/cmdtab.o ds3100.md/collect.o ds3100.md/edit.o ds3100.md/fio.o ds3100.md/getname.o ds3100.md/head.o ds3100.md/lex.o ds3100.md/list.o ds3100.md/main.o ds3100.md/names.o ds3100.md/popen.o ds3100.md/quit.o ds3100.md/send.o ds3100.md/strings.o ds3100.md/temp.o ds3100.md/tty.o ds3100.md/vars.o ds3100.md/v7.local.o ds3100.md/version.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
