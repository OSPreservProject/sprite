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
# Mon Jun  8 14:35:54 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= index.c rindex.c strcat.c strchr.c strcmp.c strcpy.c strerror.c strlen.c strncat.c strncmp.c strncpy.c strrchr.c strstr.c memcmp.c memcpy.c memset.c strcspn.c strpbrk.c strspn.c strtok.c memchr.c memmove.c strdup.c strsep.c stringArray.c strcasecmp.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/index.o sun4.md/rindex.o sun4.md/strcat.o sun4.md/strchr.o sun4.md/strcmp.o sun4.md/strcpy.o sun4.md/strerror.o sun4.md/strlen.o sun4.md/strncat.o sun4.md/strncmp.o sun4.md/strncpy.o sun4.md/strrchr.o sun4.md/strstr.o sun4.md/memcmp.o sun4.md/memcpy.o sun4.md/memset.o sun4.md/strcspn.o sun4.md/strpbrk.o sun4.md/strspn.o sun4.md/strtok.o sun4.md/memchr.o sun4.md/memmove.o sun4.md/strdup.o sun4.md/strsep.o sun4.md/stringArray.o sun4.md/strcasecmp.o
CLEANOBJS	= sun4.md/index.o sun4.md/rindex.o sun4.md/strcat.o sun4.md/strchr.o sun4.md/strcmp.o sun4.md/strcpy.o sun4.md/strerror.o sun4.md/strlen.o sun4.md/strncat.o sun4.md/strncmp.o sun4.md/strncpy.o sun4.md/strrchr.o sun4.md/strstr.o sun4.md/memcmp.o sun4.md/memcpy.o sun4.md/memset.o sun4.md/strcspn.o sun4.md/strpbrk.o sun4.md/strspn.o sun4.md/strtok.o sun4.md/memchr.o sun4.md/memmove.o sun4.md/strdup.o sun4.md/strsep.o sun4.md/stringArray.o sun4.md/strcasecmp.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
