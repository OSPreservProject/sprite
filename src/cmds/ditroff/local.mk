#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

CFLAGS	+= -DDEFPRT=\"lw\" \
	   -DDITCAP=\"/sprite/lib/ditroff/ditcap\"

MANPAGES	+= trchange.man

#include <$(SYSMAKEFILE)>

#
# Extra install target for ditcap file.
#

install		:: installlib
installlib	: .SILENT
	$(UPDATE) -m 444 $(INSTALLFLAGS) ditcap /sprite/lib/ditroff/ditcap
