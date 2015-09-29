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
# Mon Dec 14 17:36:55 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= memmove.c common.c deleteuser.c
HDRS		= common.h
MDPUBHDRS	= 
OBJS		= sun4.md/common.o sun4.md/deleteuser.o sun4.md/memmove.o
CLEANOBJS	= sun4.md/memmove.o sun4.md/common.o sun4.md/deleteuser.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
