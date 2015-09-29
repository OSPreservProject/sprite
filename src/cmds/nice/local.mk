#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

MANPAGES	+= nohup.man

#include	<$(SYSMAKEFILE)>

nohup.man:	nice.man
		ln -s $(.OODATE) $(.TARGET)

install::	$(TMINSTALLDIR)/nohup

$(TMINSTALLDIR)/nohup:	nohup.sh
		$(UPDATE) -l -m 555 $(INSTALLFLAGS) $(.OODATE) $(.TARGET)
