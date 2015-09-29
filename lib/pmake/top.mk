#
# This is a library Makefile that is included by the Makefiles for
# top-level directories.  This file just arranges for a variety of
# targets to be passed on to each of a collection of subdirectories.
# The file that includes this one should already have defined the
# following variables:
#	SUBDIRS		Subdirectories that contain "interesting" things
#			(e.g., individual commands, modules of the kernel,
#			of sub-portions of a large library).
#
# A bunch of variables are passed on to lower-level makes, if they are
# defined;  see the definitions immediately below for a complete list.
#	
# $Header: /sprite/lib/pmake/RCS/top.mk,v 1.22 91/12/13 13:30:49 jhh Exp $
#

#
# System programs -- assign conditionally so they may be redefined in
# including makefile
#
BINDIR		?= /sprite/cmds.$(MACHINE)

CAT		?= $(BINDIR)/cat
CP		?= $(BINDIR)/cp
MV		?= $(BINDIR)/mv
RM		?= $(BINDIR)/rm
TEST            ?= $(BINDIR)/test
UPDATE		?= $(BINDIR)/update

#
# Figure out what stuff we'll pass to sub-makes.
#
PASSVARS	=
#ifdef		CC
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

# MAKESUBDIRS usage:
#	<target> : MAKESUBDIRS
#
# This .USE target will simply pass <target> onto each subdirectory
# in a separate make.  If the TM variable is defined, then only pass
# the target on to subdirectories whose Makefiles include the given
# TM among their MACHINES.
#
MAKESUBDIRS	: .USE .MAKE .SILENT .NOEXPORT
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

default		: $(SUBDIRS) .NOEXPORT

#
# Here as with MAKESUBDIRS, if an explicit TM is given then only
# re-make in the subdirectories that support that particular target
# machine.
#
$(SUBDIRS)	:: .SILENT .NOEXPORT
#ifdef TM
	if grep '^MACHINES' $(.TARGET)/Makefile | grep -s $(TM); then
		(cd $(.TARGET); $(MAKE) $(PASSVARS))
	else true;
	fi
#else
	(cd $(.TARGET); $(MAKE) $(PASSVARS))
#endif

clean		:: .NOEXPORT
	rm -f *~

mkmf		! .MAKE .SILENT .NOEXPORT
	mkmf
	for i in $(SUBDIRS);
	do
		echo  %%% ${i} %%%
		(cd $i; $(MAKE) mkmf)
	done

newtm		! .MAKE .SILENT .NOEXPORT
	for i in $(SUBDIRS);
	do
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) newtm)
	done

rcsinfo		: .MAKE .SILENT .NOEXPORT
	for i in $(SUBDIRS);
	do
		echo %%% ${i} %%%
		(cd $i; rcsinfo)
	done

dist ::   subdirs_d
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk
	do
	if $(TEST) -e $${i};
	    then $(UPDATE)  $${i} $(DISTDIR)/$${i} ; else true; fi
	done
#endif

subdirs_d:
	for i in $(SUBDIRS)
	do
	    echo %%% $${i} %%%
#if defined(DISTDIR) && !empty(DISTDIR)
#ifdef TM
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} 'TM=$(TM)' )
#else
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} )
#endif
#else
#ifdef TM
	    ( cd $${i}; $(MAKE) dist 'TM=$(TM)' )
#else
	    ( cd $${i}; $(MAKE) dist )
#endif
#endif
	done

all clean tidy cleanall			:: MAKESUBDIRS
debug depend dependall install		:: MAKESUBDIRS
installhdrs installdebug installman	:: MAKESUBDIRS
installprofile lint profile rdist	:: MAKESUBDIRS
tags TAGS update snapshot		:: MAKESUBDIRS

.MAKEFLAGS	: -C		# No compatibility needed
