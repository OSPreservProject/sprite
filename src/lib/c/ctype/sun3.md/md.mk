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
# Mon Jun  8 14:22:48 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ctypeBits.c isalnum.c isalpha.c isascii.c isdigit.c iscntrl.c isgraph.c islower.c isprint.c ispunct.c isspace.c isupper.c isxdigit.c tolower.c toupper.c toascii.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/ctypeBits.o sun3.md/isalnum.o sun3.md/isalpha.o sun3.md/isascii.o sun3.md/isdigit.o sun3.md/iscntrl.o sun3.md/isgraph.o sun3.md/islower.o sun3.md/isprint.o sun3.md/ispunct.o sun3.md/isspace.o sun3.md/isupper.o sun3.md/isxdigit.o sun3.md/tolower.o sun3.md/toupper.o sun3.md/toascii.o
CLEANOBJS	= sun3.md/ctypeBits.o sun3.md/isalnum.o sun3.md/isalpha.o sun3.md/isascii.o sun3.md/isdigit.o sun3.md/iscntrl.o sun3.md/isgraph.o sun3.md/islower.o sun3.md/isprint.o sun3.md/ispunct.o sun3.md/isspace.o sun3.md/isupper.o sun3.md/isxdigit.o sun3.md/tolower.o sun3.md/toupper.o sun3.md/toascii.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
