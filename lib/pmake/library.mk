#
# Included makefile for creating a single library.  This Makefile assumes
# that the library is contained in a single directory and its .md
# subdirectories.
#
# $Header: /sprite/lib/pmake/RCS/library.mk,v 1.76 92/11/27 17:17:05 jhh Exp $ SPRITE (Berkeley)
#
# The makefile that includes this one should already have defined the
# following variables:
#	NAME		base name of library (e.g. tcl, sx, c, etc.)
#	SRCS		all sources for the library for the current target
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
# The variables below should be defined in md.mk, but they are given
# default values just in case md.mk doesn't exist yet.
#
CLEANOBJS	?=
HDRS		?=
MDPUBHDRS	?=
OBJS		?=
SRCS		?=

CSRCS		?= $(SRCS:M*.c)
SSRCS		?= $(SRCS:M*.s)
CCSRCS		?= $(SRCS:M*.cc)

#
# Define search paths for include files and source files (for sources,
# must be sure to look both in this directory and in the machine-dependent
# one).
#
.PATH.a		: # Clear out
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
BINDIR		?= /sprite/cmds.$(MACHINE)

AS		?= $(BINDIR)/as
CC		?= $(BINDIR)/cc
CHGRP		?= $(BINDIR)/chgrp
CHMOD		?= $(BINDIR)/chmod
CHOWN		?= $(BINDIR)/chown
CP		?= $(BINDIR)/cp
CPLUSPLUS	?= $(BINDIR)/g++
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
INSTALLDIR	?= /sprite/lib
INSTALLMAN	?= /sprite/man/lib/$(NAME)
LINTDIR		?= /sprite/lib/lint
#elif !empty(TYPE:Mx)
INCLUDEDIR	?= /X/lib/include
INSTALLDIR	?= /X/lib
INSTALLMAN	?= /X/man/lib/$(NAME)
LINTDIR		?= /X/lib/lint
#elif !empty(TYPE:MX11R4)
INCLUDEDIR	?= /X11/R4/lib/include/X11
INSTALLDIR	?= /X11/R4/lib
INSTALLMAN	?= /X11/R4/man/lib/$(NAME)
LINTDIR		?= /X11/R4/lib/lint
#elif !empty(TYPE:Mpersonal)
INCLUDEDIR	?= $(HOME)/lib/include
INSTALLDIR	?= $(HOME)/lib
INSTALLMAN	?= $(HOME)/man/lib/$(NAME)
LINTDIR		?= $(HOME)/lib/lint
#ifndef		USERBACKUP
NOBACKUP	=
#endif
#elif !empty(TYPE:Mlocal)
INCLUDEDIR	?= /local/lib/include
INSTALLDIR	?= /local/lib
INSTALLMAN	?= /local/man/lib/$(NAME)
LINTDIR		?= /local/lib/lint
#else
INCLUDEDIR	?=
INSTALLDIR	?=
INSTALLMAN	?=
LINTDIR		?=
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
#ifdef		NOBACKUP
PASSVARS	+= 'NOBACKUP=$(NOBACKUP)'
#endif
#ifdef		BACKUPAGE
PASSVARS	+= 'BACKUPAGE=$(BACKUPAGE)'
#endif

#
# Flags everyone should have. XCFLAGS, XAFLAGS and CLINTFLAGS are provided
# for the user to add flags for CC, AS or LINT from the command line.
#

#include	<tm.mk>
CTFLAGS		?= -wt
DEPFLAGS	?=
INSTALLFLAGS	?=
INSTALLMANFLAGS	?=
LINTFLAGS	?= -m$(TM)
LINTFLAGS	+= -u 
XAFLAGS		?=
XCFLAGS		?=
#
# The .INCLUDES variable already includes directories that should be
# used by cc and other programs by default.  Remove them, just so that
# the output looks cleaner.
#

#include <debugflags.mk>

CFLAGS		+= $(OFLAG) $(TMCFLAGS) $(XCFLAGS) -I.

