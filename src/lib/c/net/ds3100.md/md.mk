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
# Mon Jun  8 14:31:47 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= getnetbyname.c getnetent.c getproto.c getprotoent.c getprotoname.c getservbyname.c getservbyport.c herror.c rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c res_query.c res_send.c rexec.c ruserpass.c getnetbyaddr.c getservent.c sethostent.c swap.c gethostnamadr.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/gethostnamadr.o ds3100.md/getnetbyaddr.o ds3100.md/getnetbyname.o ds3100.md/getnetent.o ds3100.md/getproto.o ds3100.md/getprotoent.o ds3100.md/getprotoname.o ds3100.md/getservbyname.o ds3100.md/getservbyport.o ds3100.md/getservent.o ds3100.md/herror.o ds3100.md/rcmd.o ds3100.md/res_comp.o ds3100.md/res_debug.o ds3100.md/res_init.o ds3100.md/res_mkquery.o ds3100.md/res_query.o ds3100.md/res_send.o ds3100.md/rexec.o ds3100.md/ruserpass.o ds3100.md/sethostent.o ds3100.md/swap.o
CLEANOBJS	= ds3100.md/getnetbyname.o ds3100.md/getnetent.o ds3100.md/getproto.o ds3100.md/getprotoent.o ds3100.md/getprotoname.o ds3100.md/getservbyname.o ds3100.md/getservbyport.o ds3100.md/herror.o ds3100.md/rcmd.o ds3100.md/res_comp.o ds3100.md/res_debug.o ds3100.md/res_init.o ds3100.md/res_mkquery.o ds3100.md/res_query.o ds3100.md/res_send.o ds3100.md/rexec.o ds3100.md/ruserpass.o ds3100.md/getnetbyaddr.o ds3100.md/getservent.o ds3100.md/sethostent.o ds3100.md/swap.o ds3100.md/gethostnamadr.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
