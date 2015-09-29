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
# Sat Oct 12 19:39:29 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ctypeBits.c isalnum.c isalpha.c isascii.c iscntrl.c isdigit.c isgraph.c islower.c isprint.c ispunct.c isspace.c isupper.c isxdigit.c tolower.c toascii.c toupper.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/ctypeBits.o sun3.md/isalnum.o sun3.md/isalpha.o sun3.md/isascii.o sun3.md/iscntrl.o sun3.md/isdigit.o sun3.md/isgraph.o sun3.md/islower.o sun3.md/isprint.o sun3.md/ispunct.o sun3.md/isspace.o sun3.md/isupper.o sun3.md/isxdigit.o sun3.md/tolower.o sun3.md/toascii.o sun3.md/toupper.o
CLEANOBJS	= sun3.md/ctypeBits.o sun3.md/isalnum.o sun3.md/isalpha.o sun3.md/isascii.o sun3.md/iscntrl.o sun3.md/isdigit.o sun3.md/isgraph.o sun3.md/islower.o sun3.md/isprint.o sun3.md/ispunct.o sun3.md/isspace.o sun3.md/isupper.o sun3.md/isxdigit.o sun3.md/tolower.o sun3.md/toascii.o sun3.md/toupper.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
