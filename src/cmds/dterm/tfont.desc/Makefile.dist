DEVDIR	= /usr/local/lib/font

all:
	@echo tfont ok

install:
	../../bin/devconfig DESC
	cp DESC.out $(DEVDIR)/devter/DESC.out
	chmod 644 $(DEVDIR)/devter/DESC.out
	cp specfile $(DEVDIR)/devter/specfile
	chmod 644 $(DEVDIR)/devter/specfile
	/bin/rm -f *.out
