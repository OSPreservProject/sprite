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
# Mon Jun  8 14:31:59 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= getnetbyname.c getnetent.c getproto.c getprotoent.c getprotoname.c getservbyname.c getservbyport.c herror.c rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c res_query.c res_send.c rexec.c ruserpass.c getnetbyaddr.c getservent.c sethostent.c swap.c gethostnamadr.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/getnetbyname.o sun4.md/getnetent.o sun4.md/getproto.o sun4.md/getprotoent.o sun4.md/getprotoname.o sun4.md/getservbyname.o sun4.md/getservbyport.o sun4.md/herror.o sun4.md/rcmd.o sun4.md/res_comp.o sun4.md/res_debug.o sun4.md/res_init.o sun4.md/res_mkquery.o sun4.md/res_query.o sun4.md/res_send.o sun4.md/rexec.o sun4.md/ruserpass.o sun4.md/getnetbyaddr.o sun4.md/getservent.o sun4.md/sethostent.o sun4.md/swap.o sun4.md/gethostnamadr.o
CLEANOBJS	= sun4.md/getnetbyname.o sun4.md/getnetent.o sun4.md/getproto.o sun4.md/getprotoent.o sun4.md/getprotoname.o sun4.md/getservbyname.o sun4.md/getservbyport.o sun4.md/herror.o sun4.md/rcmd.o sun4.md/res_comp.o sun4.md/res_debug.o sun4.md/res_init.o sun4.md/res_mkquery.o sun4.md/res_query.o sun4.md/res_send.o sun4.md/rexec.o sun4.md/ruserpass.o sun4.md/getnetbyaddr.o sun4.md/getservent.o sun4.md/sethostent.o sun4.md/swap.o sun4.md/gethostnamadr.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
