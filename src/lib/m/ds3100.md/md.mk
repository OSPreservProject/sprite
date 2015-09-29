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
# Thu Nov 21 15:27:20 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__E.c expm1.c fabs.c floor.c j0.c j1.c jn.c lgamma.c log.c log10.c log1p.c log__L.c pow.c sinh.c tanh.c atan2.c cabs.c cbrt.c sincos.c support.c tan.c fmod.c
HDRS		= math.h trig.h
MDPUBHDRS	= 
OBJS		= ds3100.md/acosh.o ds3100.md/asincos.o ds3100.md/asinh.o ds3100.md/atan.o ds3100.md/atanh.o ds3100.md/cosh.o ds3100.md/erf.o ds3100.md/exp.o ds3100.md/exp__E.o ds3100.md/expm1.o ds3100.md/fabs.o ds3100.md/floor.o ds3100.md/j0.o ds3100.md/j1.o ds3100.md/jn.o ds3100.md/lgamma.o ds3100.md/log.o ds3100.md/log10.o ds3100.md/log1p.o ds3100.md/log__L.o ds3100.md/pow.o ds3100.md/sinh.o ds3100.md/tanh.o ds3100.md/atan2.o ds3100.md/cabs.o ds3100.md/cbrt.o ds3100.md/sincos.o ds3100.md/support.o ds3100.md/tan.o ds3100.md/fmod.o
CLEANOBJS	= ds3100.md/acosh.o ds3100.md/asincos.o ds3100.md/asinh.o ds3100.md/atan.o ds3100.md/atanh.o ds3100.md/cosh.o ds3100.md/erf.o ds3100.md/exp.o ds3100.md/exp__E.o ds3100.md/expm1.o ds3100.md/fabs.o ds3100.md/floor.o ds3100.md/j0.o ds3100.md/j1.o ds3100.md/jn.o ds3100.md/lgamma.o ds3100.md/log.o ds3100.md/log10.o ds3100.md/log1p.o ds3100.md/log__L.o ds3100.md/pow.o ds3100.md/sinh.o ds3100.md/tanh.o ds3100.md/atan2.o ds3100.md/cabs.o ds3100.md/cbrt.o ds3100.md/sincos.o ds3100.md/support.o ds3100.md/tan.o ds3100.md/fmod.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
