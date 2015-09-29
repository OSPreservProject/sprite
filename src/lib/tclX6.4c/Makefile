MAKEFLAGS = -B -f Makefile.sprite
.MAKEFLAGS : $(MAKEFLAGS)

default	default install clean : $(MACHINE).md/Makefile.sprite Makefiles
	cd $(MACHINE).md; $(.PMAKE) $(MAKEFLAGS) $(.TARGET)

$(MACHINE).md/Makefile.sprite : $(MACHINE).md/Makefile Makefile.sed
	sed -f Makefile.sed < $(MACHINE).md/Makefile > $(.TARGET)


MAKEFILES = 	$(MACHINE).md/ucbsrc/Makefile.sprite 	\
		$(MACHINE).md/osSupport/Makefile.sprite \
		$(MACHINE).md/tclsrc/Makefile.sprite 	\
		$(MACHINE).md/src/Makefile.sprite	\
		$(MACHINE).md/tkucbsrc/Makefile.sprite	\
		$(MACHINE).md/tksrc/Makefile.sprite

Makefiles : $(MACHINE).md/Makefile.sprite $(MAKEFILES)

$(MAKEFILES) : Makefile.sed $(.TARGET:R) MAKEIT 

MAKEIT : .USE
	sed -f $(.ALLSRC) > $(.TARGET)

