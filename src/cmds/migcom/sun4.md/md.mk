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
# Thu May 23 22:30:39 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= error.c global.c header.c lexxer.l mig.c parser.y routine.c server.c statement.c string.c type.c user.c utils.c parser.c
HDRS		= alloc.h error.h global.h lexxer.h parser.h routine.h statement.h string.h type.h utils.h write.h y.tab.h
MDPUBHDRS	= 
OBJS		= sun4.md/error.o sun4.md/global.o sun4.md/header.o sun4.md/lexxer.o sun4.md/mig.o sun4.md/parser.o sun4.md/routine.o sun4.md/server.o sun4.md/statement.o sun4.md/string.o sun4.md/type.o sun4.md/user.o sun4.md/utils.o
CLEANOBJS	= sun4.md/error.o sun4.md/global.o sun4.md/header.o sun4.md/lexxer.o sun4.md/mig.o sun4.md/parser.o sun4.md/routine.o sun4.md/server.o sun4.md/statement.o sun4.md/string.o sun4.md/type.o sun4.md/user.o sun4.md/utils.o sun4.md/parser.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
