#
# Included makefile for managing libraries.  This makefile is used as
# an include files in large libraries (like the C library) that consist
# of several source subdirectories, each potentially with machine-
# dependent subdirectories.
#
# $Header: /sprite/lib/pmake/RCS/biglib.mk,v 1.70 92/04/13 18:33:47 elm Exp $ SPRITE (Berkeley)
#
# The makefile that includes this one should already have defined the
# following variables:
#	NAME		base name of library (e.g. tcl, sx, c, etc.).  Since
#			this directory only has a piece of the library, NAME
#			is the name of the parent directory.
#	SUBDIR		the name of this directory
#	SRCS		all sources for library for the current target
#			machine
#	ALLSRCS		all sources for the library, including all sources
#			for all target machines
#	HDRS		all header files (public and private)
#	OBJS		object files from which to create it
#	CLEANOBJS	object files to be removed as part of "make clean"
#			(need not just be object files)
#	PUBHDRS		publicly-available headers for the library (this
#			contains only machine-independent headers)
#	MDPUBHDRS	machine-dependent public headers for the library
#			(for the current TM)
#	LINTSRCS	additional sources to be used only when generating
#			lint library
#	TM		target machine type for object files etc.
#	MACHINES	list of all target machines currently available
#			for this program
#	TYPE		a keyword identifying which sort of library this
#			is;  used to determine where to install, etc.
#
# Optional variables that may be defined by the invoker:
#	XAFLAGS		additional flags to pass to assembler
#	XCFLAGS		additional flags to pass to compiler
#	DEPFLAGS	additional flags to pass to makedepend
#	no_targets	if defined, this file will not define all of the
#			basic targets (make, make clean, etc.)
#
#

#
# The variables below should be defined in md.mk, but they are given
# default values just in case md.mk doesn't exist yet.
#
CLEANOBJS	?=
HDRS		?=
MANPAGES	?=
MDPUBHDRS	?=
OBJS		?=
SRCS		?=

#
# Define search paths for include files and source files (for sources,
# must be sure to look both in this directory and in the machine-dependent
# one).
#
.PATH.a		: # Clear out previous, or Pmake will not look in the
		  # right place for files like $(REGLIB)!!  This is
		  # a gross bug in Pmake.
.PATH.h		: # Clear out previous
.PATH.h		: $(TM).md /sprite/lib/include /sprite/lib/include/$(TM).md
.PATH.c		: # Clear out previous
.PATH.c		: $(TM).md
.PATH.s		: # Clear out previous
.PATH.s		: $(TM).md

#
# System programs -- assign conditionally so they may be redefined in
# including makefile
#
BINDIR		= /sprite/cmds.$(MACHINE)

AS		?= $(BINDIR)/as
CC		?= $(BINDIR)/cc
CHGRP		?= $(BINDIR)/chgrp
CHMOD		?= $(BINDIR)/chmod
CHOWN		?= $(BINDIR)/chown
CP		?= $(BINDIR)/cp
CPP		?= $(BINDIR)/cpp -traditional -$
CTAGS		?= $(BINDIR)/ctags
ECHO		?= $(BINDIR)/echo
LINT		?= $(BINDIR)/lint
MAKEDEPEND	?= $(BINDIR)/makedepend
MV		?= $(BINDIR)/mv
RANLIB		?= $(BINDIR)/ranlib
RM		?= $(BINDIR)/rm
SED		?= $(BINDIR)/sed
TEST            ?= $(BINDIR)/test
TOUCH		?= $(BINDIR)/touch
UPDATE		?= $(BINDIR)/update

# The Ultrix ar doesn't handle truncated file names correctly.

#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
AR = $(BINDIR)/ar.sprite
#else
AR = $(BINDIR)/ar
#endif

#
# Several variables (such as where to install) are set based on the
# TYPE variable.  Of course, any of these variables can be overridden
# by explicit assignments.
#
TYPE		?= unknown
#if !empty(TYPE:Msprite)
INCLUDEDIR	?= /sprite/lib/include
INSTALLMAN	?= /sprite/man/lib/$(NAME)
#elif !empty(TYPE:Mx)
INCLUDEDIR	?= /X/lib/include
INSTALLMAN	?= /X/man/lib/$(NAME)
#elif !empty(TYPE:MX11R3)
INCLUDEDIR	?= /mic/X11R3/lib/include/X11
INSTALLMAN	?= /mic/X11R3/man/lib/$(NAME)
#elif !empty(TYPE:MX11R4)
INCLUDEDIR	?= /X11/R4/lib/include/X11
INSTALLMAN	?= /X11/R4/man/lib/$(NAME)
#endif

#
# Figure out what stuff we'll pass to sub-makes.
#
PASSVARS	= 'INSTALLDIR=$(INSTALLDIR)' $(.MAKEFLAGS)
#ifdef		XCFLAGS
PASSVARS	+= 'XCFLAGS=$(XCFLAGS)'
#endif
#ifdef		XAFLAGS
PASSVARS	+= 'XAFLAGS=$(XAFLAGS)'
#endif

#
# Flags everyone should have. XCFLAGS and XAFLAGS are provided for
# the user to add flags for CC, AS or LINT from the command line.
#

