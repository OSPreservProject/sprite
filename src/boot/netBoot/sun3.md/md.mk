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
# Tue Aug 14 13:13:33 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/getidprom.s sun3.md/idprom.c sun3.md/map.s sun3.md/standalloc.c sun3.md/start.s inet.c main.c tftp.c xxboot.c
HDRS		= sun3.md/bootparam.h sun3.md/buserr.h sun3.md/cpu.addrs.h sun3.md/cpu.map.h sun3.md/diag.h sun3.md/dpy.h sun3.md/enable.h sun3.md/globram.h sun3.md/idprom.h sun3.md/interreg.h sun3.md/saio.h sun3.md/sunmon.h sun3.md/sunromvec.h asyncbuf.h boot.h if.h if_arp.h if_ether.h in.h in_systm.h ip.h memvar.h pixrect.h sainet.h socket.h systypes.h tftp.h types.h udp.h
MDPUBHDRS	= 
OBJS		= sun3.md/getidprom.o sun3.md/idprom.o sun3.md/map.o sun3.md/standalloc.o sun3.md/start.o sun3.md/inet.o sun3.md/main.o sun3.md/tftp.o sun3.md/xxboot.o
CLEANOBJS	= sun3.md/getidprom.o sun3.md/idprom.o sun3.md/map.o sun3.md/standalloc.o sun3.md/start.o sun3.md/inet.o sun3.md/main.o sun3.md/tftp.o sun3.md/xxboot.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
