#
# Included makefile for managing a directory containing only a shell script.
#
# $Header: /sprite/lib/pmake/RCS/script.mk,v 1.20 91/09/24 19:03:42 jhh Exp $ SPRITE (Berkeley)
#
# The makefile that includes this one should already have defined the
# following variables:
#	NAME		the shell script.
#       MACHINES	list of all target machines currently available
#			for this program.
#	TYPE		a keyword identifying which sort of command this
#			is;  used to determine where to install, etc.
#
# Optional variables that may be defined by the invoker:
#	INSTALLFLAGS	additional flags to pass to install script.
#

#
# System programs -- assign conditionally so they may be redefined in
# the including makefile
#
BINDIR		= /sprite/cmds.$(MACHINE)

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
INSTALLDIR	?= /sprite/cmds
INSTALLMAN	?= /sprite/man/cmds
#elif !empty(TYPE:Mx)
INSTALLDIR	?= /X/cmds
INSTALLMAN	?= /X/man/cmds
#elif !empty(TYPE:MX11R3)
INSTALLDIR	?= /mic/X11R3/cmds
INSTALLMAN	?= /mic/X11R3/man/cmds
#elif !empty(TYPE:MX11R4)
INSTALLDIR	?= /X11/R4/cmds
INSTALLMAN	?= /X11/R4/man/cmds
#elif !empty(TYPE:Mdaemon)
INSTALLDIR	?= /sprite/daemons
INSTALLMAN	?= /sprite/man/daemons
#elif !empty(TYPE:Madmin)
INSTALLDIR	?= /sprite/admin
INSTALLMAN	?= /sprite/man/admin
#elif !empty(TYPE:Mpersonal)
INSTALLDIR	?= $(HOME)/cmds
INSTALLMAN	?= $(HOME)/man/cmds
#elif !empty(TYPE:Mlocal)
INSTALLDIR	?= /local/cmds
INSTALLMAN	?= /local/man/cmds
#ifndef		USERBACKUP
NOBACKUP	=
#endif
#endif
#ifdef INSTALLDIR
TMINSTALLDIR	?= $(INSTALLDIR).$(TM)
#endif

#
# Figure out what stuff we'll pass to sub-makes.
#
PASSVARS	= 'INSTALLDIR=$(INSTALLDIR)' $(.MAKEFLAGS)

#
# Set up flags for various utility programs.
#

#include	<tm.mk>
INSTALLFLAGS	?=
INSTALLMANFLAGS	?=

#
# MAKEINSTALL usage:
#	<target> : MAKEINSTALL
# All of the shell scripts get updated to INSTALLDIR.
#
MAKEINSTALL	: .USE .SILENT
#if !empty(NAME)
	$(UPDATE) -l -m 555 $(INSTALLFLAGS) $(NAME) $(TMINSTALLDIR)/$(NAME)
#endif

#ifndef no_targets

default			:: $(NAME)

clean tidy cleanall	::
	$(RM) -f *~

#
# For "install", a couple of tricks.  First, allow local.mk to disable
# by setting no_install.  Second, use :: instead of : so that local.mk
# can augment install with additional stuff.  Third, issue a warning
# message if TMINSTALLDIR hasn't been defined.
#
#ifndef no_install
#ifdef TMINSTALLDIR
install 		:: installman MAKEINSTALL
#else
install			:: .SILENT
	echo "Can't install $(NAME):  no install directory defined"
#endif TMINSTALLDIR
#endif no_install

#if empty(MANPAGES)
installman		:: .SILENT
	echo "There's no man page for $(NAME).  Please write one."
#elif !empty(MANPAGES:MNONE)
installman		::
#elif defined(INSTALLMAN)
installman		:: .SILENT
	$(UPDATE) -m 444 -l $(INSTALLMANFLAGS) $(MANPAGES) $(INSTALLMAN)
#else
installman		:: .SILENT
	echo "Can't install man page(s): no install directory defined"
#endif

mkmf newtm		! .SILENT
	mkmf

#
# Most targets get ignored completely for scripts, since there's
# nothing to do.
#

depend lint tags	:: # says nothing, does nothing

dist        !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk $(TM).md/md.mk $(MANPAGES) $(NAME)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ; else true; fi
	done
#else
	@echo "Sorry, no distribution directory defined."
#endif

#include		<all.mk>

#endif no_targets

.MAKEFLAGS	: -C		# No compatibility needed

#include		<rdist.mk>
