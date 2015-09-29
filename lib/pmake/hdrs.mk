#
# Included makefile for managing a directory containing only public
# header file sources.
#
# $Header: /sprite/lib/pmake/RCS/hdrs.mk,v 1.10 90/02/20 11:50:09 douglis Exp $ SPRITE (Berkeley)
#
# The makefile that includes this one should already have defined the
# following variables:
#	HDRS		all header files (all of which are public)
#	INCLUDEDIR	place to install public header files
#	SUBDIRS		list of subdirectories that contain additional
#			related header files (used in "make mkmf").
#
# Optional variables that may be defined by the invoker:
#	INSTALLFLAGS	additional flags to pass to install scripts.
#

#
# System programs -- assign conditionally so they may be redefined in
# including makefile
#
BINDIR		= /sprite/cmds.$(MACHINE)

CHGRP		?= $(BINDIR)/chgrp
CHMOD		?= $(BINDIR)/chmod
CHOWN		?= $(BINDIR)/chown
CP		?= $(BINDIR)/cp
ECHO		?= $(BINDIR)/echo
MV		?= $(BINDIR)/mv
RM		?= $(BINDIR)/rm
UPDATE		?= $(BINDIR)/update

INSTALLFLAGS	?=

.MAKEFLAGS	: -C

#
# MAKEINSTALLHDRS usage:
#	<target> : MAKEINSTALLHDRS
# All of the public headers files get updated to INCLUDEDIR.
#
MAKEINSTALLHDRS	: .USE .SILENT
#if !empty(HDRS)
	$(UPDATE) -l -m 444 -t $(INSTALLFLAGS) $(HDRS) $(INCLUDEDIR)
#endif

#ifndef no_targets

default			: # Says nothing, does nothing. MAKEINSTALLHDRS

clean			!
	$(RM) -f *~

install installhdrs	:: MAKEINSTALLHDRS

installworld		:: install .MAKE .SILENT
#if !empty(SUBDIRS)
	for i in $(SUBDIRS); do
		echo %%% ${i} %%%
		(cd $i; pmake $(.TARGET))
	done
#endif

mkmf			: .MAKE .SILENT
#if !empty(SUBDIRS)
	for i in $(SUBDIRS); do
		echo  %%% ${i} %%%
		(cd $i; mkmf)
	done
#endif

dist                    : subdirs_d
#if !empty(HDRS) && defined(DISTDIR) && !empty(DISTDIR)
	for i in $(HDRS); do
	    $(UPDATE) $${i} $(DISTDIR)/$${i}
	done
#endif

subdirs_d:
#if !empty(SUBDIRS) && defined(DISTDIR) && !empty(DISTDIR)
	for i in $(SUBDIRS); do
	    echo %%% $${i} %%%
#ifdef TM
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} 'TM=$(TM)' )
#else
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} )
#endif
	done
#endif

newtm			: # Says nothing, does nothing.

#endif no_targets

#include		<rdist.mk>
