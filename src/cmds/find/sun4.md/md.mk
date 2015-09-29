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
# Tue Jan  5 16:21:51 PST 1993
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= find.c fstype.c parser.c pred.c tree.c util.c version.c alloca.c dirname.c error.c fileblocks.c filemode.c fnmatch.c getopt.c getopt1.c idcache.c listfile.c memset.c modechange.c nextelem.c regex.c savedir.c stpcpy.c strdup.c strftime.c strspn.c strstr.c strtol.c xmalloc.c xstrdup.c
HDRS		= defs.h fnmatch.h getopt.h modechange.h modetype.h pathmax.h regex.h wait.h
MDPUBHDRS	= 
OBJS		= sun4.md/find.o sun4.md/fstype.o sun4.md/parser.o sun4.md/pred.o sun4.md/tree.o sun4.md/util.o sun4.md/version.o sun4.md/alloca.o sun4.md/dirname.o sun4.md/error.o sun4.md/fileblocks.o sun4.md/filemode.o sun4.md/fnmatch.o sun4.md/getopt.o sun4.md/getopt1.o sun4.md/idcache.o sun4.md/listfile.o sun4.md/memset.o sun4.md/modechange.o sun4.md/nextelem.o sun4.md/regex.o sun4.md/savedir.o sun4.md/stpcpy.o sun4.md/strdup.o sun4.md/strftime.o sun4.md/strspn.o sun4.md/strstr.o sun4.md/strtol.o sun4.md/xmalloc.o sun4.md/xstrdup.o
CLEANOBJS	= sun4.md/find.o sun4.md/fstype.o sun4.md/parser.o sun4.md/pred.o sun4.md/tree.o sun4.md/util.o sun4.md/version.o sun4.md/alloca.o sun4.md/dirname.o sun4.md/error.o sun4.md/fileblocks.o sun4.md/filemode.o sun4.md/fnmatch.o sun4.md/getopt.o sun4.md/getopt1.o sun4.md/idcache.o sun4.md/listfile.o sun4.md/memset.o sun4.md/modechange.o sun4.md/nextelem.o sun4.md/regex.o sun4.md/savedir.o sun4.md/stpcpy.o sun4.md/strdup.o sun4.md/strftime.o sun4.md/strspn.o sun4.md/strstr.o sun4.md/strtol.o sun4.md/xmalloc.o sun4.md/xstrdup.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