#include	<tm.mk>
CTFLAGS		?= -wt
DEPFLAGS	?=
INSTALLFLAGS	?=
INSTALLMANFLAGS	?=
LINTFLAGS	?= -m$(TM)
LINTFLAGS	+= -u -M
XCFLAGS		?=
XAFLAGS		?=
#
# The .INCLUDES variable already includes directories that should be
# used by cc and other programs by default.  Remove them, just so that
# the output looks cleaner.
#

#
# The ds3100 port is not ready for the -O yet.
#

#if !empty(TM:Mds3100)
CFLAGS		+= -O $(TMCFLAGS) $(XCFLAGS) -I.
GFLAG		= -g3
#elif !empty(TM:Mspur)
CFLAGS		+= $(TMCFLAGS) $(XCFLAGS) -I.
GFLAG		=
#else
CFLAGS		+= -O $(TMCFLAGS) $(XCFLAGS) -I.
GFLAG		= -g
#endif

#
# Since the dec compiler doesn't include /sprite/lib/include we must leave
# the path as is when TM=ds3100.
#

CFLAGS		+= $(.INCLUDES:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g)
#if empty(TM:Mds3100)
AFLAGS		+= $(TMAFLAGS) $(XAFLAGS)
#else
CFLAGS		+= -I/sprite/lib/include -I/sprite/lib/include/$(TM).md
AFLAGS		+= $(.INCLUDES)
#endif

.MAKEFLAGS	: -C

#
# The c library is used by the kernel.  Since the kernel does not
# use the floating point coprocessor, the c library must be compiled
# to use software floating point.  This isn't any big deal because
# there isn't very much floating point stuff in libc anyway.
#

#if !empty(TM:Msun3) && !empty(NAME:Mc)
CFLAGS          += -msoft-float
#endif

#
# Define the various types of libraries we can make to make our rules and
# maybe the user's easier to write.
#
REGLIB		= ../$(TM).md/lib$(NAME).a
PROFLIB		= ../$(TM).md/lib$(NAME)_p.a
DEBUGLIB	= ../$(TM).md/lib$(NAME)_g.a
LINTLIB		= llib-l$(SUBDIR).ln

#
# Figure out which files to use in cases where the file may be either
# machine-dependent or machine-independent
#

DEPFILE		= $(TM).md/dependencies.mk
LINTFILE	= $(TM).md/lint

#
# Transformation rules: these have special features to place .o files
# in md subdirectories, run preprocessor over .s files, and generate
# .po files for profiling.
#

.SUFFIXES	: .po .go

.c.o		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -c $(.IMPSRC) -o $(.TARGET)
.c.po		:
	$(RM) -f $(.TARGET)
#ifdef NOPROFILE	    
	$(CC) $(CFLAGS) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
#else	    
	$(CC) $(CFLAGS) -p -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
#endif	   	    

.c.go		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(GFLAG) -c $(.IMPSRC) -o $(.TARGET)
.s.go .s.po .s.o	:
#if empty(TM:Mds3100)
	$(CPP) $(CFLAGS:M-[IDU]*) -D$(TM) -m$(TM) -D_ASM $(.IMPSRC) > $(.PREFIX).pp
	$(AS) -o $(.TARGET) $(AFLAGS) $(.PREFIX).pp
	$(RM) -f $(.PREFIX).pp
#else 
	$(RM) -f $(.TARGET)
	$(AS) $(AFLAGS) $(.IMPSRC) -o $(.TARGET)
#endif

#
# The rule below is needed to make archives, so that the archive
# member depends on the corresponding .o (or .po) file.  For some
# reason, this rule doesn't work without some commands (and the ...
# is enough).
.go.a .po.a .o.a	:
	...

#
# MAKEINSTALLHDRS usage:
#	<target> : MAKEINSTALLHDRS
# All of the public headers files get updated to INCLUDEDIR.
#
MAKEINSTALLHDRS	: .USE .SILENT
#if !empty(PUBHDRS)
	$(UPDATE) -l -m 664 -t $(INSTALLFLAGS) $(PUBHDRS) $(INCLUDEDIR)
#endif
#if !empty(MDPUBHDRS)
	$(UPDATE) -l -m 664 -t $(INSTALLFLAGS) \
		$(MDPUBHDRS) $(INCLUDEDIR)/$(TM).md
#endif

#
# MAKELINT usage:
#	<fluff-file> : <sources to be linted> MAKELINT
#
# <fluff-file> is the place to store the output from the lint.
#
MAKELINT	: .USE
	$(RM) -f $(.TARGET)
	$(LINT) $(LINTFLAGS) $(CFLAGS:M-[IDU]*) $(.ALLSRC:M*.c) \
		> $(.TARGET) 2>&1
#
# MAKEDEPEND usage:
#	<dependency-file> : <sources> MAKEDEPEND
#
# Generate dependency file suitable for inclusion in future makes.

