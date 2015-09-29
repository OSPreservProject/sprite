#
# Makefile for commands.  This is a library Makefile that is included
# by the Makefile's for individual commands.  The file that includes
# this one should already have defined the following variables:
#	NAME		name of program to be created
#	SRCS		all source files, used for linting and making
#			dependencies
#	OBJS		object files from which to create it
#	CLEANOBJS	object files to be removed as part of "make clean"
#			(need not just be object files)
#	TM		target machine type for object files etc.
#	MACHINES	list of all target machines currently available
#			for this program
#	TYPE		a keyword identifying which sort of command this
#			is;  used to determine where to install, etc.
#
# Optional variables that may be defined by the invoker:
#	XAFLAGS		additional flags to pass to assembler
#	XCFLAGS		additional flags to pass to linker
#	DEPFLAGS	additional flags to pass to makedepend
#	no_targets	if defined, this file will not define all of the
#			basic targets (make, make clean, etc.)
#	use_version	if defined, then this file will set things up
#			to include a version number that is automatically
#			incremented
#
# $Header: /sprite/lib/pmake/RCS/command.mk,v 1.68 92/04/13 18:33:52 elm Exp Locker: mottsmth $
#

#
# The variables below should be defined in md.mk, but they are given
# default values just in case md.mk doesn't exist yet.
#
HDRS		?=
OBJS		?=
SRCS		?=

#
# First define search paths for libraries, include files, lint libraries,
# and even sources.
#
.PATH.h		:
.PATH.h		: $(TM).md . /sprite/lib/include /sprite/lib/include/$(TM).md
.PATH.ln	: /sprite/lib/lint
.PATH.c		:
.PATH.c		: $(TM).md
.PATH.s		:
.PATH.s		: $(TM).md

#
# Suffix for profiled targets.
#
PROFSUFFIX	?= .pg

#
# Important directories. 
#
MISCLIBDIR	?= /sprite/lib/misc
BINDIR		?= /sprite/cmds.$(MACHINE)

#
# System programs -- assign conditionally so they may be redefined in
# including makefile.  These need to be defined absolutely so that we
# can remake these programs without flakey new versions accidentally
# getting used to make themselves
#
AS		?= $(BINDIR)/as
CC		?= $(BINDIR)/cc
CPLUSPLUS	?= $(BINDIR)/g++
CP		?= $(BINDIR)/cp
CPP		?= $(BINDIR)/cpp -traditional -$
CTAGS		?= $(BINDIR)/ctags
ETAGS		?= /emacs/cmds/etags
ECHO		?= $(BINDIR)/echo
LINT		?= $(BINDIR)/lint
MAKEDEPEND	?= $(BINDIR)/makedepend
MKVERSION	?= $(BINDIR)/mkversion
MV		?= $(BINDIR)/mv
RM		?= $(BINDIR)/rm
SED		?= $(BINDIR)/sed
TEST            ?= $(BINDIR)/test
TOUCH		?= $(BINDIR)/touch
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
#elif !empty(TYPE:Mlocal)
INSTALLDIR	?= /local/cmds
INSTALLMAN	?= /local/man/cmds
#elif !empty(TYPE:MX11R4)
INSTALLDIR	?= /X11/R4/cmds
INSTALLMAN	?= /X11/R4/man/cmds
#elif !empty(TYPE:Memacs)
INSTALLDIR	?= /emacs/cmds
INSTALLMAN	?= /emacs/man/cmds
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
PASSVARS	= 'INSTALLDIR=$(INSTALLDIR)' 'TM=$(TM)' $(.MAKEFLAGS)
#ifdef		XCFLAGS
PASSVARS	+= 'XCFLAGS=$(XCFLAGS)'
#endif
#ifdef		XAFLAGS
PASSVARS	+= 'XAFLAGS=$(XAFLAGS)'
#endif
#ifdef		NOBACKUP
PASSVARS	+= 'NOBACKUP=$(NOBACKUP)'
#endif
#ifdef		BACKUPAGE
PASSVARS	+= 'BACKUPAGE=$(BACKUPAGE)'
#endif

#
# Flags. These are ones that are needed by *all* programs. Any other
# ones should be added with the += operator in the command-specific makefile.
# The FLAGS variables are defined with the += operator in case this file
# is included after the main makefile has already defined them...
#

#include	<tm.mk>
CTFLAGS		?= -wtd
INSTALLFLAGS	?=
INSTALLMANFLAGS	?=
LINTFLAGS	?= -m$(TM)
XCFLAGS		?=
LOADFLAGS	?=
XAFLAGS		?=
#
# The .INCLUDES variable already includes directories that should be
# used by cc and other programs by default.  Remove them, just so that
# the output looks cleaner.
#
# The dec compiler doesn't include /sprite/lib/include, so we must leave
# the path as is when TM=ds3100.  
#

