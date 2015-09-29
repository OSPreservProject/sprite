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
# Mon Jun  8 14:27:38 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/_dtou.s sun4.md/alloca.s sun4.md/divide.s sun4.md/stret2.s sun4.md/modf.s sun4.md/multiply.s sun4.md/ptr_call.s sun4.md/rem.s sun4.md/stret1.s sun4.md/stret4.s sun4.md/umultiply.s sun4.md/varargs.s sun4.md/_fixunsdfsi.s sun4.md/_builtin_New.s sun4.md/_builtin_new.s sun4.md/_builtin_del.s _adddi3.c _eprintf.c _anddi3.c _ashldi3.c _ashrdi3.c _bdiv.c _cmpdi2.c _divdi3.c _fixdfdi.c _fixunsdfdi.c _floatdidf.c _iordi3.c _lshldi3.c _lshrdi3.c _moddi3.c _muldi3.c _negdi2.c _one_cmpldi2.c _subdi3.c _ucmpdi2.c _udivdi3.c _umoddi3.c _xordi3.c gnulib3.c
HDRS		= sun4.md/config.h sun4.md/tm.h gnulib2.h
MDPUBHDRS	= 
OBJS		= sun4.md/_dtou.o sun4.md/alloca.o sun4.md/divide.o sun4.md/stret2.o sun4.md/modf.o sun4.md/multiply.o sun4.md/ptr_call.o sun4.md/rem.o sun4.md/stret1.o sun4.md/stret4.o sun4.md/umultiply.o sun4.md/varargs.o sun4.md/_fixunsdfsi.o sun4.md/_builtin_New.o sun4.md/_builtin_new.o sun4.md/_builtin_del.o sun4.md/_adddi3.o sun4.md/_eprintf.o sun4.md/_anddi3.o sun4.md/_ashldi3.o sun4.md/_ashrdi3.o sun4.md/_bdiv.o sun4.md/_cmpdi2.o sun4.md/_divdi3.o sun4.md/_fixdfdi.o sun4.md/_fixunsdfdi.o sun4.md/_floatdidf.o sun4.md/_iordi3.o sun4.md/_lshldi3.o sun4.md/_lshrdi3.o sun4.md/_moddi3.o sun4.md/_muldi3.o sun4.md/_negdi2.o sun4.md/_one_cmpldi2.o sun4.md/_subdi3.o sun4.md/_ucmpdi2.o sun4.md/_udivdi3.o sun4.md/_umoddi3.o sun4.md/_xordi3.o sun4.md/gnulib3.o
CLEANOBJS	= sun4.md/_dtou.o sun4.md/alloca.o sun4.md/divide.o sun4.md/stret2.o sun4.md/modf.o sun4.md/multiply.o sun4.md/ptr_call.o sun4.md/rem.o sun4.md/stret1.o sun4.md/stret4.o sun4.md/umultiply.o sun4.md/varargs.o sun4.md/_fixunsdfsi.o sun4.md/_builtin_New.o sun4.md/_builtin_new.o sun4.md/_builtin_del.o sun4.md/_adddi3.o sun4.md/_eprintf.o sun4.md/_anddi3.o sun4.md/_ashldi3.o sun4.md/_ashrdi3.o sun4.md/_bdiv.o sun4.md/_cmpdi2.o sun4.md/_divdi3.o sun4.md/_fixdfdi.o sun4.md/_fixunsdfdi.o sun4.md/_floatdidf.o sun4.md/_iordi3.o sun4.md/_lshldi3.o sun4.md/_lshrdi3.o sun4.md/_moddi3.o sun4.md/_muldi3.o sun4.md/_negdi2.o sun4.md/_one_cmpldi2.o sun4.md/_subdi3.o sun4.md/_ucmpdi2.o sun4.md/_udivdi3.o sun4.md/_umoddi3.o sun4.md/_xordi3.o sun4.md/gnulib3.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