MAKEDEPEND	: .USE
	@$(TOUCH) $(DEPFILE)
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*) -m $(TM) -w80 -f $(DEPFILE) $(.ALLSRC)
	@$(MV) -f $(DEPFILE) $(DEPFILE).tmp
	@$(SED) -e '/^#/!s|^\([^:]*\)\.o[ 	]*:|$(TM).md/\1.po $(TM).md/\1.go $(TM).md/&|' <$(DEPFILE).tmp > $(DEPFILE)
	@$(RM) -f $(DEPFILE).tmp

#ifndef no_targets
#
# Define all the main targets.  See the Mkmf man page for details.
#
default			: $(REGLIB)
$(REGLIB)		: $(REGLIB)($(OBJS)) .PRECIOUS
	$(AR) $(ARFLAGS) $(.TARGET) $(.OODATE)
	...
	$(RANLIB) $(.TARGET)
#if !empty(CLEANOBJS:M*.o) && empty(TM:Mds3100)
	rm -rf $(CLEANOBJS:M*.o)
#endif

# 
# Rebuild is used to remake from scratch, doing ar q instead of ar r
# to make it fast, and doing only a single ranlib at the top level
#
rebuild		: $(REGLIB)($(OBJS)) .PRECIOUS
	$(AR) q $(REGLIB) $(.OODATE)

debug			: $(DEBUGLIB)
$(DEBUGLIB)		: $(DEBUGLIB)($(OBJS:S/.o$/.go/g)) .PRECIOUS
	$(AR) $(ARFLAGS) $(.TARGET) $(.OODATE)
	...
	$(RANLIB)  $(.TARGET)
#if !empty(CLEANOBJS:M*.o) && empty(TM:Mds3100)
	rm -rf $(CLEANOBJS:M*.o:S/.o$/.go/g)
#endif

clean			! .IGNORE
	$(RM) -f $(CLEANOBJS) $(CLEANOBJS:M*.o:S/.o$/.po/g) \
		$(CLEANOBJS:M*.o:S/.o$/.go/g) $(TM).md/$(LINTLIB) \
		$(LINTFILE) y.tab.c lex.yy.c core a.out *~ \
		$(TM).md/*~ version.h
	$(AR) d $(REGLIB) $(OBJS)
	$(RANLIB) $(REGLIB)
	$(AR) d $(PROFLIB) $(OBJS:S/.o$/.po/g)
	$(RANLIB) $(PROFLIB)
	$(AR) d $(DEBUGLIB) $(OBJS:S/.o$/.go/g)
	$(RANLIB) $(DEBUGLIB)

depend			:: $(DEPFILE)
$(DEPFILE)		! $(SRCS:M*.c) $(SRCS:M*.s) $(SRCS:M*.cc) MAKEDEPEND

install			:: $(REGLIB) installhdrs installman lintlib
installdebug		:: $(DEBUGLIB)
installhdrs		:: MAKEINSTALLHDRS
#if empty(MANPAGES)
installman		:: .SILENT
	echo "No man pages for library $(NAME)/$(SUBDIR)?  Please write some."
#elif !empty(MANPAGES:MNONE)
installman		::

#elif defined(INSTALLMAN)
installman		:: .SILENT
	$(UPDATE) -m 444 -l $(INSTALLMANFLAGS) $(MANPAGES) $(INSTALLMAN)
#else
installman		:: .SILENT
	echo "Can't install man page(s): no install directory defined"
#endif
installprofile		:: $(PROFLIB)

lint			:: $(LINTFILE)
$(LINTFILE)		: $(SRCS:M*.c) $(HDRS) MAKELINT

lintlib			:: $(TM).md/$(LINTLIB)
$(TM).md/$(LINTLIB)	: $(SRCS:M*.c) $(HDRS) $(LINTSRCS)
	$(RM) -f $(.TARGET) llib-l$(SUBDIR).ln
	$(LINT) -C$(NAME) $(CFLAGS:M-[IDU]*) -DLINTLIB $(LINTFLAGS) \
		$(SRCS:M*.c) $(LINTSRCS)
	$(MV) llib-l$(NAME).ln $(.TARGET)

mkmf			! .SILENT
	mkmf

newtm			! .SILENT
	if $(TEST) -d $(TM).md; then
	    true
	else
	    mkdir $(TM).md;
	    chmod 775 $(TM).md;
	    mkmf -m$(TM)
	fi

profile			:: $(PROFLIB)
$(PROFLIB)		: $(PROFLIB)($(OBJS:S/.o$/.po/g)) .PRECIOUS
	$(AR) $(ARFLAGS) $(.TARGET) $(.OODATE)
	...
	$(RANLIB)  $(.TARGET)
#if !empty(CLEANOBJS:M*.o) && empty(TM:Mds3100)
	rm -rf $(CLEANOBJS:M*.o:S/.o$/.po/g)
#endif

tags			:: $(ALLSRCS:M*.c) $(HDRS)
	$(CTAGS) $(CTFLAGS) $(ALLSRCS:M*.c)

DISTFILES    ?=

dist        !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk $(TM).md/md.mk \
	    $(MANPAGES) $(SRCS) $(HDRS) $(DISTFILES)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i} ;else true; fi
	done
#else
	@echo "Sorry, no distribution directory defined"
#endif

#include		<all.mk>
#endif no_targets

#include		<rdist.mk>
