#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

LIBS		= -ltermlib

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff for help message.
#
install		:: installlib
installlib	: .SILENT
	$(UPDATE) -m 664 -o root $(INSTALLFLAGS) more.help\
		/sprite/lib/more/more.help
