#
# Prototype makefile for libraries with multiple source directories
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.biglib
# Fri Aug 10 16:21:09 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.biglib,v 1.15 89/10/09 21:28:28 rab Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= _adddi3.c _anddi3.c _ashldi3.c _ashrdi3.c _bdiv.c _cmpdi2.c _divdi3.c _eprintf.c _fixdfdi.c _fixunsdfdi.c _floatdidf.c _iordi3.c _lshldi3.c _lshrdi3.c _moddi3.c _muldi3.c _negdi2.c _one_cmpldi2.c _subdi3.c _ucmpdi2.c _udivdi3.c _umoddi3.c _xordi3.c gnulib3.c spur.md/__adddf3.c spur.md/__addsf3.c spur.md/__ashlsi3.c spur.md/__ashrsi3.c spur.md/__checkExp.c spur.md/__cmpdf2.c spur.md/__cmpsf2.c spur.md/__divdf3.c spur.md/__divsf3.c spur.md/__divsi3.c spur.md/__eprintf.c spur.md/__extendsfdf2.c spur.md/__fixdfdi.c spur.md/__fixdfsi.c spur.md/__fixunsdfdi.c spur.md/__fixunsdfsi.c spur.md/__floatdidf.c spur.md/__floatsidf.c spur.md/__lshlsi3.c spur.md/__lshrsi3.c spur.md/__modsi3.c spur.md/__muldf3.c spur.md/__mulsf3.c spur.md/__mulsi3.c spur.md/__negdf2.c spur.md/__negsf2.c spur.md/__normalize.c spur.md/__shld.c spur.md/__subd.c spur.md/__subdf3.c spur.md/__subsf3.c spur.md/__testNan.c spur.md/__testNanAdd.c spur.md/__truncdfsf2.c spur.md/__udivsi3.c spur.md/__umodsi3.c spur.md/__umulsi3.c spur.md/_adddf3.c spur.md/_addsf3.c spur.md/_ashlsi3.c spur.md/_ashrsi3.c spur.md/_checkExp.c spur.md/_cmpdf2.c spur.md/_cmpsf2.c spur.md/_divdf3.c spur.md/_divsf3.c spur.md/_divsi3.c spur.md/_eprintf.c spur.md/_extendsfdf2.c spur.md/_fixdfdi.c spur.md/_fixdfsi.c spur.md/_fixunsdfdi.c spur.md/_fixunsdfsi.c spur.md/_floatdidf.c spur.md/_floatsidf.c spur.md/_lshlsi3.c spur.md/_lshrsi3.c spur.md/_modsi3.c spur.md/_muldf3.c spur.md/_mulsf3.c spur.md/_mulsi3.c spur.md/_negdf2.c spur.md/_negsf2.c spur.md/_normalize.c spur.md/_shld.c spur.md/_subd.c spur.md/_subdf3.c spur.md/_subsf3.c spur.md/_testNan.c spur.md/_testNanAdd.c spur.md/_truncdfsf2.c spur.md/_udivsi3.c spur.md/_umodsi3.c spur.md/_umulsi3.c spur.md/adddf3.c spur.md/modf.c sun3.md/_adddf3.s sun3.md/_addsf3.s sun3.md/_ashlsi3.s sun3.md/_ashrsi3.s sun3.md/_builtin_New.s sun3.md/_builtin_del.s sun3.md/_builtin_new.s sun3.md/_cmpdf2.s sun3.md/_cmpdi2.s sun3.md/_cmpsf2.s sun3.md/_divdf3.s sun3.md/_divsf3.s sun3.md/_divsi3.s sun3.md/_extendsfdf2.s sun3.md/_fixdfdi.s sun3.md/_fixdfsi.s sun3.md/_fixunsdfdi.s sun3.md/_fixunsdfsi.s sun3.md/_floatdidf.s sun3.md/_floatsidf.s sun3.md/_lshlsi3.s sun3.md/_lshrsi3.s sun3.md/_modsi3.s sun3.md/_muldf3.s sun3.md/_mulsf3.s sun3.md/_mulsi3.s sun3.md/_negdf2.s sun3.md/_negsf2.s sun3.md/_subdf3.s sun3.md/_subsf3.s sun3.md/_truncdfsf2.s sun3.md/_ucmpdi2.s sun3.md/_udivsi3.s sun3.md/_umodsi3.s sun3.md/_umulsi3.s sun4.md/_builtin_New.s sun4.md/_builtin_del.s sun4.md/_builtin_new.s sun4.md/_dtou.s sun4.md/_fixunsdfsi.s sun4.md/alloca.s sun4.md/divide.s sun4.md/modf.s sun4.md/multiply.s sun4.md/ptr_call.s sun4.md/rem.s sun4.md/stret1.s sun4.md/stret2.s sun4.md/stret4.s sun4.md/umultiply.s sun4.md/varargs.s symm.md/_adddf3.s symm.md/_addsf3.s symm.md/_ashlsi3.s symm.md/_ashrsi3.s symm.md/_builtin_New.s symm.md/_builtin_del.s symm.md/_builtin_new.s symm.md/_cmpdf2.s symm.md/_cmpdi2.s symm.md/_cmpsf2.s symm.md/_divdf3.s symm.md/_divsf3.s symm.md/_divsi3.s symm.md/_extendsfdf2.s symm.md/_fixdfdi.s symm.md/_fixdfsi.s symm.md/_fixunsdfdi.s symm.md/_fixunsdfsi.s symm.md/_floatdidf.s symm.md/_floatsidf.s symm.md/_lshlsi3.s symm.md/_lshrsi3.s symm.md/_modsi3.s symm.md/_muldf3.s symm.md/_mulsf3.s symm.md/_mulsi3.s symm.md/_negdf2.s symm.md/_negsf2.s symm.md/_subdf3.s symm.md/_subsf3.s symm.md/_truncdfsf2.s symm.md/_ucmpdi2.s symm.md/_udivsi3.s symm.md/_umodsi3.s symm.md/_umulsi3.s symm.md/alloca.c
LINTSRCS	= 
MACHINES	= ds3100 spur sun3 sun4 symm
MAKEFILE	= Makefile
MANPAGES	= 
NAME		= c
PUBHDRS		= gnulib2.h
SUBDIR		= gnulib
TM     	        ?= $(MACHINE)
TYPE		= sprite
DISTDIR         ?= /dist/sprite/src/lib/c/gnulib

SYSMAKEFILE	= biglib.mk

#if exists($(TM).md/md.mk)
#include	"$(TM).md/md.mk"
#endif

#if exists(local.mk)
#include 	"local.mk"
#else
#include	<$(SYSMAKEFILE)>
#endif

#if exists($(DEPFILE))
#include	"$(DEPFILE)"
#endif
