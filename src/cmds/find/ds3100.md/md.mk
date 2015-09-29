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
# Tue Jan  5 16:21:44 PST 1993
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= find.c fstype.c parser.c pred.c tree.c util.c version.c alloca.c dirname.c error.c fileblocks.c filemode.c fnmatch.c getopt.c getopt1.c idcache.c listfile.c memset.c modechange.c nextelem.c regex.c savedir.c stpcpy.c strdup.c strftime.c strspn.c strstr.c strtol.c xmalloc.c xstrdup.c
HDRS		= defs.h fnmatch.h getopt.h modechange.h modetype.h pathmax.h regex.h wait.h
MDPUBHDRS	= 
OBJS		= ds3100.md/find.o ds3100.md/fstype.o ds3100.md/parser.o ds3100.md/pred.o ds3100.md/tree.o ds3100.md/util.o ds3100.md/version.o ds3100.md/alloca.o ds3100.md/dirname.o ds3100.md/error.o ds3100.md/fileblocks.o ds3100.md/filemode.o ds3100.md/fnmatch.o ds3100.md/getopt.o ds3100.md/getopt1.o ds3100.md/idcache.o ds3100.md/listfile.o ds3100.md/memset.o ds3100.md/modechange.o ds3100.md/nextelem.o ds3100.md/regex.o ds3100.md/savedir.o ds3100.md/stpcpy.o ds3100.md/strdup.o ds3100.md/strftime.o ds3100.md/strspn.o ds3100.md/strstr.o ds3100.md/strtol.o ds3100.md/xmalloc.o ds3100.md/xstrdup.o
CLEANOBJS	= ds3100.md/find.o ds3100.md/fstype.o ds3100.md/parser.o ds3100.md/pred.o ds3100.md/tree.o ds3100.md/util.o ds3100.md/version.o ds3100.md/alloca.o ds3100.md/dirname.o ds3100.md/error.o ds3100.md/fileblocks.o ds3100.md/filemode.o ds3100.md/fnmatch.o ds3100.md/getopt.o ds3100.md/getopt1.o ds3100.md/idcache.o ds3100.md/listfile.o ds3100.md/memset.o ds3100.md/modechange.o ds3100.md/nextelem.o ds3100.md/regex.o ds3100.md/savedir.o ds3100.md/stpcpy.o ds3100.md/strdup.o ds3100.md/strftime.o ds3100.md/strspn.o ds3100.md/strstr.o ds3100.md/strtol.o ds3100.md/xmalloc.o ds3100.md/xstrdup.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
