#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#
# Must run set-user-id to root. Let's also make it so mere mortals can't
# exec it by accident.
#
INSTALLFLAGS	+= -o root -m 4770

#include	<$(SYSMAKEFILE)>
