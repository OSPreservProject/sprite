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
# Mon Jun  8 14:23:02 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ctypeBits.c isalnum.c isalpha.c isascii.c isdigit.c iscntrl.c isgraph.c islower.c isprint.c ispunct.c isspace.c isupper.c isxdigit.c tolower.c toupper.c toascii.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/ctypeBits.o symm.md/isalnum.o symm.md/isalpha.o symm.md/isascii.o symm.md/isdigit.o symm.md/iscntrl.o symm.md/isgraph.o symm.md/islower.o symm.md/isprint.o symm.md/ispunct.o symm.md/isspace.o symm.md/isupper.o symm.md/isxdigit.o symm.md/tolower.o symm.md/toupper.o symm.md/toascii.o
CLEANOBJS	= symm.md/ctypeBits.o symm.md/isalnum.o symm.md/isalpha.o symm.md/isascii.o symm.md/isdigit.o symm.md/iscntrl.o symm.md/isgraph.o symm.md/islower.o symm.md/isprint.o symm.md/ispunct.o symm.md/isspace.o symm.md/isupper.o symm.md/isxdigit.o symm.md/tolower.o symm.md/toupper.o symm.md/toascii.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile
SACREDOBJS	= 
