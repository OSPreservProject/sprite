#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#


#
# Install as `gas' instead of `as' on decStations.
#
#if   !empty(TM:Mds3100)
NAME            =   gas
CFLAGS          += -DTARGET_DS3100

#elif !empty(TM:Msun2)
CFLAGS          += -DTARGET_SUN2

#elif !empty(TM:Msun3)
CFLAGS          += -DTARGET_SUN3

#elif !empty(TM:Msun4)
CFLAGS          += -DTARGET_SUN4

#endif

#include	<$(SYSMAKEFILE)>

