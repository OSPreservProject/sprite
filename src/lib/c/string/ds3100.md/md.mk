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
# Mon Jun  8 14:35:40 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= index.c rindex.c strcat.c strchr.c strcmp.c strcpy.c strerror.c strlen.c strncat.c strncmp.c strncpy.c strrchr.c strstr.c memcmp.c memcpy.c memset.c strcspn.c strpbrk.c strspn.c strtok.c memchr.c memmove.c strdup.c strsep.c stringArray.c strcasecmp.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/index.o ds3100.md/memchr.o ds3100.md/memcmp.o ds3100.md/memcpy.o ds3100.md/memmove.o ds3100.md/memset.o ds3100.md/rindex.o ds3100.md/strcasecmp.o ds3100.md/strcat.o ds3100.md/strchr.o ds3100.md/strcmp.o ds3100.md/strcpy.o ds3100.md/strcspn.o ds3100.md/strdup.o ds3100.md/strerror.o ds3100.md/stringArray.o ds3100.md/strlen.o ds3100.md/strncat.o ds3100.md/strncmp.o ds3100.md/strncpy.o ds3100.md/strpbrk.o ds3100.md/strrchr.o ds3100.md/strsep.o ds3100.md/strspn.o ds3100.md/strstr.o ds3100.md/strtok.o
CLEANOBJS	= ds3100.md/index.o ds3100.md/rindex.o ds3100.md/strcat.o ds3100.md/strchr.o ds3100.md/strcmp.o ds3100.md/strcpy.o ds3100.md/strerror.o ds3100.md/strlen.o ds3100.md/strncat.o ds3100.md/strncmp.o ds3100.md/strncpy.o ds3100.md/strrchr.o ds3100.md/strstr.o ds3100.md/memcmp.o ds3100.md/memcpy.o ds3100.md/memset.o ds3100.md/strcspn.o ds3100.md/strpbrk.o ds3100.md/strspn.o ds3100.md/strtok.o ds3100.md/memchr.o ds3100.md/memmove.o ds3100.md/strdup.o ds3100.md/strsep.o ds3100.md/stringArray.o ds3100.md/strcasecmp.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
