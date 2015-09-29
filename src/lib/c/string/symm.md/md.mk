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
# Mon Jun  8 14:36:02 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= index.c rindex.c strcat.c strchr.c strcmp.c strcpy.c strerror.c strlen.c strncat.c strncmp.c strncpy.c strrchr.c strstr.c memcmp.c memcpy.c memset.c strcspn.c strpbrk.c strspn.c strtok.c memchr.c memmove.c strdup.c strsep.c stringArray.c strcasecmp.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/index.o symm.md/rindex.o symm.md/strcat.o symm.md/strchr.o symm.md/strcmp.o symm.md/strcpy.o symm.md/strerror.o symm.md/strlen.o symm.md/strncat.o symm.md/strncmp.o symm.md/strncpy.o symm.md/strrchr.o symm.md/strstr.o symm.md/memcmp.o symm.md/memcpy.o symm.md/memset.o symm.md/strcspn.o symm.md/strpbrk.o symm.md/strspn.o symm.md/strtok.o symm.md/memchr.o symm.md/memmove.o symm.md/strdup.o symm.md/strsep.o symm.md/stringArray.o symm.md/strcasecmp.o
CLEANOBJS	= symm.md/index.o symm.md/rindex.o symm.md/strcat.o symm.md/strchr.o symm.md/strcmp.o symm.md/strcpy.o symm.md/strerror.o symm.md/strlen.o symm.md/strncat.o symm.md/strncmp.o symm.md/strncpy.o symm.md/strrchr.o symm.md/strstr.o symm.md/memcmp.o symm.md/memcpy.o symm.md/memset.o symm.md/strcspn.o symm.md/strpbrk.o symm.md/strspn.o symm.md/strtok.o symm.md/memchr.o symm.md/memmove.o symm.md/strdup.o symm.md/strsep.o symm.md/stringArray.o symm.md/strcasecmp.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile
SACREDOBJS	= 
