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
# Thu Sep 10 18:47:36 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= admin.c dev.c hash.c indx.c lock.c mem.c option.c queue.c regexp.c sock.c str.c tbuf.c tlog.c ttime.c utils.c
HDRS		= cfuncproto.h jaquith.h option.h regexp.h
MDPUBHDRS	= 
OBJS		= sun3.md/admin.o sun3.md/dev.o sun3.md/hash.o sun3.md/indx.o sun3.md/lock.o sun3.md/mem.o sun3.md/option.o sun3.md/queue.o sun3.md/regexp.o sun3.md/sock.o sun3.md/str.o sun3.md/tbuf.o sun3.md/tlog.o sun3.md/ttime.o sun3.md/utils.o
CLEANOBJS	= sun3.md/admin.o sun3.md/dev.o sun3.md/hash.o sun3.md/indx.o sun3.md/lock.o sun3.md/mem.o sun3.md/option.o sun3.md/queue.o sun3.md/regexp.o sun3.md/sock.o sun3.md/str.o sun3.md/tbuf.o sun3.md/tlog.o sun3.md/ttime.o sun3.md/utils.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