#
# Since the dec compiler doesn't include /sprite/lib/include, we must leave
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
# Transformation rules: these have special features to place .o files
# in md subdirectories, run preprocessor over .s files, and generate
# .po files for profiling.
#

.SUFFIXES	: .po .go

.c.o		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) -c $(.IMPSRC) -o $(.TARGET)
.c.go		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(GFLAG) -c $(.IMPSRC) -o $(.TARGET)
.c.po		:
	$(RM) -f $(.TARGET)
#ifdef NOPROFILE	    
	$(CC) $(CFLAGS) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
#else	    
	$(CC) $(CFLAGS) -p -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
#endif	    

.s.go .s.po .s.o:
#if empty(TM:Mds3100) && empty(TM:Mjhh) && empty(TM:Mcleands3100)
	$(CPP) $(CFLAGS:M-[IDU]*) -m$(TM) -D$(TM) -D_ASM $(.IMPSRC) > $(.PREFIX).pp
	$(AS) -o $(.TARGET) $(AFLAGS) $(.PREFIX).pp
	$(RM) -f $(.PREFIX).pp
#else
	$(RM) -f $(.TARGET)
	$(AS) $(AFLAGS) $(.IMPSRC) -o $(.TARGET)
#endif

.cc.o	:
	$(RM) -f $(.TARGET)
	$(CPLUSPLUS) $(CFLAGS) -c $(.IMPSRC) -o $(.TARGET)
.cc.go	:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(GFLAG) -c $(.IMPSRC) -o $(.TARGET)
.cc.po		:
	$(RM) -f $(.TARGET)
#ifdef NOPROFILE	    
	$(CPLUSPLUS) $(CFLAGS) -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
#else	    
	$(CPLUSPLUS) $(CFLAGS) -p -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
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
# MAKEINSTALLLIB usage:
#	<target> : <source> MAKEINSTALLLIB
# Will copy over a library and re-ranlib it.
#
MAKEINSTALLLIB : .USE
	$(RM) -f $(.TARGET)
	$(CP) $(.ALLSRC) $(.TARGET)
	$(RANLIB) $(.TARGET)

#
# MAKELINT usage:
#	<fluff-file> : <sources to be linted> MAKELINT
#
# <fluff-file> is the place to store the output from the lint.
#
MAKELINT	: .USE
	$(RM) -f $(.TARGET)
	$(LINT) $(LINTFLAGS) $(CFLAGS:M-[IDU]*) $(.ALLSRC) \
		> $(.TARGET) 2>&1

#
# MAKEDEPEND usage:
#	<dependency-file> : <sources> MAKEDEPEND
#
# Generate dependency file suitable for inclusion in future makes.
#
MAKEDEPEND	: .USE
	@$(TOUCH) $(.TARGET)
#if empty(TM:Mds3100)
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*) -m $(TM) -w80 -f $(.TARGET) $(.ALLSRC)
#else
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g) -m $(TM) -w80 -f $(.TARGET) $(.ALLSRC)
#endif
	@$(MV) -f $(.TARGET) $(.TARGET).tmp
	@$(SED) -e '/^#/!s|^\([^:]*\)\.o[ 	]*:|$(TM).md/\1.po $(TM).md/\1.go $(TM).md/&|' <$(.TARGET).tmp > $(.TARGET)
	@$(RM) -f $(.TARGET).tmp

#ifndef no_targets
#
# We should define the main targets.  See the Mkmf man page for details.
#

REGLIB			= $(TM).md/lib$(NAME).a
PROFLIB			= $(TM).md/lib$(NAME)_p.a
DEBUGLIB		= $(TM).md/lib$(NAME)_g.a
LINTLIB			= $(TM).md/llib-l$(NAME).ln
INSTLIB			= $(INSTALLDIR)/$(REGLIB)
INSTPROFILE		= $(INSTALLDIR)/$(PROFLIB)
INSTDEBUG		= $(INSTALLDIR)/$(DEBUGLIB)

default			: $(REGLIB)
$(REGLIB)		: $(REGLIB)($(OBJS))
	$(AR) $(ARFLAGS) $(.TARGET) $(.OODATE)
	$(RANLIB) $(.TARGET)
