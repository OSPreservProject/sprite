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
# Mon Oct 26 17:40:15 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/rawvmmach.c rawrpc.c rawfs.c rawproc.c rawrecov.c rawstat.c rawvm.c rawmig.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/rawfs.o sun4.md/rawmig.o sun4.md/rawproc.o sun4.md/rawrecov.o sun4.md/rawrpc.o sun4.md/rawstat.o sun4.md/rawvm.o sun4.md/rawvmmach.o
CLEANOBJS	= sun4.md/rawvmmach.o sun4.md/rawrpc.o sun4.md/rawfs.o sun4.md/rawproc.o sun4.md/rawrecov.o sun4.md/rawstat.o sun4.md/rawvm.o sun4.md/rawmig.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
