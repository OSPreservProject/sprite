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
# Mon Jun  8 14:31:53 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= getnetbyname.c getnetent.c getproto.c getprotoent.c getprotoname.c getservbyname.c getservbyport.c herror.c rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c res_query.c res_send.c rexec.c ruserpass.c getnetbyaddr.c getservent.c sethostent.c swap.c gethostnamadr.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/getnetbyname.o sun3.md/getnetent.o sun3.md/getproto.o sun3.md/getprotoent.o sun3.md/getprotoname.o sun3.md/getservbyname.o sun3.md/getservbyport.o sun3.md/herror.o sun3.md/rcmd.o sun3.md/res_comp.o sun3.md/res_debug.o sun3.md/res_init.o sun3.md/res_mkquery.o sun3.md/res_query.o sun3.md/res_send.o sun3.md/rexec.o sun3.md/ruserpass.o sun3.md/getnetbyaddr.o sun3.md/getservent.o sun3.md/sethostent.o sun3.md/swap.o sun3.md/gethostnamadr.o
CLEANOBJS	= sun3.md/getnetbyname.o sun3.md/getnetent.o sun3.md/getproto.o sun3.md/getprotoent.o sun3.md/getprotoname.o sun3.md/getservbyname.o sun3.md/getservbyport.o sun3.md/herror.o sun3.md/rcmd.o sun3.md/res_comp.o sun3.md/res_debug.o sun3.md/res_init.o sun3.md/res_mkquery.o sun3.md/res_query.o sun3.md/res_send.o sun3.md/rexec.o sun3.md/ruserpass.o sun3.md/getnetbyaddr.o sun3.md/getservent.o sun3.md/sethostent.o sun3.md/swap.o sun3.md/gethostnamadr.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
