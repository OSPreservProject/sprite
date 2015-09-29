LIBS += -ll

HELPDIR		= /sprite/lib
HELPFILE 	= nslookup.help

XCFLAGS += -DHELPFILE=\"$(HELPDIR)/$(HELPFILE)\"

#include <$(SYSMAKEFILE)>

install		::
	$(UPDATE) -m 444 $(HELPFILE) $(HELPDIR)
#	$(UPDATE) -m 444 nslookup.1 /sprite/doc/ref/cmds/nslookup


