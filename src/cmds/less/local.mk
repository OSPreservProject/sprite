
LIBS	+= -ltermcap

#include <$(SYSMAKEFILE)>

install::
	$(UPDATE) -m 444 -l less.hlp /sprite/lib/less/less.help

default install clean cleanall::	.NOEXPORT
	(cd lesskey; $(MAKE) $(MFLAGS) TM=$(TM) $(.TARGET))
