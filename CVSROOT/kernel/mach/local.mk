#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

NAME = mach

NOOPTIMIZATION	= no -O please

#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
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

#if !empty(TM:Msymm)

# The symmetry generates a header file for assembler files on the fly.
# This is a problem for two reasons 1) we need a program to generate
# the file, and 2) we can only run the program on the symmetry. If the
# header file is out of date then you can't cross-compile.
#

machAsmSymbols.h:	symm.md/machGenAsmSymbols
#if empty(MACHINE:Msymm)
	@echo "You must compile this on a symmetry"
	@exit 1
#else
	rm -f symm.md/machAsmSymbols.h
	symm.md/machGenAsmSymbols > symm.md/machAsmSymbols.h
#endif

symm.md/machGenAsmSymbols:	symm.md/machGenAsmSymbols.c ${HDRS:Nsymm.md/machAsmSymbols.h}
	${CC} ${CFLAGS} -o $(.TARGET) symm.md/machGenAsmSymbols.c
#endif
