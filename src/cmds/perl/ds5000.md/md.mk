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
# Thu Nov 14 12:42:52 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= array.c cmd.c cons.c consarg.c doarg.c doio.c dolist.c dump.c eval.c form.c hash.c perl.c regcomp.c regexec.c stab.c str.c toke.c usersub.c util.c perly.y perly.c
HDRS		= ds3100.md/config.h EXTERN.h INTERN.h arg.h array.h cmd.h form.h handy.h hash.h patchlevel.h perl.h perly.h regcomp.h regexp.h spat.h stab.h str.h util.h
MDPUBHDRS	= 
OBJS		= ds3100.md/array.o ds3100.md/cmd.o ds3100.md/cons.o ds3100.md/consarg.o ds3100.md/doarg.o ds3100.md/doio.o ds3100.md/dolist.o ds3100.md/dump.o ds3100.md/eval.o ds3100.md/form.o ds3100.md/hash.o ds3100.md/perl.o ds3100.md/perly.o ds3100.md/regcomp.o ds3100.md/regexec.o ds3100.md/stab.o ds3100.md/str.o ds3100.md/toke.o ds3100.md/usersub.o ds3100.md/util.o
CLEANOBJS	= ds3100.md/array.o ds3100.md/cmd.o ds3100.md/cons.o ds3100.md/consarg.o ds3100.md/doarg.o ds3100.md/doio.o ds3100.md/dolist.o ds3100.md/dump.o ds3100.md/eval.o ds3100.md/form.o ds3100.md/hash.o ds3100.md/perl.o ds3100.md/regcomp.o ds3100.md/regexec.o ds3100.md/stab.o ds3100.md/str.o ds3100.md/toke.o ds3100.md/usersub.o ds3100.md/util.o ds3100.md/perly.o ds3100.md/perly.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
