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
# Fri Oct 25 22:26:43 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/netConfig.c netRoute.c netArp.c netCode.c netMach.c netEther.c
HDRS		= net.h netArp.h netInt.h netRoute.h netTypes.h netUltraInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/netConfig.o sun3.md/netRoute.o sun3.md/netArp.o sun3.md/netCode.o sun3.md/netMach.o sun3.md/netEther.o
CLEANOBJS	= sun3.md/netConfig.o sun3.md/netRoute.o sun3.md/netArp.o sun3.md/netCode.o sun3.md/netMach.o sun3.md/netEther.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
