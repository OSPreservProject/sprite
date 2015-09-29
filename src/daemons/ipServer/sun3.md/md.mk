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
# Tue Jun 16 13:09:17 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= icmp.c ip.c main.c raw.c route.c sockMisc.c sockOps.c stat.c tcpInput.c tcpOutput.c tcpTimer.c tcpTrace.c udp.c tcpSock.c sockBuf.c
HDRS		= icmp.h ip.h ipServer.h raw.h route.h sockInt.h socket.h stat.h tcp.h tcpInt.h tcpTimer.h udp.h version.h
MDPUBHDRS	= 
OBJS		= sun3.md/icmp.o sun3.md/ip.o sun3.md/main.o sun3.md/raw.o sun3.md/route.o sun3.md/sockBuf.o sun3.md/sockMisc.o sun3.md/sockOps.o sun3.md/stat.o sun3.md/tcpInput.o sun3.md/tcpOutput.o sun3.md/tcpSock.o sun3.md/tcpTimer.o sun3.md/tcpTrace.o sun3.md/udp.o
CLEANOBJS	= sun3.md/icmp.o sun3.md/ip.o sun3.md/main.o sun3.md/raw.o sun3.md/route.o sun3.md/sockMisc.o sun3.md/sockOps.o sun3.md/stat.o sun3.md/tcpInput.o sun3.md/tcpOutput.o sun3.md/tcpTimer.o sun3.md/tcpTrace.o sun3.md/udp.o sun3.md/tcpSock.o sun3.md/sockBuf.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
