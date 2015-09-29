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
# Mon Jun  8 14:27:29 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/_extendsfdf2.s sun3.md/_builtin_new.s sun3.md/_lshrsi3.s sun3.md/_subsf3.s sun3.md/_divdf3.s sun3.md/_adddf3.s sun3.md/_addsf3.s sun3.md/_ashlsi3.s sun3.md/_ashrsi3.s sun3.md/_builtin_New.s sun3.md/_builtin_del.s sun3.md/_cmpdf2.s sun3.md/_cmpdi2.s sun3.md/_cmpsf2.s sun3.md/_divsf3.s sun3.md/_divsi3.s sun3.md/_fixdfdi.s sun3.md/_fixdfsi.s sun3.md/_fixunsdfdi.s sun3.md/_fixunsdfsi.s sun3.md/_floatdidf.s sun3.md/_floatsidf.s sun3.md/_lshlsi3.s sun3.md/_modsi3.s sun3.md/_muldf3.s sun3.md/_mulsf3.s sun3.md/_mulsi3.s sun3.md/_negdf2.s sun3.md/_negsf2.s sun3.md/_subdf3.s sun3.md/_truncdfsf2.s sun3.md/_ucmpdi2.s sun3.md/_udivsi3.s sun3.md/_umodsi3.s sun3.md/_umulsi3.s _adddi3.c _eprintf.c _anddi3.c _ashldi3.c _ashrdi3.c _bdiv.c _cmpdi2.c _divdi3.c _fixdfdi.c _fixunsdfdi.c _floatdidf.c _iordi3.c _lshldi3.c _lshrdi3.c _moddi3.c _muldi3.c _negdi2.c _one_cmpldi2.c _subdi3.c _ucmpdi2.c _udivdi3.c _umoddi3.c _xordi3.c gnulib3.c
HDRS		= sun3.md/config.h sun3.md/tm-m68k.h sun3.md/tm.h gnulib2.h
MDPUBHDRS	= 
OBJS		= sun3.md/_extendsfdf2.o sun3.md/_builtin_new.o sun3.md/_lshrsi3.o sun3.md/_subsf3.o sun3.md/_divdf3.o sun3.md/_adddf3.o sun3.md/_addsf3.o sun3.md/_ashlsi3.o sun3.md/_ashrsi3.o sun3.md/_builtin_New.o sun3.md/_builtin_del.o sun3.md/_cmpdf2.o sun3.md/_cmpdi2.o sun3.md/_cmpsf2.o sun3.md/_divsf3.o sun3.md/_divsi3.o sun3.md/_fixdfdi.o sun3.md/_fixdfsi.o sun3.md/_fixunsdfdi.o sun3.md/_fixunsdfsi.o sun3.md/_floatdidf.o sun3.md/_floatsidf.o sun3.md/_lshlsi3.o sun3.md/_modsi3.o sun3.md/_muldf3.o sun3.md/_mulsf3.o sun3.md/_mulsi3.o sun3.md/_negdf2.o sun3.md/_negsf2.o sun3.md/_subdf3.o sun3.md/_truncdfsf2.o sun3.md/_ucmpdi2.o sun3.md/_udivsi3.o sun3.md/_umodsi3.o sun3.md/_umulsi3.o sun3.md/_adddi3.o sun3.md/_eprintf.o sun3.md/_anddi3.o sun3.md/_ashldi3.o sun3.md/_ashrdi3.o sun3.md/_bdiv.o sun3.md/_cmpdi2.o sun3.md/_divdi3.o sun3.md/_fixdfdi.o sun3.md/_fixunsdfdi.o sun3.md/_floatdidf.o sun3.md/_iordi3.o sun3.md/_lshldi3.o sun3.md/_lshrdi3.o sun3.md/_moddi3.o sun3.md/_muldi3.o sun3.md/_negdi2.o sun3.md/_one_cmpldi2.o sun3.md/_subdi3.o sun3.md/_ucmpdi2.o sun3.md/_udivdi3.o sun3.md/_umoddi3.o sun3.md/_xordi3.o sun3.md/gnulib3.o
CLEANOBJS	= sun3.md/_extendsfdf2.o sun3.md/_builtin_new.o sun3.md/_lshrsi3.o sun3.md/_subsf3.o sun3.md/_divdf3.o sun3.md/_adddf3.o sun3.md/_addsf3.o sun3.md/_ashlsi3.o sun3.md/_ashrsi3.o sun3.md/_builtin_New.o sun3.md/_builtin_del.o sun3.md/_cmpdf2.o sun3.md/_cmpdi2.o sun3.md/_cmpsf2.o sun3.md/_divsf3.o sun3.md/_divsi3.o sun3.md/_fixdfdi.o sun3.md/_fixdfsi.o sun3.md/_fixunsdfdi.o sun3.md/_fixunsdfsi.o sun3.md/_floatdidf.o sun3.md/_floatsidf.o sun3.md/_lshlsi3.o sun3.md/_modsi3.o sun3.md/_muldf3.o sun3.md/_mulsf3.o sun3.md/_mulsi3.o sun3.md/_negdf2.o sun3.md/_negsf2.o sun3.md/_subdf3.o sun3.md/_truncdfsf2.o sun3.md/_ucmpdi2.o sun3.md/_udivsi3.o sun3.md/_umodsi3.o sun3.md/_umulsi3.o sun3.md/_adddi3.o sun3.md/_eprintf.o sun3.md/_anddi3.o sun3.md/_ashldi3.o sun3.md/_ashrdi3.o sun3.md/_bdiv.o sun3.md/_cmpdi2.o sun3.md/_divdi3.o sun3.md/_fixdfdi.o sun3.md/_fixunsdfdi.o sun3.md/_floatdidf.o sun3.md/_iordi3.o sun3.md/_lshldi3.o sun3.md/_lshrdi3.o sun3.md/_moddi3.o sun3.md/_muldi3.o sun3.md/_negdi2.o sun3.md/_one_cmpldi2.o sun3.md/_subdi3.o sun3.md/_ucmpdi2.o sun3.md/_udivdi3.o sun3.md/_umoddi3.o sun3.md/_xordi3.o sun3.md/gnulib3.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
