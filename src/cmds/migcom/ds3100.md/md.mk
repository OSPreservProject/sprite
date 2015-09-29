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
# Tue Oct 29 22:21:01 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= error.c global.c header.c lexxer.l mig.c parser.y routine.c server.c statement.c string.c type.c user.c utils.c parser.c
HDRS		= alloc.h error.h global.h lexxer.h parser.h routine.h statement.h string.h type.h utils.h write.h y.tab.h
MDPUBHDRS	= 
OBJS		= ds3100.md/error.o ds3100.md/global.o ds3100.md/header.o ds3100.md/lexxer.o ds3100.md/mig.o ds3100.md/parser.o ds3100.md/routine.o ds3100.md/server.o ds3100.md/statement.o ds3100.md/string.o ds3100.md/type.o ds3100.md/user.o ds3100.md/utils.o ds3100.md/parser.o
CLEANOBJS	= ds3100.md/error.o ds3100.md/global.o ds3100.md/header.o ds3100.md/lexxer.o ds3100.md/mig.o ds3100.md/parser.o ds3100.md/routine.o ds3100.md/server.o ds3100.md/statement.o ds3100.md/string.o ds3100.md/type.o ds3100.md/user.o ds3100.md/utils.o ds3100.md/parser.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
