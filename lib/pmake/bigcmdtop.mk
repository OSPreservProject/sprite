#
# This is a library Makefile that is included by the Makefiles for
# the top-level directories of multi-directory libraries ("biglib"s).
# For most targets, this file just passes the targets on to each of
# the subdirectories.  The file that includes this one should already
# have defined the following variables:
#	LIBRARY		base name of library (e.g. tcl, sx, c, etc.)
#	LINTDIR		place to install lint library (we'll add a .mach
#			extension here)
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
# $Header: /sprite/lib/pmake/RCS/bigcmdtop.mk,v 1.38 91/11/06 18:34:52 kupfer Exp Locker: mottsmth $
#

OBJS		= $(SUBDIRS:S|$|/$(TM).md/linked.o|g)
POBJS		= $(SUBDIRS:S|$|/$(TM).md/linked.po|g)

#
# Suffix for profiled targets.
#
PROFSUFFIX	?= .pg

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
LOADFLAGS	+= -L$(HOME)/lib/$(TM).md
XCFLAGS		+= -I$(HOME)/lib/include
.PATH.h		: $(HOME)/lib/include
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
#ifdef		TM
PASSVARS	+= 'TM=$(TM)'
#endif

#
# Collect flags and other machine-dependent things for compilation.
#
#include	<tm.mk>
CTFLAGS		?= -wt
INSTALLFLAGS	?=
INSTALLMANFLAGS	?=
LINTFLAGS	?= -S -m$(TM)
XCFLAGS		?=
LOADFLAGS	?=
XAFLAGS		?=
CFLAGS		+= -g -O $(TMCFLAGS) $(XCFLAGS)

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

#
# MAKEINSTALL usage:
#	target : source MAKEINSTALL
#
# The source is installed at the target and backed-up to
# $(INSTALLDIR).$(TM).old
#
#ifndef NOBACKUP
BACKUP		= -b $(INSTALLDIR).$(TM).old
#ifdef BACKUPAGE
BACKUP += -B $(BACKUPAGE)
#endif
#else
BACKUP		=
#endif  NOBACKUP
#if !empty(TM:Mspur)
# use a separate install script that doesn't strip
# note that XLD has already been run
MAKEINSTALL	: .USE .SILENT
	$(UPDATE) -m 775 $(BACKUP) $(INSTALLFLAGS) $(.ALLSRC) $(.TARGET)
#else
MAKEINSTALL	: .USE .SILENT
	$(UPDATE) -m 775 -s $(BACKUP) $(INSTALLFLAGS) $(.ALLSRC) $(.TARGET)
#endif

#ifndef no_targets
#
# We should define the main targets.  See the Mkmf man page for details.
#

LIBS			?=

default			: $(TM).md/$(NAME)
$(TM).md/$(NAME)	: subdirs $(LIBS) $(OBJS) -lc
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -o $(.TARGET) $(LOADFLAGS) $(OBJS) $(LIBS)
#if !empty(TM:Mspur)
	$(XLD) $(.TARGET)
#endif

#
# make the linked.o files depend on the subdirectories so pmake won't 
# stat linked.o before remaking the subdirectories and think that the top-level
# target is up-to-date.  This generates N^2 dependencies, but there aren't
# typically many subdirectories so this should be okay, and all the
# subdirectories get remade anyway.
#
$(SUBDIRS:S|$|/$(TM).md/linked.o|g): $(SUBDIRS)

clean			:: .MAKE .SILENT tidy
	echo "rm -f $(TM).md/$(NAME)"
	rm -f $(TM).md/$(NAME)
	echo "rm -f $(TM).md/$(NAME)$(PROFSUFFIX)"
	rm -f $(TM).md/$(NAME)$(PROFSUFFIX)

tidy			:: .MAKE .SILENT .NOEXPORT
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

#
# For "install", a couple of tricks.  First, allow local.mk to disable
# by setting no_install.  Second, use :: instead of : so that local.mk
# can augment install with additional stuff.  Third, don't install if
# TMINSTALLDIR isn't set.
#
INSTCMD			= $(INSTALLDIR).$(TM)/$(NAME)
#ifndef no_install
#ifdef TMINSTALLDIR
install			:: $(INSTCMD) installman
$(INSTCMD)		: $(TM).md/$(NAME) MAKEINSTALL
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

lint			:: lintlib
lint			:: MAKESUBDIRS
lintlib			: $(TM).md/lintlib.ln
$(TM).md/lintlib.ln	: .MAKE .SILENT
	$(RM) -f $(.TARGET)
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
		$(CAT) ${i}/$(TM).md/llib-l${i}.ln >> $(.TARGET)
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

profile			: $(TM).md/$(NAME)$(PROFSUFFIX)
#if empty(TM:Mds3100) && empty(TM:Mds5000)
PROFFLAG = -pg
#else
PROFFLAG = -p
#endif
$(TM).md/$(NAME)$(PROFSUFFIX)	: subdirs_p $(POBJS) $(LIBS:S/.a$/_p.a/g) 
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(PROFFLAG) -o $(.TARGET) $(LOADFLAGS) $(POBJS) $(LIBS) 


#
# The following target does a quick make without cycling through to
# recompile in each of the subdirectories.
#
quick			: $(LIBS) -lc
	$(RM) -f $(TM).md/$(NAME)
	$(CC) $(CFLAGS) -o $(TM).md/$(NAME) $(LOADFLAGS) $(OBJS) $(LIBS)

rcsinfo			: .MAKE .SILENT
	for i in $(SUBDIRS);
	do
		echo %%% ${i} %%%
		(cd $i; rcsinfo)
	done

#
# The rules immediately below are a trick to get Pmake to remake in
# the subdirectories, but not to consider $(TM).md/$(NAME) to
# be out-of-date with respect to $(OBJS) unless $(OBJS) are actually
# more recent than $(TM).md/$(NAME).
#

$(OBJS)			: .DONTCARE
subdirs			: .MAKE .EXEC .SILENT .NOEXPORT
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS))
	done

$(POBJS)			: .DONTCARE
subdirs_p			: .MAKE .EXEC .SILENT .NOEXPORT
	for i in $(SUBDIRS);
	do
#ifdef TM
		if grep '^MACHINES' $i/Makefile | grep -s $(TM); then
			true;
		else continue;
		fi
#endif TM
		echo %%% ${i} %%%
		(cd $i; $(MAKE) $(PASSVARS) profile)
	done

depend mkmf tags 	:: MAKESUBDIRS

dist        !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk $(MANPAGES)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ;else true; fi
	done
	for i in $(SUBDIRS)
	do
	    echo %%% $${i} %%%
#ifdef TM
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} 'TM=$(TM)' )
#else
	    ( cd $${i}; $(MAKE) dist 'DISTDIR=$(DISTDIR)'/$${i} )
#endif
	done
#else
	@echo "Sorry, no distribution directory defined for $(NAME)."
#endif

#include <all.mk>

#endif no_targets

.MAKEFLAGS	: -C		# No compatibility needed
