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
# Tue Aug 14 13:13:42 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/getidprom.s sun4.md/idprom.c sun4.md/map.s sun4.md/standalloc.c sun4.md/start.s inet.c main.c tftp.c xxboot.c
HDRS		= sun4.md/bootparam.h sun4.md/buserr.h sun4.md/cpu.addrs.h sun4.md/cpu.map.h sun4.md/diag.h sun4.md/dpy.h sun4.md/enable.h sun4.md/globram.h sun4.md/idprom.h sun4.md/interreg.h sun4.md/saio.h sun4.md/sunmon.h sun4.md/sunromvec.h asyncbuf.h boot.h if.h if_arp.h if_ether.h in.h in_systm.h ip.h memvar.h pixrect.h sainet.h socket.h systypes.h tftp.h types.h udp.h
MDPUBHDRS	= 
OBJS		= sun4.md/getidprom.o sun4.md/idprom.o sun4.md/inet.o sun4.md/map.o sun4.md/start.o sun4.md/standalloc.o sun4.md/main.o sun4.md/tftp.o sun4.md/xxboot.o
CLEANOBJS	= sun4.md/getidprom.o sun4.md/idprom.o sun4.md/map.o sun4.md/standalloc.o sun4.md/start.o sun4.md/inet.o sun4.md/main.o sun4.md/tftp.o sun4.md/xxboot.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
