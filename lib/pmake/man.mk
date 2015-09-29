#
# Included makefile for managing a directory containing only public
# header file sources.
#
# $Header: /sprite/lib/pmake/RCS/man.mk,v 1.6 90/02/20 11:50:10 douglis Exp $ SPRITE (Berkeley)
#
# The makefile that includes this one should already have defined the
# following variables:
#	INCLUDEDIR	place to install public header files
#	MANPAGES	list of all manual pages in this directory.
#	NAME		name of this subdirectory within the manual area.
#	TYPE		a keyword identifying which portion of the
#			system these man pages belong to ("sprite" for
#			the Sprite system code, "x" for X-related
#			programs, etc.).
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
TEST            ?= $(BINDIR)/test
UPDATE		?= $(BINDIR)/update

#
# Some variables (such as where to install) are set based on the
# TYPE variable.  Of course, any of these variables can be overridden
# by explicit assignments.
#
TYPE		?= unknown
#if !empty(TYPE:Msprite)
INSTALLMAN	?= /sprite/man/$(NAME)
#elif !empty(TYPE:Mx)
INSTALLMAN	?= /X/man/$(NAME)
#endif

INSTALLFLAGS	?=
INSTALLMANFLAGS	?=

.MAKEFLAGS	: -C

#ifndef no_targets

default			: # Says nothing, does nothing.

clean			!
	$(RM) -f *~

install			:: installman
#if empty(MANPAGES)
installman		:: .SILENT
	echo "There are no man pages for $(NAME).  Please write some."
#elif defined(INSTALLMAN)
installman		:: .SILENT $(MANPAGES)
	$(UPDATE) -m 444 -l $(INSTALLMANFLAGS) $(MANPAGES) $(INSTALLMAN)
#else
installman		:: .SILENT
	echo "Can't install man page(s): no install directory defined"
#endif

mkmf			:: .SILENT
	mkmf

newtm		 	: # Says nothing, does nothing.

dist        !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk $(MANPAGES)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ;else true; fi
	done
#endif

#endif no_targets
