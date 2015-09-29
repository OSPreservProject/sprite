
NAME	= 
DEVDIR	= /sprite/lib/ditroff

#include <$(SYSMAKEFILE)>

default::	$(TM).md/DESC.out

$(TM).md/DESC.out:	DESC R I B CW S
	makedev DESC
	mv DESC.out $(TM).md

install::	specfile $(TM).md/DESC.out
	update -m 644 DESC specfile $(TM).md/DESC.out $(DEVDIR)/devter

clean::
	rm -f *.out $(TM).md/*.out
