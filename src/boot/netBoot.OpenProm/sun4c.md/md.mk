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
# Mon Aug 12 15:27:41 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/start.s sun4c.md/idprom.c sun4c.md/getidprom.c sun4c.md/map.s inet.c main.c tftp.c boot.c proto.c
HDRS		= sun4c.md/idprom.h boot.h proto.h sainet.h sunmon.h sunromvec.h
MDPUBHDRS	= 
OBJS		= sun4c.md/start.o sun4c.md/idprom.o sun4c.md/getidprom.o sun4c.md/map.o sun4c.md/inet.o sun4c.md/main.o sun4c.md/tftp.o sun4c.md/boot.o sun4c.md/proto.o
CLEANOBJS	= sun4c.md/start.o sun4c.md/idprom.o sun4c.md/getidprom.o sun4c.md/map.o sun4c.md/inet.o sun4c.md/main.o sun4c.md/tftp.o sun4c.md/boot.o sun4c.md/proto.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
