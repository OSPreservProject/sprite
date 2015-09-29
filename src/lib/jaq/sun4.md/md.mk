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
# Thu Sep 10 18:47:43 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= admin.c dev.c hash.c indx.c lock.c mem.c option.c queue.c regexp.c sock.c str.c tbuf.c tlog.c ttime.c utils.c
HDRS		= cfuncproto.h jaquith.h option.h regexp.h
MDPUBHDRS	= 
OBJS		= sun4.md/admin.o sun4.md/dev.o sun4.md/hash.o sun4.md/indx.o sun4.md/lock.o sun4.md/mem.o sun4.md/option.o sun4.md/queue.o sun4.md/regexp.o sun4.md/sock.o sun4.md/str.o sun4.md/tbuf.o sun4.md/tlog.o sun4.md/ttime.o sun4.md/utils.o
CLEANOBJS	= sun4.md/admin.o sun4.md/dev.o sun4.md/hash.o sun4.md/indx.o sun4.md/lock.o sun4.md/mem.o sun4.md/option.o sun4.md/queue.o sun4.md/regexp.o sun4.md/sock.o sun4.md/str.o sun4.md/tbuf.o sun4.md/tlog.o sun4.md/ttime.o sun4.md/utils.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