#include <debugflags.mk>

#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
CFLAGS		+= $(GFLAG) $(OFLAG) $(TMCFLAGS) $(XCFLAGS) -I.
#elif !empty(TM:Mspur)
CFLAGS		+= $(GFLAG) $(TMCFLAGS) $(XCFLAGS) -I.
#else
CFLAGS		+= $(GFLAG) $(OFLAG) $(TMCFLAGS) $(XCFLAGS) -I.
#endif
CFLAGS		+= $(.INCLUDES:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g)
#if empty(TM:Mds3100) && empty(TM:Mds5000)
AFLAGS		+= $(TMAFLAGS) $(XAFLAGS)
#else
CFLAGS		+= -I/sprite/lib/include -I/sprite/lib/include/$(TM).md
AFLAGS		+= $(.INCLUDES)
#endif

#
# Transformation rules: these have special features to place .o files
# in md subdirectories, run preprocessor over .s files, and generate
# .po files for profiling.
#

.SUFFIXES	: .po

.c.o		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -c $(.IMPSRC) -o $(.TARGET)
.c.po		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
.cc.o		:
	$(RM) -f $(.TARGET)
	$(CPLUSPLUS) $(CFLAGS) -c $(.IMPSRC) -o $(.TARGET)
.cc.po		:
	$(RM) -f $(.TARGET)
	$(CPLUSPLUS) $(CFLAGS) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
.s.po .s.o	:
#if empty(TM:Mds3100) && empty(TM:Mds5000)
	$(CPP) $(CFLAGS:M-[IDU]*) -m$(TM) -D$(TM) -D_ASM $(.IMPSRC) > $(.PREFIX).pp
	$(AS) -o $(.TARGET) $(AFLAGS) $(.PREFIX).pp
	$(RM) -f $(.PREFIX).pp
#else
	$(RM) -f $(.TARGET)
	$(AS) $(AFLAGS) $(.IMPSRC) -o $(.TARGET)
#endif

#
# The following targets are .USE rules for creating things.
#

#
# MAKECMD usage:
#	program : <objects> <libraries> MAKECMD
#
# Creates and links in the file version.o as well. Also makes program depend
# on the state of the C library.
#
# Using .ALLSRC constrains the local.mk, in that additions to LIBS
# must come before the system makefile(s).  The advantage (or at least
# an advantage) is that it filters out duplicate .o files that would
# appear in the OBJS list.  These duplicates can appear if the command
# generates source files on-the-fly: the local.mk typically has a
# line like OBJS += foo.o, and if mkmf is run after foo.c is created,
# foo.o will appear twice.  Also, using .ALLSRC lets pmake generate
# a command based on knowledge of the search path, rather than
# relying on the particular tool (e.g., cc or ld) to do the search
# path processing.
#
MAKECMD		: .USE -lc
	@echo "Generating date stamp"
	@$(RM) -f version.h
	@$(MKVERSION) > version.h
	$(RM) -f $(TM).md/version.o
	$(CC) $(CFLAGS) -c -o $(TM).md/version.o $(MISCLIBDIR)/version.c
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -o $(.TARGET) $(LOADFLAGS) $(TM).md/version.o $(.ALLSRC:N-lc) 
#if !empty(TM:Mspur) && empty(MACHINE:Mspur)
	$(XLD) $(.TARGET)
#endif

#
# MAKECMDNOVERS usage:
#	<program> : <objects> <libraries> MAKECMDNOVERS
#
# Similar to MAKECMD, except it doesn't create the version.[ho] files.
#
MAKECMDNOVERS	:  .USE -lc
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -o $(.TARGET) $(LOADFLAGS) $(.ALLSRC:N-lc)
#if !empty(TM:Mspur) && empty(MACHINE:Mspur)
	$(XLD) $(.TARGET)
#endif

#
# MAKEINSTALL usage:
#	install :: <dependencies> MAKEINSTALL
#
# The program is installed in $(TMINSTALLDIR) and backed-up to
# $(TMINSTALLDIR).old
#
#ifndef NOBACKUP
BACKUP		= -b $(TMINSTALLDIR).old
#ifdef BACKUPAGE
BACKUP += -B $(BACKUPAGE)
#endif
#else
BACKUP		=
#endif  NOBACKUP

#if !empty(TM:Mspur)
# use a separate install script that doesn't strip
# note that XLD has already been run
MAKEINSTALL	: .USE
	$(UPDATE) -m 775 $(BACKUP) $(INSTALLFLAGS) $(TM).md/$(NAME) \
		$(TMINSTALLDIR)/$(NAME)
#else
MAKEINSTALL	: .USE
	$(UPDATE) -m 775 -s $(BACKUP) $(INSTALLFLAGS) $(TM).md/$(NAME) \
		$(TMINSTALLDIR)/$(NAME)
