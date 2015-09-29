#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

# Override SUBDIRS to only hold those subdirectory names that should
# be included in recursive "make" operations.  As of today (6/17/89)
# this is just a first guess, and may not be completely right (JO).

SUBDIRS		= c cmd curses dbm l m monitorClient mx net old pattern \
		  sunrpc sx tcl termlib util

#include	<$(SYSMAKEFILE)>
