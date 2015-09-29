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
# Tue Aug 14 13:13:51 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/getidprom.s sun4c.md/idprom.c sun4c.md/map.s sun4c.md/standalloc.c sun4c.md/start.s inet.c main.c tftp.c xxboot.c
HDRS		= sun4c.md/bootparam.h sun4c.md/cpu.addrs.h sun4c.md/cpu.map.h sun4c.md/idprom.h sun4c.md/openprom.h sun4c.md/saio.h sun4c.md/sunmon.h sun4c.md/sunromvec.h asyncbuf.h boot.h if.h if_arp.h if_ether.h in.h in_systm.h ip.h memvar.h pixrect.h sainet.h socket.h systypes.h tftp.h types.h udp.h
MDPUBHDRS	= 
OBJS		= sun4c.md/getidprom.o sun4c.md/idprom.o sun4c.md/map.o sun4c.md/standalloc.o sun4c.md/start.o sun4c.md/inet.o sun4c.md/main.o sun4c.md/tftp.o sun4c.md/xxboot.o
CLEANOBJS	= sun4c.md/getidprom.o sun4c.md/idprom.o sun4c.md/map.o sun4c.md/standalloc.o sun4c.md/start.o sun4c.md/inet.o sun4c.md/main.o sun4c.md/tftp.o sun4c.md/xxboot.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
