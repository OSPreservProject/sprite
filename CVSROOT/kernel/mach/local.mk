#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NOOPTIMIZATION	= no -O please

#if !empty(TM:Mds3100)
DISTFILES   +=  ds3100.md/softfp.o
# Can't migrate processes doing compatibility calls.
CFLAGS += -DCANT_MIGRATE_COMPAT
#endif

#if !empty(TM:Msun4) || !empty(TM:Msun4c)
DISTFILES   +=  $(TM).md/sun4 $(TM).md/sun4/reg.h $(TM).md/sun4/fpu \
                $(TM).md/sys $(TM).md/sys/ieeefp.h
#endif

#if !empty(TM:Msun4c)
DISTFILES   +=  $(TM).md/sparcStationPromMap $(TM).md/sunFiles \
                $(TM).md/sunFiles/openprom.h
#endif

#include	<$(SYSMAKEFILE)>

#if !empty(TM:Msun4) || !empty(TM:Msun4c)
INSTFILES   +=  $(TM).md/sun4 $(TM).md/sys
#endif

