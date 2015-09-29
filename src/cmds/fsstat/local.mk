#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

# If the sun3 version gets optimized, you get NaN for some of the Mb
# Read and Mb Write numbers.  (Even with optimization off, the Mb
# percentages seem buggy, sigh.)

#if !empty(TM:Msun3)
NOOPTIMIZATION	= dont do it
#endif

#include	<$(SYSMAKEFILE)>
