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
# Mon Jun  8 14:27:52 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= symm.md/_adddf3.s symm.md/_addsf3.s symm.md/_ashlsi3.s symm.md/_ashrsi3.s symm.md/_builtin_New.s symm.md/_builtin_del.s symm.md/_builtin_new.s symm.md/_cmpdf2.s symm.md/_cmpdi2.s symm.md/_cmpsf2.s symm.md/_divdf3.s symm.md/_divsf3.s symm.md/_divsi3.s symm.md/_extendsfdf2.s symm.md/_fixdfdi.s symm.md/_fixdfsi.s symm.md/_fixunsdfdi.s symm.md/_fixunsdfsi.s symm.md/_floatdidf.s symm.md/_lshlsi3.s symm.md/_floatsidf.s symm.md/_lshrsi3.s symm.md/_modsi3.s symm.md/_muldf3.s symm.md/_mulsf3.s symm.md/_mulsi3.s symm.md/_negdf2.s symm.md/_negsf2.s symm.md/_subdf3.s symm.md/_subsf3.s symm.md/_truncdfsf2.s symm.md/_ucmpdi2.s symm.md/_udivsi3.s symm.md/_umodsi3.s symm.md/_umulsi3.s symm.md/alloca.c _adddi3.c _eprintf.c _anddi3.c _ashldi3.c _ashrdi3.c _bdiv.c _cmpdi2.c _divdi3.c _fixdfdi.c _fixunsdfdi.c _floatdidf.c _iordi3.c _lshldi3.c _lshrdi3.c _moddi3.c _muldi3.c _negdi2.c _one_cmpldi2.c _subdi3.c _ucmpdi2.c _udivdi3.c _umoddi3.c _xordi3.c gnulib3.c
HDRS		= symm.md/config.h symm.md/tm-bsd386.h symm.md/tm-i386.h symm.md/tm.h gnulib2.h
MDPUBHDRS	= 
OBJS		= symm.md/_adddf3.o symm.md/_addsf3.o symm.md/_ashlsi3.o symm.md/_ashrsi3.o symm.md/_builtin_New.o symm.md/_builtin_del.o symm.md/_builtin_new.o symm.md/_cmpdf2.o symm.md/_cmpdi2.o symm.md/_cmpsf2.o symm.md/_divdf3.o symm.md/_divsf3.o symm.md/_divsi3.o symm.md/_extendsfdf2.o symm.md/_fixdfdi.o symm.md/_fixdfsi.o symm.md/_fixunsdfdi.o symm.md/_fixunsdfsi.o symm.md/_floatdidf.o symm.md/_lshlsi3.o symm.md/_floatsidf.o symm.md/_lshrsi3.o symm.md/_modsi3.o symm.md/_muldf3.o symm.md/_mulsf3.o symm.md/_mulsi3.o symm.md/_negdf2.o symm.md/_negsf2.o symm.md/_subdf3.o symm.md/_subsf3.o symm.md/_truncdfsf2.o symm.md/_ucmpdi2.o symm.md/_udivsi3.o symm.md/_umodsi3.o symm.md/_umulsi3.o symm.md/alloca.o symm.md/_adddi3.o symm.md/_eprintf.o symm.md/_anddi3.o symm.md/_ashldi3.o symm.md/_ashrdi3.o symm.md/_bdiv.o symm.md/_cmpdi2.o symm.md/_divdi3.o symm.md/_fixdfdi.o symm.md/_fixunsdfdi.o symm.md/_floatdidf.o symm.md/_iordi3.o symm.md/_lshldi3.o symm.md/_lshrdi3.o symm.md/_moddi3.o symm.md/_muldi3.o symm.md/_negdi2.o symm.md/_one_cmpldi2.o symm.md/_subdi3.o symm.md/_ucmpdi2.o symm.md/_udivdi3.o symm.md/_umoddi3.o symm.md/_xordi3.o symm.md/gnulib3.o
CLEANOBJS	= symm.md/_adddf3.o symm.md/_addsf3.o symm.md/_ashlsi3.o symm.md/_ashrsi3.o symm.md/_builtin_New.o symm.md/_builtin_del.o symm.md/_builtin_new.o symm.md/_cmpdf2.o symm.md/_cmpdi2.o symm.md/_cmpsf2.o symm.md/_divdf3.o symm.md/_divsf3.o symm.md/_divsi3.o symm.md/_extendsfdf2.o symm.md/_fixdfdi.o symm.md/_fixdfsi.o symm.md/_fixunsdfdi.o symm.md/_fixunsdfsi.o symm.md/_floatdidf.o symm.md/_lshlsi3.o symm.md/_floatsidf.o symm.md/_lshrsi3.o symm.md/_modsi3.o symm.md/_muldf3.o symm.md/_mulsf3.o symm.md/_mulsi3.o symm.md/_negdf2.o symm.md/_negsf2.o symm.md/_subdf3.o symm.md/_subsf3.o symm.md/_truncdfsf2.o symm.md/_ucmpdi2.o symm.md/_udivsi3.o symm.md/_umodsi3.o symm.md/_umulsi3.o symm.md/alloca.o symm.md/_adddi3.o symm.md/_eprintf.o symm.md/_anddi3.o symm.md/_ashldi3.o symm.md/_ashrdi3.o symm.md/_bdiv.o symm.md/_cmpdi2.o symm.md/_divdi3.o symm.md/_fixdfdi.o symm.md/_fixunsdfdi.o symm.md/_floatdidf.o symm.md/_iordi3.o symm.md/_lshldi3.o symm.md/_lshrdi3.o symm.md/_moddi3.o symm.md/_muldi3.o symm.md/_negdi2.o symm.md/_one_cmpldi2.o symm.md/_subdi3.o symm.md/_ucmpdi2.o symm.md/_udivdi3.o symm.md/_umoddi3.o symm.md/_xordi3.o symm.md/gnulib3.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
