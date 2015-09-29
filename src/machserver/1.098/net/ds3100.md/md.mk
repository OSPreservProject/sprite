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
# Tue Jul  2 13:50:59 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/netConfig.c ds3100.md/netLE.c ds3100.md/netLERecv.c ds3100.md/netLEXmit.c netCode.c netRoute.c netEther.c netArp.c
HDRS		= ds3100.md/netLEInt.h kernelTime.h net.h netArp.h netInt.h netInterface.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/netArp.o ds3100.md/netCode.o ds3100.md/netConfig.o ds3100.md/netEther.o ds3100.md/netLE.o ds3100.md/netLERecv.o ds3100.md/netLEXmit.o ds3100.md/netRoute.o
CLEANOBJS	= ds3100.md/netConfig.o ds3100.md/netLE.o ds3100.md/netLERecv.o ds3100.md/netLEXmit.o ds3100.md/netCode.o ds3100.md/netRoute.o ds3100.md/netEther.o ds3100.md/netArp.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
