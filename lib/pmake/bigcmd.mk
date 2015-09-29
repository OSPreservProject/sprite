#
# Included makefile for creating a kernel module.
# Variables provided by Makefile that includes this one:
#	NAME		module name
#	SRCS		all sources for the module for the current target
#			machine
#	ALLSRCS		all sources for the module, including all sources
#			for all target machines (used for ctags)
#	OBJS		object files from which to create the module
#	CLEANOBJS	object files to be removed as part of "make clean"
#			(need not just be object files)
#	HDRS		all header files for the module
#	TM		target machine type for object files etc.
#	MACHINES	list of all target machines currently available
#			for this program.
#
# $Header: /sprite/lib/pmake/RCS/bigcmd.mk,v 1.28 92/04/13 18:32:58 elm Exp $ SPRITE (Berkeley)
#

#
# The variables below should be defined in md.mk, but they are given
# default values just in case md.mk doesn't exist yet.
#
SRCS		?=
OBJS		?=
HDRS		?=

CSRCS		?= $(SRCS:M*.c)
SSRCS		?= $(SRCS:M*.s)
POBJS		?= $(OBJS:S/.o$/.po/g)
ALLCSRCS	?= $(ALLSRCS:M*.c)

#
# Define search paths for libraries, include files and lint libraries
#
.PATH.a		:
.PATH.h		:
.PATH.h		: $(TM).md /sprite/lib/include /sprite/lib/include/$(TM).md
.PATH.ln	: 
.PATH.ln	: /sprite/lib/lint
.PATH.c		:
.PATH.c		: $(TM).md
.PATH.s		:
.PATH.s		: $(TM).md

#
# Important directories. 
#
BINDIR		= /sprite/cmds.$(MACHINE)

#
# System programs -- assign conditionally so they may be redefined in
# including makefile.
#
AS		?= $(BINDIR/as
CC		?= $(BINDIR)/cc
CP		?= $(BINDIR)/cp
CPP		?= $(BINDIR)/cpp -traditional -$
CTAGS		?= $(BINDIR)/ctags
LD		?= $(BINDIR)/ld.new
LINT		?= $(BINDIR)/lint
MAKEDEPEND	?= $(BINDIR)/makedepend
MV		?= $(BINDIR)/mv
RM		?= $(BINDIR)/rm
SED		?= $(BINDIR)/sed
TEST            ?= $(BINDIR)/test
TOUCH		?= $(BINDIR)/touch
UPDATE		?= $(BINDIR)/update

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
# Flags. These are ones that are needed by *all* modules. Any other
# ones should be added with the += operator in local.mk files.
# The FLAGS variables are defined with the += operator in case this file
# is included after the main makefile has already defined them...
#

#include	<tm.mk>
CTFLAGS		?= -wt
INSTALLFLAGS	?=
LINTFLAGS	?= -m$(TM) 
LINTFLAGS	+= -u -M
XCFLAGS		?=
XAFLAGS		?=
#
# The .INCLUDES variable already includes directories that should be
# used by cc and other programs by default.  Remove them, just so that
# the output looks cleaner.
#
# The ds3100 compiler doesn't include /sprite/lib/include, so we must leave
# the path as is when TM=ds3100.  Also, the ds3100 port isn't ready for the -O
# flag yet.
#

#include <debugflags.mk>

CFLAGS		+= $(GFLAG) ${OFLAG} $(TMCFLAGS) $(XCFLAGS) -I.
CFLAGS		+= $(.INCLUDES:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g)
#if !empty(TM:Mds3100) 
CFLAGS		+= -I/sprite/lib/include -I/sprite/lib/include/$(TM).md
#endif
AFLAGS		+= $(TMAFLAGS) $(XAFLAGS)

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
# MAKEDEPEND usage:
#	<dependency-file> : <sources> MAKEDEPEND
#
# Generate dependency file suitable for inclusion in future makes.  Must
# mung the dependency file in two ways:  a) add a .md prefix on all the .o
# file names;  b) for each entry for a .o file, generate an equivalent
# entry for a .po file.

MAKEDEPEND	: .USE
	@$(TOUCH) $(DEPFILE)
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*) -m $(TM) -w50 -f $(DEPFILE) $(.ALLSRC)
	@$(MV) -f $(DEPFILE) $(DEPFILE).tmp
	@$(SED) -e '/^#/!s|^\([^:]*\)\.o[ 	]*:|$(TM).md/\1.po $(TM).md/&|' <$(DEPFILE).tmp > $(DEPFILE)
	@$(RM) -f $(DEPFILE).tmp

#if !defined(no_targets)
#
# We should define the main targets (make, make install, etc.).
#

default				: $(TM).md/linked.o
$(TM).md/linked.o		: $(OBJS)
	$(RM) -f $(.TARGET)
	$(LD) $(LDFLAGS) -r $(.ALLSRC) -o $(.TARGET)

clean				::
	$(RM) -f $(CLEANOBJS) $(CLEANOBJS:S/.o$/.po/g) $(TM).md/linked.o \
		$(TM).md/linked.po *~ $(TM).md/*~

DEPFILE = $(TM).md/dependencies.mk
depend			: $(DEPFILE)
$(DEPFILE)		! $(SRCS:M*.c) $(SRCS:M*.s) $(SRCS:M*.cc) MAKEDEPEND

install				:: default

# Name of module lint library.
MODLINTLIB	= llib-l$(NAME).ln

lint				: $(TM).md/lint
$(TM).md/lint			: $(CSRCS) ../$(TM).md/lintlib.ln
	$(RM) -f $(.TARGET)
	$(LINT) $(LINTFLAGS) $(CFLAGS:M-[IDU]*) $(.ALLSRC) \
		> $(.TARGET) 2>& 1

lintlib			:: $(TM).md/$(MODLINTLIB)
$(TM).md/$(MODLINTLIB)	: $(CSRCS) $(HDRS)
	$(RM) -f $(.TARGET)
	$(LINT) -C$(NAME) $(CFLAGS:M-[IDU]*) -DLINTLIB $(LINTFLAGS) $(.ALLSRC:M*.c)
	$(MV) $(MODLINTLIB) $(.TARGET)

mkmf				!
	mkmf

newtm					! .SILENT
	if $(TEST) -d $(TM).md; then
	    true
	else
	    mkdir $(TM).md;
	    chmod 775 $(TM).md;
	    mkmf -m$(TM)
	fi

profile				: $(TM).md/linked.po
$(TM).md/linked.po: $(POBJS)
	$(RM) -f $(.TARGET)
	$(LD) $(LDFLAGS) -r $(.ALLSRC) -o $(.TARGET)

tags				:: $(ALLCSRCS) $(HDRS)
	$(CTAGS) $(CTFLAGS) $(ALLCSRCS)

DISTFILES    ?=

dist        !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk $(TM).md/md.mk $(SRCS) $(HDRS) $(DISTFILES)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  $${i} $(DISTDIR)/$${i}; else true ; fi
	done
#else
	@echo "Sorry, no distribution directory defined"
#endif

#include	<all.mk>

#endif no_targets

.MAKEFLAGS	: -C		# No compatibility needed
