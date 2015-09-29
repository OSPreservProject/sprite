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
# Tue Jul  2 13:51:21 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/netLE.c sun4c.md/netConfig.c sun4c.md/netLERecv.c sun4c.md/netLEXmit.c sun4c.md/netLEMach.c netCode.c netRoute.c netEther.c netArp.c
HDRS		= sun4c.md/netLEInt.h sun4c.md/netLEMachInt.h kernelTime.h net.h netArp.h netInt.h netInterface.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= sun4c.md/netArp.o sun4c.md/netCode.o sun4c.md/netConfig.o sun4c.md/netEther.o sun4c.md/netLE.o sun4c.md/netLEMach.o sun4c.md/netLERecv.o sun4c.md/netLEXmit.o sun4c.md/netRoute.o
CLEANOBJS	= sun4c.md/netLE.o sun4c.md/netConfig.o sun4c.md/netLERecv.o sun4c.md/netLEXmit.o sun4c.md/netLEMach.o sun4c.md/netCode.o sun4c.md/netRoute.o sun4c.md/netEther.o sun4c.md/netArp.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
