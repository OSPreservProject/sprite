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
# Fri Apr 13 11:17:21 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= _adddi3.c _anddi3.c _ashldi3.c _ashrdi3.c _bdiv.c _cmpdi2.c _divdi3.c _eprintf.c _fixdfdi.c _fixunsdfdi.c _floatdidf.c _iordi3.c _lshldi3.c _lshrdi3.c _moddi3.c _muldi3.c _negdi2.c _one_cmpldi2.c _subdi3.c _ucmpdi2.c _udivdi3.c _umoddi3.c _xordi3.c gnulib3.c
HDRS		= ds3100.md/config.h ds3100.md/tm.h gnulib2.h
MDPUBHDRS	= 
OBJS		= ds3100.md/_adddi3.o ds3100.md/_anddi3.o ds3100.md/_ashldi3.o ds3100.md/_ashrdi3.o ds3100.md/_bdiv.o ds3100.md/_cmpdi2.o ds3100.md/_divdi3.o ds3100.md/_eprintf.o ds3100.md/_fixdfdi.o ds3100.md/_fixunsdfdi.o ds3100.md/_floatdidf.o ds3100.md/_iordi3.o ds3100.md/_lshldi3.o ds3100.md/_lshrdi3.o ds3100.md/_moddi3.o ds3100.md/_muldi3.o ds3100.md/_negdi2.o ds3100.md/_one_cmpldi2.o ds3100.md/_subdi3.o ds3100.md/_ucmpdi2.o ds3100.md/_udivdi3.o ds3100.md/_umoddi3.o ds3100.md/_xordi3.o ds3100.md/gnulib3.o
CLEANOBJS	= ds3100.md/_adddi3.o ds3100.md/_anddi3.o ds3100.md/_ashldi3.o ds3100.md/_ashrdi3.o ds3100.md/_bdiv.o ds3100.md/_cmpdi2.o ds3100.md/_divdi3.o ds3100.md/_eprintf.o ds3100.md/_fixdfdi.o ds3100.md/_fixunsdfdi.o ds3100.md/_floatdidf.o ds3100.md/_iordi3.o ds3100.md/_lshldi3.o ds3100.md/_lshrdi3.o ds3100.md/_moddi3.o ds3100.md/_muldi3.o ds3100.md/_negdi2.o ds3100.md/_one_cmpldi2.o ds3100.md/_subdi3.o ds3100.md/_ucmpdi2.o ds3100.md/_udivdi3.o ds3100.md/_umoddi3.o ds3100.md/_xordi3.o ds3100.md/gnulib3.o
INSTFILES	= ds3100.md/md.mk Makefile local.mk
SACREDOBJS	= 
