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
# Tue Jul  2 13:51:16 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/netIE.c sun4.md/netIECmd.c sun4.md/netConfig.c sun4.md/netIEMem.c sun4.md/netIERecv.c sun4.md/netIESubr.c sun4.md/netIEXmit.c sun4.md/netUltra.c netCode.c netRoute.c netEther.c netArp.c
HDRS		= sun4.md/netIEInt.h kernelTime.h net.h netArp.h netInt.h netInterface.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/netArp.o sun4.md/netCode.o sun4.md/netConfig.o sun4.md/netEther.o sun4.md/netIE.o sun4.md/netIECmd.o sun4.md/netIEMem.o sun4.md/netIERecv.o sun4.md/netIESubr.o sun4.md/netIEXmit.o sun4.md/netRoute.o sun4.md/netUltra.o
CLEANOBJS	= sun4.md/netIE.o sun4.md/netIECmd.o sun4.md/netConfig.o sun4.md/netIEMem.o sun4.md/netIERecv.o sun4.md/netIESubr.o sun4.md/netIEXmit.o sun4.md/netUltra.o sun4.md/netCode.o sun4.md/netRoute.o sun4.md/netEther.o sun4.md/netArp.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
