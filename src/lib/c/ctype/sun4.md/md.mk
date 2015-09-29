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
# Mon Jun  8 14:22:56 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ctypeBits.c isalnum.c isalpha.c isascii.c isdigit.c iscntrl.c isgraph.c islower.c isprint.c ispunct.c isspace.c isupper.c isxdigit.c tolower.c toupper.c toascii.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/ctypeBits.o sun4.md/isalnum.o sun4.md/isalpha.o sun4.md/isascii.o sun4.md/isdigit.o sun4.md/iscntrl.o sun4.md/isgraph.o sun4.md/islower.o sun4.md/isprint.o sun4.md/ispunct.o sun4.md/isspace.o sun4.md/isupper.o sun4.md/isxdigit.o sun4.md/tolower.o sun4.md/toupper.o sun4.md/toascii.o
CLEANOBJS	= sun4.md/ctypeBits.o sun4.md/isalnum.o sun4.md/isalpha.o sun4.md/isascii.o sun4.md/isdigit.o sun4.md/iscntrl.o sun4.md/isgraph.o sun4.md/islower.o sun4.md/isprint.o sun4.md/ispunct.o sun4.md/isspace.o sun4.md/isupper.o sun4.md/isxdigit.o sun4.md/tolower.o sun4.md/toupper.o sun4.md/toascii.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
