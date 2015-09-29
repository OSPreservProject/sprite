#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provdied by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

#include	<$(SYSMAKEFILE)>

#
# Add more install stuff in order install library files, synonyms.
#

install		:: libraryFiles synonym
libraryFiles	: .SILENT
	$(UPDATE) -m 444 Mail.help Mail.rc Mail.tildehelp \
		/sprite/lib/$(NAME)
synonym		: .SILENT
	-if test ! -h $(INSTALLDIR).$(TM)/mail; then \
		echo Creating link for $(INSTALLDIR).$(TM)/mail; \
		rm -f $(INSTALLDIR).$(TM)/mail; \
		ln -s $(NAME) $(INSTALLDIR).$(TM)/mail; \
	fi
