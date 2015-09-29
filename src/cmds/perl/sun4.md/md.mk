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
# Thu Nov 14 12:43:05 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= array.c cmd.c cons.c consarg.c doarg.c doio.c dolist.c dump.c eval.c form.c hash.c perl.c regcomp.c regexec.c stab.c str.c toke.c usersub.c util.c perly.y perly.c
HDRS		= sun4.md/config.h EXTERN.h INTERN.h arg.h array.h cmd.h form.h handy.h hash.h patchlevel.h perl.h perly.h regcomp.h regexp.h spat.h stab.h str.h util.h
MDPUBHDRS	= 
OBJS		= sun4.md/array.o sun4.md/cmd.o sun4.md/cons.o sun4.md/consarg.o sun4.md/doarg.o sun4.md/doio.o sun4.md/dolist.o sun4.md/dump.o sun4.md/eval.o sun4.md/form.o sun4.md/hash.o sun4.md/perl.o sun4.md/perly.o sun4.md/regcomp.o sun4.md/regexec.o sun4.md/stab.o sun4.md/str.o sun4.md/toke.o sun4.md/usersub.o sun4.md/util.o
CLEANOBJS	= sun4.md/array.o sun4.md/cmd.o sun4.md/cons.o sun4.md/consarg.o sun4.md/doarg.o sun4.md/doio.o sun4.md/dolist.o sun4.md/dump.o sun4.md/eval.o sun4.md/form.o sun4.md/hash.o sun4.md/perl.o sun4.md/regcomp.o sun4.md/regexec.o sun4.md/stab.o sun4.md/str.o sun4.md/toke.o sun4.md/usersub.o sun4.md/util.o sun4.md/perly.o sun4.md/perly.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
