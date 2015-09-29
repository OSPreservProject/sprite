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
# Tue Jun 16 15:20:13 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/cksum.c acksend.c candidate.c readmsg.c correct.c byteorder.c master.c measure.c networkdelta.c slave.c timed.c
HDRS		= globals.h timed.h
MDPUBHDRS	= 
OBJS		= sun3.md/acksend.o sun3.md/byteorder.o sun3.md/candidate.o sun3.md/cksum.o sun3.md/correct.o sun3.md/master.o sun3.md/measure.o sun3.md/networkdelta.o sun3.md/readmsg.o sun3.md/slave.o sun3.md/timed.o
CLEANOBJS	= sun3.md/cksum.o sun3.md/acksend.o sun3.md/candidate.o sun3.md/readmsg.o sun3.md/correct.o sun3.md/byteorder.o sun3.md/master.o sun3.md/measure.o sun3.md/networkdelta.o sun3.md/slave.o sun3.md/timed.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