#endif

#
# MAKELINT usage:
#	<fluff-file> : <sources to be linted> MAKELINT
#
# <fluff-file> is the place to store the output from the lint.
#
MAKELINT	: .USE
	$(RM) -f $(.TARGET)
	$(LINT) $(LINTFLAGS) $(CFLAGS:M-[IDU]*) $(.ALLSRC) > $(.TARGET) 2>&1

#
# MAKEDEPEND usage:
#	<dependency-file> : <sources> MAKEDEPEND
#
# Generate dependency file suitable for inclusion in future makes.

MAKEDEPEND	: .USE
	@$(TOUCH) $(DEPFILE)
#if empty(TM:Mds3100) && empty(TM:Mds5000)
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*) -m $(TM) -w60 -f $(DEPFILE) $(.ALLSRC)
#else
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g) -m $(TM) -w60 -f $(DEPFILE) $(.ALLSRC)
#endif
	@$(MV) -f $(DEPFILE) $(DEPFILE).tmp
	@$(SED) -e '/^#/!s|^.|$(TM).md/&|' <$(DEPFILE).tmp > $(DEPFILE)
	@$(RM) -f $(DEPFILE).tmp

#if !defined(no_targets) && defined(NAME)
#
# We should define the main targets (make, make install, etc.).  See the
# mkmf man page for details on what these do.
#
LIBS			?=

default			:: $(TM).md/$(NAME)
#if defined(use_version)
$(TM).md/$(NAME)	: $(OBJS) $(LIBS) MAKECMD
#else
$(TM).md/$(NAME)	: $(OBJS) $(LIBS) MAKECMDNOVERS
#endif


clean			:: .NOEXPORT tidy 
	$(RM) -f $(TM).md/$(NAME) $(TM).md/$(NAME)$(PROFSUFFIX)

tidy			:: .NOEXPORT
#if defined(CLEANOBJS) && !empty(CLEANOBJS)
	$(RM) -f $(CLEANOBJS) $(CLEANOBJS:M*.o:S/.o$/.po/g)
#endif
	$(RM) -f y.tab.c lex.yy.c core \
	        $(TM).md/lint \
		a.out *~ $(TM).md/*~ gmon.out mon.out

DEPFILE = $(TM).md/dependencies.mk

depend			: $(DEPFILE)
$(DEPFILE)		! $(SRCS:M*.c) $(SRCS:M*.s) $(SRCS:M*.cc) MAKEDEPEND


#
# For "install", a couple of tricks.  First, allow local.mk to disable
# by setting no_install.  Second, use :: instead of : so that local.mk
# can augment install with additional stuff.  Third, don't install if
# TMINSTALLDIR isn't set.
#
#ifndef no_install
#ifdef TMINSTALLDIR
install			:: $(TM).md/$(NAME) installman MAKEINSTALL
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
installman		:: .SILENT $(MANPAGES)
	$(UPDATE) -m 444 -l $(INSTALLMANFLAGS) $(MANPAGES) $(INSTALLMAN)
#else
installman		:: .SILENT
	echo "Can't install man page(s): no install directory defined"
#endif


lint			: $(TM).md/lint
$(TM).md/lint		: $(SRCS:M*.c) $(LIBS:M-l*) MAKELINT


mkmf			:: .SILENT
	mkmf


newtm			:: .SILENT
	if $(TEST) -d $(TM).md; then
	    true
	else
	    mkdir $(TM).md;
	    chmod 775 $(TM).md;
	    mkmf -m$(TM)
	fi


profile			: $(TM).md/$(NAME)$(PROFSUFFIX)
#if empty(TM:Mds3100) && empty(TM:Mds5000)
PROFFLAG = -pg
#else
PROFFLAG = -p
#endif
$(TM).md/$(NAME)$(PROFSUFFIX)	: $(OBJS:S/.o$/.po/g) $(LIBS:S/.a$/_p.a/g)
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(PROFFLAG) -o $(.TARGET) $(.ALLSRC)


tags			:: $(SRCS:M*.c) $(HDRS)
	$(CTAGS) $(CTFLAGS) $(SRCS:M*.c) $(HDRS)

TAGS			:: $(SRCS:M*.[ch]) $(HDRS)
	$(ETAGS) $(SRCS:M*.[ch])

version.h		:
	$(RM) -f version.h
	$(MKVERSION) > version.h

DISTFILES    ?=

dist        !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk sprite dist \
	    $(TM).md/md.mk $(SRCS) $(HDRS) $(DISTFILES)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ;else true; fi
	done
#endif

#include	<all.mk>

#endif no_targets && NAME

.MAKEFLAGS	: -C		# No compatibility needed

#include	<rdist.mk>
