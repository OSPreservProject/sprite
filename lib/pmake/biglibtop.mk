#
# This is a library Makefile that is included by the Makefiles for
# the top-level directories of multi-directory libraries ("biglib"s).
# For most targets, this file just passes the targets on to each of
# the subdirectories.  The file that includes this one should already
# have defined the following variables:
#	INSTALLDIR	generic place to install archive (we'll add a .md
#			subdirectory specifier here)
#	LINTDIR		place to install lint library (we'll add a .mach
#			extension here)
#	NAME		base name of library (e.g. tcl, sx, c, etc.)
#	SUBDIRS		subdirectories that contain "interesting" things
#			(e.g., individual commands, modules of the kernel,
#			of sub-portions of a large library).
#	TM		target machine type for object files etc.
#	TYPE		a keyword identifying which sort of command this
#			is;  used to determine where to install, etc.
#
# A bunch of variables are passed on to lower-level makes, if they are
# defined;  see the definitions immediately below for a complete list.
#	
# $Header: /sprite/lib/pmake/RCS/biglibtop.mk,v 1.26 90/11/02 08:43:08 mendel Exp $
#

#
# Set up search paths.
#
.PATH.a		: # Clear out;  otherwise Pmake uses the installed libraries
		  # where it should be using uninstalled ones.

#
# System programs -- assign conditionally so they may be redefined in
# including makefile
#
BINDIR		?= /sprite/cmds.$(MACHINE)

CAT		?= $(BINDIR)/cat
CP		?= $(BINDIR)/cp
MV		?= $(BINDIR)/mv
RANLIB		?= $(BINDIR)/ranlib
RM		?= $(BINDIR)/rm
TEST            ?= $(BINDIR)/test
UPDATE		?= $(BINDIR)/update

#
# Several variables (such as where to install) are set based on the
# TYPE variable.  Of course, any of these variables can be overridden
# by explicit assignments.
#
TYPE		?= unknown
#if !empty(TYPE:Msprite)
INSTALLDIR	?= /sprite/lib
INSTALLMAN	?= /sprite/man/lib/$(NAME)
LINTDIR		?= /sprite/lib/lint
#elif !empty(TYPE:Mx)
INSTALLDIR	?= /X/lib
INSTALLMAN	?= /X/man/lib/$(NAME)
LINTDIR		?= /X/lib/lint
#elif !empty(TYPE:MX11R4)
INSTALLDIR	?= /X11/R4/lib
INSTALLMAN	?= /X11/R4/man/lib/$(NAME)
LINTDIR		?= /X11/R4/lib/lint
#else
INSTALLDIR	?=
INSTALLMAN	?=
LINTDIR		?=
#endif

#
# Figure out what stuff we'll pass to sub-makes.
#
PASSVARS	=
#if		defined(CC) && empty(CC:Mcc)
PASSVARS	+= 'CC=$(CC)'
#endif
#ifdef		XCFLAGS
PASSVARS	+= 'XCFLAGS=$(XCFLAGS)'
#endif
#ifdef		XAFLAGS
PASSVARS	+= 'XAFLAGS=$(XAFLAGS)'
#endif
#ifdef		INCLUDEDIR
PASSVARS	+= 'INCLUDEDIR=$(INCLUDEDIR)'
#endif
#ifdef		INSTALLDIR
PASSVARS	+= 'INSTALLDIR=$(INSTALLDIR)'
#endif
#ifdef		NOBACKUP
PASSVARS	+= 'NOBACKUP=$(NOBACKUP)'
#endif
#ifdef		BACKUPAGE
PASSVARS	+= 'BACKUPAGE=$(BACKUPAGE)'
#endif
#ifdef		TM
PASSVARS	+= 'TM=$(TM)'
#endif

#
# The following include is so that RANLIB can get re-set depending
# on the machine type.
#
#include	<tm.mk>
INSTALLFLAGS	?=
INSTALLMANFLAGS	?=

# MAKESUBDIRS usage:
#	<target> : MAKESUBDIRS
#
# This .USE target will simply pass <target> onto each subdirectory
# in a separate make.If the TM variable is defined, then only pass
# the target on to subdirectories whose Makefiles include the given
# TM among their MACHINES.
#
MAKESUBDIRS	: .USE .MAKE .EXEC .SILENT .NOEXPORT
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) $(.TARGET))
	done

#
# MAKEINSTALLLIB usage:
#	<target> : <source> MAKEINSTALLLIB
# Will copy over a library and re-ranlib it.
#
MAKEINSTALLLIB : .USE
	$(RM) -f $(.TARGET).old $(.TARGET).new~
	$(CP) $(.ALLSRC) $(.TARGET).new~
	$(RANLIB) $(.TARGET).new~
	- $(MV) $(.TARGET) $(.TARGET).old
	$(MV) $(.TARGET).new~ $(.TARGET)

REGLIB			= $(TM).md/lib$(NAME).a
PROFLIB			= $(TM).md/lib$(NAME)_p.a
DEBUGLIB		= $(TM).md/lib$(NAME)_g.a
LINTLIB			= $(TM).md/llib-l$(NAME).ln
INSTLIB			= $(INSTALLDIR)/$(REGLIB)
INSTPROFILE		= $(INSTALLDIR)/$(PROFLIB)
INSTDEBUG		= $(INSTALLDIR)/$(DEBUGLIB)
LINTLIB			= $(TM).md/llib-l$(NAME).ln
INSTLINT		= $(LINTDIR).$(TM)/llib-l$(NAME).ln

#ifndef no_targets
#
# We should define the main targets.  See the Mkmf man page for details.
#

