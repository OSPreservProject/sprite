#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile.
#

#if !empty(TM:Mds3100)
NAME		= cpp.gnu
#endif

#
# Don't automatically back up Gcc stuff:  want to make sure the
# backup version is VERY reliable and don't want two quick buggy
# changes to result in unusable backup.
#
NOBACKUP	= true

#include	<$(SYSMAKEFILE)>

#
# Use headers from the main gcc area, including both stuff from
# the distribution and stuff that's been modified specially for Sprite.
#
.PATH.h		: ../gcc/sprite ../gcc/dist
