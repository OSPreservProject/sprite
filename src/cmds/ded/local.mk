# Choose which version of Unix here.  Currently set up for 4.2 bsd, since
# we are close to that.
#For 4.2 bsd:
SYSTEM= BSD42

# Other things you might want to change
DEDNAME= ded
PRINTER= lpr
PAGER= more
HELPFILE= /sprite/lib/ded.hlp
#HELPFILE= /usr/local/lib/ded.hlp
# DFLTEDITOR is only used if the user's EDITOR environ var is not set.
DFLTEDITOR= vi

# The rest should be pretty standard
CFLAGS += -D$(SYSTEM) -DDEDNAME=\"$(DEDNAME)\" -DPRINTER=\"$(PRINTER)\" -DPAGER=\"$(PAGER)\" -DDFLTEDITOR=\"$(DFLTEDITOR)\" -DHELPFILE=\"$(HELPFILE)\"

LIBS += -ltermcap

#include <$(SYSMAKEFILE)>
