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
# Sat Nov  2 22:41:44 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/netConfig.c netRoute.c netMach.c netArp.c netCode.c netEther.c
HDRS		= net.h netArp.h netInt.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/netConfig.o ds3100.md/netRoute.o ds3100.md/netMach.o ds3100.md/netArp.o ds3100.md/netCode.o ds3100.md/netEther.o
CLEANOBJS	= ds3100.md/netConfig.o ds3100.md/netRoute.o ds3100.md/netMach.o ds3100.md/netArp.o ds3100.md/netCode.o ds3100.md/netEther.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
