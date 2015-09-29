#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
# 
# $Header: /sprite/src/kernel/libc/RCS/local.mk,v 1.1 90/09/11 12:39:05 kupfer Exp $
#

# Make sure that we don't use a floating point coprocessor.
# Ordinarily <math.h> tries to use the 68881 on sun3s.

#if !empty(TM:Msun3)
CFLAGS		+= -D__SOFT_FLOAT__
#endif

#include	<$(SYSMAKEFILE)>
