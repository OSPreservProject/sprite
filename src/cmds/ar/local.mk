#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

# Enable locking of the archive file.

CFLAGS		+= -DLOCKS -DLOCK_FLOCK -DGNU_TRUNCATION=0

#include	<$(SYSMAKEFILE)>