default		: $(REGLIB)

#
# Here as with MAKESUBDIRS, if an explicit TM is given then only
# re-make in the subdirectories that support that target machine.
#
$(REGLIB)	: $(SUBDIRS)	
$(SUBDIRS)	:: .MAKE .EXEC .SILENT .NOEXPORT
#ifdef TM
	if grep '^MACHINES' $(.TARGET)/Makefile | grep -s $(TM); then
		cd $(.TARGET); $(MAKE) $(PASSVARS)
	else true;
	fi
#else
	cd $(.TARGET); $(MAKE) $(PASSVARS)
#endif

$(PROFLIB)	: profile
$(DEBUGLIB)	: debug

clean tidy		:: .MAKE .SILENT
	echo "rm -f $(REGLIB) $(PROFLIB) $(DEBUGLIB)"
	rm -f $(REGLIB) $(PROFLIB) $(DEBUGLIB)
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) clean)
	done

#if empty(TYPE:Munknown)
#
# The install target is handled specially, in order to avoid multiple
# passes through the subdirectories for compiling, generating lint
# libraries, installing headers, and so on.
#
install			:: .MAKE .EXEC .NOEXPORT
	$(RM) -f $(TM).md/llib-l$(NAME).ln
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) install)
		$(CAT) ${i}/$(TM).md/llib-l${i}.ln >> $(LINTLIB)
	done
	$(RM) -f $(INSTLIB).old $(INSTLIB).new~
	$(CP) $(REGLIB) $(INSTLIB).new~
	$(RANLIB) $(INSTLIB).new~
	- $(MV) $(INSTLIB) $(INSTLIB).old
	$(MV) $(INSTLIB).new~ $(INSTLIB)
	$(UPDATE) -m 664 $(INSTALLFLAGS) $(LINTLIB) $(INSTLINT)

#
# Rebuild is like install, but it nukes the library first and starts
# from scratch, telling the subdirs to append and not to ranlib.
#
rebuild			:: .MAKE .EXEC .NOEXPORT
	$(RM) -f $(REGLIB)
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) rebuild)
	done
	ranlib $(REGLIB)

$(INSTLIB)		: $(REGLIB) MAKEINSTALLLIB
installdebug		:: $(DEBUGLIB) debuglink
debuglink		!
	rm -f $(INSTDEBUG)
	ln -s `pwd`/$(DEBUGLIB) $(INSTDEBUG)
installlint		:: $(INSTLINT)
$(INSTLINT)		:: $(LINTLIB) .SILENT
	$(UPDATE) -m 664 $(INSTALLFLAGS) $(LINTLIB) $(INSTLINT)
installman		:: .SILENT
#if !empty(MANPAGES)
	$(UPDATE) -m 444 -l $(INSTALLMANFLAGS) $(MANPAGES) $(INSTALLMAN)
#endif
installprofile		:: $(INSTPROFILE)
$(INSTPROFILE)		: $(PROFLIB) MAKEINSTALLLIB

#
# The following target does a quick install without cycling through to
# recompile in each of the subdirectories.  Can't use the MAKEINSTALLLIB
# macro for this, unfortunately.
#
installquick		::
	$(RM) -f $(INSTLIB).old $(INSTLIB).new~
	$(CP) $(REGLIB) $(INSTLIB).new~
	$(RANLIB) $(INSTLIB).new~
	- $(MV) $(INSTLIB) $(INSTLIB).old
	$(MV) $(INSTLIB).new~ $(INSTLIB)

#else
#
# The targets below are used if there's no known place to install this
# library:  just output a warning message.
#
install installdebug installlint installman installprofile \
installquick		:: .SILENT
	echo "Can't install library $(NAME): no install directory defined"
#endif


lintlib			: $(LINTLIB)
$(LINTLIB)		: .MAKE .EXEC .SILENT
	$(RM) -f $(TM).md/llib-l$(NAME).ln
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) lintlib)
		$(CAT) ${i}/$(TM).md/llib-l${i}.ln >> $(LINTLIB)
	done

mkmf			::
	mkmf

newtm			! .MAKE .SILENT
	if $(TEST) -d $(TM).md; then
	    true
	else
	    mkdir $(TM).md;
	    chmod 775 $(TM).md;
	    mkmf -m$(TM)
	fi
	for i in $(SUBDIRS);
	do
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) newtm)
	done

rcsinfo			: .MAKE .SILENT
	for i in $(SUBDIRS);
	do
		echo %%% ${i} %%%
		(cd $i; rcsinfo)
	done

debug depend installdebug 		:: MAKESUBDIRS
installhdrs installman installprofile	:: MAKESUBDIRS
lint mkmf profile rdist			:: MAKESUBDIRS

dist: subdirs_d
#if defined(DISTDIR) && !empty(DISTDIR)
	if $(TEST) -e $(DISTDIR)/$(TM).md ;then
	    echo ""
	else
	    mkdir $(DISTDIR)/$(TM).md
	fi
	for i in Makefile local.mk
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ;else true; fi
	done
#endif

subdirs_d:
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in $(SUBDIRS)
	do
	    echo %%% $${i} %%%
#ifdef TM
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} 'TM=$(TM)' )
#else
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} )
#endif
	done
#if !empty(MANPAGES)
	$(UPDATE)  $(MANPAGES) $(DISTDIR)/$(MANPAGES)
#endif
#else
	@echo "Sorry, no distribution directory defined."
#endif

#include <all.mk>

#endif no_targets

.MAKEFLAGS	: -C		# No compatibility needed
