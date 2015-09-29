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
# Thu Nov 21 15:27:35 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= acosh.c asincos.c asinh.c atan.c atanh.c cosh.c erf.c exp.c exp__E.c expm1.c fabs.c floor.c j0.c j1.c jn.c lgamma.c log.c log10.c log1p.c log__L.c pow.c sinh.c tanh.c atan2.c cabs.c cbrt.c sincos.c support.c tan.c fmod.c
HDRS		= math.h trig.h
MDPUBHDRS	= 
OBJS		= sun4.md/acosh.o sun4.md/asincos.o sun4.md/asinh.o sun4.md/atan.o sun4.md/atanh.o sun4.md/cosh.o sun4.md/erf.o sun4.md/exp.o sun4.md/exp__E.o sun4.md/expm1.o sun4.md/fabs.o sun4.md/floor.o sun4.md/j0.o sun4.md/j1.o sun4.md/jn.o sun4.md/lgamma.o sun4.md/log.o sun4.md/log10.o sun4.md/log1p.o sun4.md/log__L.o sun4.md/pow.o sun4.md/sinh.o sun4.md/tanh.o sun4.md/atan2.o sun4.md/cabs.o sun4.md/cbrt.o sun4.md/sincos.o sun4.md/support.o sun4.md/tan.o sun4.md/fmod.o
CLEANOBJS	= sun4.md/acosh.o sun4.md/asincos.o sun4.md/asinh.o sun4.md/atan.o sun4.md/atanh.o sun4.md/cosh.o sun4.md/erf.o sun4.md/exp.o sun4.md/exp__E.o sun4.md/expm1.o sun4.md/fabs.o sun4.md/floor.o sun4.md/j0.o sun4.md/j1.o sun4.md/jn.o sun4.md/lgamma.o sun4.md/log.o sun4.md/log10.o sun4.md/log1p.o sun4.md/log__L.o sun4.md/pow.o sun4.md/sinh.o sun4.md/tanh.o sun4.md/atan2.o sun4.md/cabs.o sun4.md/cbrt.o sun4.md/sincos.o sun4.md/support.o sun4.md/tan.o sun4.md/fmod.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