#if !empty(CLEANOBJS:M*.o)
	rm -rf $(CLEANOBJS:M*.o)
#endif

clean tidy		::
	$(RM) -f $(REGLIB) $(DEBUGLIB) $(PROFLIB) $(CLEANOBJS) \
		$(CLEANOBJS:M*.o:S/.o$/.po/g) $(CLEANOBJS:M*.o:S/.o$/.go/g) \
		$(LINTLIB) y.tab.c lex.yy.c core a.out *~ $(TM).md/*~ \
		version.h $(TM).md/lint

debug			: $(DEBUGLIB)
$(DEBUGLIB)		: $(DEBUGLIB)($(OBJS:S/.o$/.go/g))
	$(AR) $(ARFLAGS) $(.TARGET) $(.OODATE)
	...
	$(RANLIB) $(.TARGET)
#if !empty(CLEANOBJS:M*.o)
	rm -rf $(CLEANOBJS:M*.o:S/.o$/.go/g)
#endif

DEPFILE = $(TM).md/dependencies.mk
depend			: $(DEPFILE)
$(DEPFILE)		! $(CSRCS) $(CCSRCS) $(SSRCS) MAKEDEPEND

#if empty(TYPE:Munknown)
#ifndef NOLINT
install			:: installlib installhdrs installlint installman
#else
install			:: installlib installhdrs installman
#endif
installdebug		:: $(DEBUGLIB) debuglink
debuglink		!
	rm -f $(INSTDEBUG)
	ln -s `pwd`/$(DEBUGLIB) $(INSTDEBUG)
installhdrs		:: MAKEINSTALLHDRS
installlib		:: $(INSTALLDIR)/$(REGLIB)
installlint		:: $(LINTDIR).$(TM)/llib-l$(NAME).ln
#if empty(MANPAGES)
installman		:: .SILENT
	echo "No man pages for library $(NAME)?  Please write some."
#elif !empty(MANPAGES:MNONE)
installman		::
#else
installman		:: .SILENT
	$(UPDATE) -m 444 -l $(INSTALLMANFLAGS) $(MANPAGES) $(INSTALLMAN)
#endif
installprofile		:: $(INSTPROFILE)
$(INSTLIB)		: $(REGLIB) MAKEINSTALLLIB
$(INSTPROFILE)		: $(PROFLIB) MAKEINSTALLLIB
#else
#
# The targets below are used if there's no known place to install this
# library:  just output a warning message.
#
install installdebug installlint installlib installman installprofile \
installquick		:: .SILENT
	echo "Can't install library $(NAME): no install directory defined"
#endif

library			: $(REGLIB)

lint			: $(TM).md/lint
$(TM).md/lint		: $(SRCS:M*.c) MAKELINT

$(LINTDIR).$(TM)/llib-l$(NAME).ln	: $(LINTLIB) .SILENT
	$(UPDATE) -m 664 $(INSTALLFLAGS) $(LINTLIB) $(.TARGET)
lintlib			: $(LINTLIB)
$(LINTLIB)		: $(CSRCS) $(HDRS) $(LINTSRCS)
	$(RM) -f $(.TARGET)
	$(LINT) -C$(NAME) $(CFLAGS:M-[IDU]*) -DLINTLIB $(LINTFLAGS) \
		$(.ALLSRC:M*.c) $(.ALLSRC:M*.lint)
	$(MV) llib-l$(NAME).ln $(.TARGET)

mkmf			::
	mkmf

newtm			:: .SILENT
	if $(TEST) -d $(TM).md; then
	    true
	else
	    mkdir $(TM).md;
	    chmod 775 $(TM).md;
	    mkmf -m$(TM)
	fi

profile			: $(PROFLIB)
$(PROFLIB)		: $(PROFLIB)($(OBJS:S/.o$/.po/g))
	$(AR) $(ARFLAGS) $(.TARGET) $(.OODATE)
	...
	$(RANLIB) $(.TARGET)
#if !empty(CLEANOBJS:M*.o)
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
