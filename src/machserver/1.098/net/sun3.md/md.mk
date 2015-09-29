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
# Tue Jul  2 13:51:10 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/netIE.c sun3.md/netLE.c sun3.md/netIECmd.c sun3.md/netLEMach.c sun3.md/netLERecv.c sun3.md/netConfig.c sun3.md/netIEMem.c sun3.md/netIERecv.c sun3.md/netIESubr.c sun3.md/netIEXmit.c sun3.md/netUltra.c sun3.md/netLEXmit.c netCode.c netRoute.c netEther.c netArp.c
HDRS		= sun3.md/netIEInt.h sun3.md/netLEInt.h sun3.md/netLEMachInt.h kernelTime.h net.h netArp.h netInt.h netInterface.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/netArp.o sun3.md/netCode.o sun3.md/netConfig.o sun3.md/netEther.o sun3.md/netIE.o sun3.md/netIECmd.o sun3.md/netIEMem.o sun3.md/netIERecv.o sun3.md/netIESubr.o sun3.md/netIEXmit.o sun3.md/netLE.o sun3.md/netLEMach.o sun3.md/netLERecv.o sun3.md/netLEXmit.o sun3.md/netRoute.o sun3.md/netUltra.o
CLEANOBJS	= sun3.md/netIE.o sun3.md/netLE.o sun3.md/netIECmd.o sun3.md/netLEMach.o sun3.md/netLERecv.o sun3.md/netConfig.o sun3.md/netIEMem.o sun3.md/netIERecv.o sun3.md/netIESubr.o sun3.md/netIEXmit.o sun3.md/netUltra.o sun3.md/netLEXmit.o sun3.md/netCode.o sun3.md/netRoute.o sun3.md/netEther.o sun3.md/netArp.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
