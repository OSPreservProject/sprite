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
# Thu Sep 10 18:47:30 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= admin.c dev.c hash.c indx.c lock.c mem.c option.c queue.c regexp.c sock.c str.c tbuf.c tlog.c ttime.c utils.c
HDRS		= cfuncproto.h jaquith.h option.h regexp.h
MDPUBHDRS	= 
OBJS		= ds3100.md/admin.o ds3100.md/dev.o ds3100.md/hash.o ds3100.md/indx.o ds3100.md/lock.o ds3100.md/mem.o ds3100.md/option.o ds3100.md/queue.o ds3100.md/regexp.o ds3100.md/sock.o ds3100.md/str.o ds3100.md/tbuf.o ds3100.md/tlog.o ds3100.md/ttime.o ds3100.md/utils.o
CLEANOBJS	= ds3100.md/admin.o ds3100.md/dev.o ds3100.md/hash.o ds3100.md/indx.o ds3100.md/lock.o ds3100.md/mem.o ds3100.md/option.o ds3100.md/queue.o ds3100.md/regexp.o ds3100.md/sock.o ds3100.md/str.o ds3100.md/tbuf.o ds3100.md/tlog.o ds3100.md/ttime.o ds3100.md/utils.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
