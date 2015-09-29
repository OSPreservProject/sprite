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
# Mon Jun  8 14:35:47 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= index.c rindex.c strcat.c strchr.c strcmp.c strcpy.c strerror.c strlen.c strncat.c strncmp.c strncpy.c strrchr.c strstr.c memcmp.c memcpy.c memset.c strcspn.c strpbrk.c strspn.c strtok.c memchr.c memmove.c strdup.c strsep.c stringArray.c strcasecmp.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/index.o sun3.md/rindex.o sun3.md/strcat.o sun3.md/strchr.o sun3.md/strcmp.o sun3.md/strcpy.o sun3.md/strerror.o sun3.md/strlen.o sun3.md/strncat.o sun3.md/strncmp.o sun3.md/strncpy.o sun3.md/strrchr.o sun3.md/strstr.o sun3.md/memcmp.o sun3.md/memcpy.o sun3.md/memset.o sun3.md/strcspn.o sun3.md/strpbrk.o sun3.md/strspn.o sun3.md/strtok.o sun3.md/memchr.o sun3.md/memmove.o sun3.md/strdup.o sun3.md/strsep.o sun3.md/stringArray.o sun3.md/strcasecmp.o
CLEANOBJS	= sun3.md/index.o sun3.md/rindex.o sun3.md/strcat.o sun3.md/strchr.o sun3.md/strcmp.o sun3.md/strcpy.o sun3.md/strerror.o sun3.md/strlen.o sun3.md/strncat.o sun3.md/strncmp.o sun3.md/strncpy.o sun3.md/strrchr.o sun3.md/strstr.o sun3.md/memcmp.o sun3.md/memcpy.o sun3.md/memset.o sun3.md/strcspn.o sun3.md/strpbrk.o sun3.md/strspn.o sun3.md/strtok.o sun3.md/memchr.o sun3.md/memmove.o sun3.md/strdup.o sun3.md/strsep.o sun3.md/stringArray.o sun3.md/strcasecmp.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
