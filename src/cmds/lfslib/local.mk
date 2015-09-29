#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff. 

# The lfs library routines uses this
# file to invoke the C compiler gcc with the -Wall option. The -Wall
# option tells gcc to do lint-like checking while compiling. 
#
# Since the lfs library exist to share code amoung a few programs that
# access LFS file systems, we installed it locally.

INSTALLDIR = /sprite/src/admin/lfslib/installed
INCLUDEDIR = /sprite/src/admin/lfslib/installed
LINTDIR = /sprite/src/admin/lfslib/installed

#include	<$(SYSMAKEFILE)>

#if empty(TM:Mds3100)
CFLAGS +=  -Wall -g
#endif

