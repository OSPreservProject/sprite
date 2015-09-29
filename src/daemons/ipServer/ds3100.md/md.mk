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
# Tue Jun 16 13:09:05 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= icmp.c ip.c main.c raw.c route.c sockMisc.c sockOps.c stat.c tcpInput.c tcpOutput.c tcpTimer.c tcpTrace.c udp.c tcpSock.c sockBuf.c
HDRS		= icmp.h ip.h ipServer.h raw.h route.h sockInt.h socket.h stat.h tcp.h tcpInt.h tcpTimer.h udp.h version.h
MDPUBHDRS	= 
OBJS		= ds3100.md/sockOps.o ds3100.md/stat.o ds3100.md/tcpInput.o ds3100.md/tcpOutput.o ds3100.md/tcpSock.o ds3100.md/tcpTimer.o ds3100.md/tcpTrace.o ds3100.md/udp.o ds3100.md/icmp.o ds3100.md/ip.o ds3100.md/main.o ds3100.md/raw.o ds3100.md/route.o ds3100.md/sockMisc.o ds3100.md/sockBuf.o
CLEANOBJS	= ds3100.md/icmp.o ds3100.md/ip.o ds3100.md/main.o ds3100.md/raw.o ds3100.md/route.o ds3100.md/sockMisc.o ds3100.md/sockOps.o ds3100.md/stat.o ds3100.md/tcpInput.o ds3100.md/tcpOutput.o ds3100.md/tcpTimer.o ds3100.md/tcpTrace.o ds3100.md/udp.o ds3100.md/tcpSock.o ds3100.md/sockBuf.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
