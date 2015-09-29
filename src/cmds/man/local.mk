#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff for configuration file.
#
install		:: installconfig
installconfig	: .SILENT
	$(UPDATE) -m 444 $(INSTALLFLAGS) config /sprite/lib/man/config
