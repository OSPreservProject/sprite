#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		= -lmx_g -lutil -lpattern -lmonitorClient -lcmd -lsx_g -ltcl_g -lX11 -lc_g

#include	<$(SYSMAKEFILE)>

.PATH.h:	/sprite/src/lib/monitorClient
