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
# Fri Apr 24 17:11:52 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= inet_addr.c inet_lnaof.c inet_makeaddr.c inet_netof.c inet_network.c inet_ntoa.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/inet_addr.o ds3100.md/inet_lnaof.o ds3100.md/inet_makeaddr.o ds3100.md/inet_netof.o ds3100.md/inet_network.o ds3100.md/inet_ntoa.o
CLEANOBJS	= ds3100.md/inet_addr.o ds3100.md/inet_lnaof.o ds3100.md/inet_makeaddr.o ds3100.md/inet_netof.o ds3100.md/inet_network.o ds3100.md/inet_ntoa.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
