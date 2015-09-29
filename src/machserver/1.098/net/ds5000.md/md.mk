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
# Tue Jul  2 13:51:04 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/netConfig.c ds5000.md/netLE.c ds5000.md/netLEMach.c ds5000.md/netLERecv.c ds5000.md/netLEXmit.c netCode.c netRoute.c netEther.c netArp.c
HDRS		= ds5000.md/netLEInt.h ds5000.md/netLEMachInt.h kernelTime.h net.h netArp.h netInt.h netInterface.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= ds5000.md/netArp.o ds5000.md/netCode.o ds5000.md/netConfig.o ds5000.md/netEther.o ds5000.md/netLE.o ds5000.md/netLEMach.o ds5000.md/netLERecv.o ds5000.md/netLEXmit.o ds5000.md/netRoute.o
CLEANOBJS	= ds5000.md/netConfig.o ds5000.md/netLE.o ds5000.md/netLEMach.o ds5000.md/netLERecv.o ds5000.md/netLEXmit.o ds5000.md/netCode.o ds5000.md/netRoute.o ds5000.md/netEther.o ds5000.md/netArp.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
