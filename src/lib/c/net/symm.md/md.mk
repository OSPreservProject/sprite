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
# Mon Jun  8 14:32:06 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= getnetbyname.c getnetent.c getproto.c getprotoent.c getprotoname.c getservbyname.c getservbyport.c herror.c rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c res_query.c res_send.c rexec.c ruserpass.c getnetbyaddr.c getservent.c sethostent.c swap.c gethostnamadr.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/getnetbyname.o symm.md/getnetent.o symm.md/getproto.o symm.md/getprotoent.o symm.md/getprotoname.o symm.md/getservbyname.o symm.md/getservbyport.o symm.md/herror.o symm.md/rcmd.o symm.md/res_comp.o symm.md/res_debug.o symm.md/res_init.o symm.md/res_mkquery.o symm.md/res_query.o symm.md/res_send.o symm.md/rexec.o symm.md/ruserpass.o symm.md/getnetbyaddr.o symm.md/getservent.o symm.md/sethostent.o symm.md/swap.o symm.md/gethostnamadr.o
CLEANOBJS	= symm.md/getnetbyname.o symm.md/getnetent.o symm.md/getproto.o symm.md/getprotoent.o symm.md/getprotoname.o symm.md/getservbyname.o symm.md/getservbyport.o symm.md/herror.o symm.md/rcmd.o symm.md/res_comp.o symm.md/res_debug.o symm.md/res_init.o symm.md/res_mkquery.o symm.md/res_query.o symm.md/res_send.o symm.md/rexec.o symm.md/ruserpass.o symm.md/getnetbyaddr.o symm.md/getservent.o symm.md/sethostent.o symm.md/swap.o symm.md/gethostnamadr.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile
SACREDOBJS	= 
