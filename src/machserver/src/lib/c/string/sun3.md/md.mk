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
# Sun Nov 10 21:50:40 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= strdup.c index.c rindex.c strcat.c strchr.c strcmp.c strcpy.c strcspn.c strerror.c strlen.c strncat.c strncmp.c strncpy.c strpbrk.c strrchr.c strsep.c strspn.c strstr.c strtok.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/strdup.o sun3.md/index.o sun3.md/rindex.o sun3.md/strcat.o sun3.md/strchr.o sun3.md/strcmp.o sun3.md/strcpy.o sun3.md/strcspn.o sun3.md/strerror.o sun3.md/strlen.o sun3.md/strncat.o sun3.md/strncmp.o sun3.md/strncpy.o sun3.md/strpbrk.o sun3.md/strrchr.o sun3.md/strsep.o sun3.md/strspn.o sun3.md/strstr.o sun3.md/strtok.o
CLEANOBJS	= sun3.md/strdup.o sun3.md/index.o sun3.md/rindex.o sun3.md/strcat.o sun3.md/strchr.o sun3.md/strcmp.o sun3.md/strcpy.o sun3.md/strcspn.o sun3.md/strerror.o sun3.md/strlen.o sun3.md/strncat.o sun3.md/strncmp.o sun3.md/strncpy.o sun3.md/strpbrk.o sun3.md/strrchr.o sun3.md/strsep.o sun3.md/strspn.o sun3.md/strstr.o sun3.md/strtok.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
