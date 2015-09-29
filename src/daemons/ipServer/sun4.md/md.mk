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
# Tue Jun 16 13:09:28 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= icmp.c ip.c main.c raw.c route.c sockMisc.c sockOps.c stat.c tcpInput.c tcpOutput.c tcpTimer.c tcpTrace.c udp.c tcpSock.c sockBuf.c
HDRS		= icmp.h ip.h ipServer.h raw.h route.h sockInt.h socket.h stat.h tcp.h tcpInt.h tcpTimer.h udp.h version.h
MDPUBHDRS	= 
OBJS		= sun4.md/icmp.o sun4.md/ip.o sun4.md/main.o sun4.md/raw.o sun4.md/route.o sun4.md/sockBuf.o sun4.md/sockMisc.o sun4.md/sockOps.o sun4.md/stat.o sun4.md/tcpInput.o sun4.md/tcpOutput.o sun4.md/tcpSock.o sun4.md/tcpTimer.o sun4.md/tcpTrace.o sun4.md/udp.o
CLEANOBJS	= sun4.md/icmp.o sun4.md/ip.o sun4.md/main.o sun4.md/raw.o sun4.md/route.o sun4.md/sockMisc.o sun4.md/sockOps.o sun4.md/stat.o sun4.md/tcpInput.o sun4.md/tcpOutput.o sun4.md/tcpTimer.o sun4.md/tcpTrace.o sun4.md/udp.o sun4.md/tcpSock.o sun4.md/sockBuf.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
