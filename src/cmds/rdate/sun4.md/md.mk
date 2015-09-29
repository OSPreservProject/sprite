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
# Fri Jan  1 20:29:05 PST 1993
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= port.c rdate.c sockio.c
HDRS		= rdate.h
MDPUBHDRS	= 
OBJS		= sun4.md/port.o sun4.md/rdate.o sun4.md/sockio.o
CLEANOBJS	= sun4.md/port.o sun4.md/rdate.o sun4.md/sockio.o
INSTFILES	= Makefile
SACREDOBJS	= 
