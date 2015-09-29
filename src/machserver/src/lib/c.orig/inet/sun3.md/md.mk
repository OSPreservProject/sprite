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
# Tue Oct 24 00:36:47 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= inet_addr.c inet_lnaof.c inet_makeaddr.c inet_netof.c inet_network.c inet_ntoa.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/inet_addr.o sun3.md/inet_lnaof.o sun3.md/inet_makeaddr.o sun3.md/inet_netof.o sun3.md/inet_network.o sun3.md/inet_ntoa.o
CLEANOBJS	= sun3.md/inet_addr.o sun3.md/inet_lnaof.o sun3.md/inet_makeaddr.o sun3.md/inet_netof.o sun3.md/inet_network.o sun3.md/inet_ntoa.o
DISTDIR        ?= @(DISTDIR)
