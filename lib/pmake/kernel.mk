#
# This is a library Makefile that is included by the Makefile's for
# modules of the Sprite kernel.  The file that includes this one should
# already have defined the following variables:
# Variables:
#	NAME		module name
#	SRCS		all sources for the module for the current target
#			machine
#	ALLSRCS		all sources for the module, including all sources
#			for all target machines
#	ALLHDRS		all headers for the module
#	OBJS		object files from which to create the module
#	CLEANOBJS	object files to be removed as part of "make clean"
#			(need not just be object files)
#	HDRS		all header files for the module
#	PUBHDRS		publicly-available headers for the module (this
#			contains only machine-independent headers)
#	MDPUBHDRS	machine-dependent public headers for the module
#			(for the current TM)
#	TM		target machine type for object files etc.
#	MACHINES	list of all target machines currently available
#			for this program.
#
# "Make install" installs the source before the object.  This is because
# make depend may get confused and have files depend on Include files rather
# than header files in this directory, and doing an installhdrs after
# making the object files might result in remaking all the object files a
# second time.  
#
# $Header: /sprite/lib/pmake/RCS/kernel.mk,v 1.91 92/11/27 17:17:37 jhh Exp $ SPRITE (Berkeley)
#

#
# Default certain variables so that Pmake won't barf during "make newtm"
# operations.
#
CLEANOBJS	?=
HDRS		?=
MDPUBHDRS	?=
OBJS		?=
SRCS		?=
INSTFILES	?=
SACREDOBJS	?=

#
# Important directories.
#
BINDIR		?= /sprite/cmds.$(MACHINE)
INCLUDEDIR	?= /sprite/src/kernel/Include
INSTALLDIR	?= /sprite/src/kernel/Installed/$(NAME)
LIBDIR		?= /sprite/src/kernel/$(TM).md
LINTDIR		?= /sprite/src/kernel/Lint/$(TM).md
PROFDIR		?= /sprite/src/kernel/Profiled/$(TM).md
USERINCLUDEDIR	?= /sprite/lib/include
SNAPDIR		?= /sprite/src/kernel/Snapshots

CSRCS		?= $(SRCS:M*.c)
SSRCS		?= $(SRCS:M*.s)
POBJS		?= $(OBJS:S/.o$/.po/g)
GOBJS		?= $(OBJS:S/.o$/.go/g)
ALLCSRCS	?= $(ALLSRCS:M*.c)
MDHDRS		?= $(HDRS:M*.md/*)
MDSRCS		?= $(SRCS:M*.md/*)
MDINSTFILES	?= $(INSTFILES:M*.md/*)

#
# Define search paths for libraries, include files and lint libraries
#
.PATH.a		:
.PATH.h		:
#ifdef FIRSTHDRDIRS
.PATH.h		: $(FIRSTHDRDIRS)
#endif
.PATH.h		: $(TM).md . $(INCLUDEDIR)/$(TM).md $(INCLUDEDIR) \
			$(USERINCLUDEDIR)/$(TM).md $(USERINCLUDEDIR) 
.PATH.ln	: 
#ifdef MYLINTDIR
.PATH.ln	: $(MYLINTDIR)
#endif
.PATH.ln	: $(LINTDIR)
.PATH.c		:
.PATH.c		: $(TM).md
.PATH.s		:
.PATH.s		: $(TM).md

#ifdef MYLINTDIR
LINTINSTALLDIR = $(MYLINTDIR)
#else
LINTINSTALLDIR = $(LINTDIR)
#endif 

#
# System programs -- assign conditionally so they may be redefined in
# including makefile.
#
AS		?= $(BINDIR)/as
CC		?= $(BINDIR)/cc
CP		?= $(BINDIR)/cp
CPP		?= $(BINDIR)/cpp -traditional -$
CTAGS		?= $(BINDIR)/ctags
ETAGS		?= /emacs/cmds/etags
LD		?= $(BINDIR)/ld
LINT		?= $(BINDIR)/lint
MV		?= $(BINDIR)/mv
MAKEDEPEND	?= $(BINDIR)/makedepend
MKDIR		?= $(BINDIR)/mkdir
RDIST		?= $(BINDIR)/rdist
RM		?= $(BINDIR)/rm
SCVS		?= $(BINDIR)/scvs
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
# ones should be added with the += operator in the local.mk makefile.
# The FLAGS variables are defined with the += operator in case this file
# is included after the main makefile has already defined them...
#

#include	<tm.mk>
CTFLAGS		?= -wtd
LINTFLAGS	?= -m$(TM)
LINTFLAGS	+= -S -M -n -u -D$(TM)
XCFLAGS		?=
XAFLAGS		?=

#include <debugflags.mk>

# If we're compiling with gcc (i.e., on Suns), ask gcc to give all
# warning messages.

#if empty(TM:Mds3100) && empty(TM:Mds5000)
GFLAG += -Wall
#endif

CFLAGS		+= -DKERNEL $(TMCFLAGS) $(XCFLAGS) 

#
# The .INCLUDES variable already includes directories that should be
# used by cc and other programs by default.  Remove them, just so that
# the command lines look cleaner.
#
# 
# Since the dec compiler doesn't include /sprite/lib/include we must leave
# the path as is when TM=ds3100.  Also the mips compiler doesn't include
# /sprite/lib/include either so add these in for both as and cc.
# For the sun4c, there is a problem with user include files pointing at
# kernel include files, since the compiler will cause this sort of reference
# to point at sun4 instead of sun4c, unless we specifically keep the reference
# on the compile line.
#

#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
CFLAGS          += $(.INCLUDES)
AFLAGS          += $(.INCLUDES) $(TMAFLAGS) $(XAFLAGS) -DKERNEL
#else if !empty(TM:Msun4c)
CFLAGS          += $(.INCLUDES)
AFLAGS		+= $(TMAFLAGS) $(XAFLAGS) 
#else
CFLAGS		+= $(.INCLUDES:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g)
AFLAGS		+= $(TMAFLAGS) $(XAFLAGS) 
#endif

#
# Floating point coprocessor instructions should not be used
# inside the kernel.
#

#if !empty(TM:Msun3) || !empty(TM:Mcleansun3)
CFLAGS          += -msoft-float
#endif

#
# Transformation rules: these have special features to place .o files
# in md subdirectories, run preprocessor over .s files, and generate
# .po files for profiling.
#
##	$(CC) -S $(CFLAGS) -p -DPROFILE $(.IMPSRC)
##	$(AS) -o $(.TARGET) $(AFLAGS) $(.PREFIX).s
##	$(RM) -f $(.PREFIX).s

.SUFFIXES	: .go .po

.c.o		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(OFLAG) $(GFLAG) -c $(.IMPSRC) -o $(.TARGET)
.c.go		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS)  $(GDFLAG) -c $(.IMPSRC) -o $(.TARGET)
.c.po		:
	$(RM) -f $(.TARGET)
	$(CC) $(CFLAGS) $(OFLAG) $(GFLAG) -p -DPROFILE -c $(.IMPSRC) -o $(.TARGET)
.s.po .s.o :
#if empty(TM:Mds3100) && empty(TM:Mds5000)
	$(CPP) $(CFLAGS:M-[IDU]*) -m$(TM) -D$(TM) -D_ASM $(.IMPSRC) > $(.PREFIX).pp
	$(AS) -o $(.TARGET) $(AFLAGS) $(AGFLAG) $(.PREFIX).pp
	$(RM) -f $(.PREFIX).pp
#else
	$(RM) -f $(.TARGET)
	$(AS) $(AFLAGS) $(AGFLAG) $(.IMPSRC) -o $(.TARGET)
#endif

.s.go :
#if empty(TM:Mds3100) && empty(TM:Mds5000)
	$(CPP) $(CFLAGS:M-[IDU]*) -m$(TM) -D$(TM) -D_ASM $(.IMPSRC) > $(.PREFIX).pp
	$(AS) -o $(.TARGET) $(AFLAGS) $(AGDFLAG) $(.PREFIX).pp
	$(RM) -f $(.PREFIX).pp
#else
	$(RM) -f $(.TARGET)
	$(AS) $(AFLAGS) $(AGDFLAG) $(.IMPSRC) -o $(.TARGET)
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
#if empty(TM:Mds3100)  && empty(TM:Mds5000)
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*) -m $(TM) -w50 -f $(DEPFILE) $(.ALLSRC)
#else
	$(MAKEDEPEND) $(CFLAGS:M-[ID]*:S|^-I/sprite/lib/include$||g:S|^-I/sprite/lib/include/$(TM).md$||g) -m $(TM) -w50 -f $(DEPFILE) $(.ALLSRC)
#endif
	@$(MV) -f $(DEPFILE) $(DEPFILE).tmp
	@$(SED) -e '/^#/!s|^\([^:]*\)\.o[ 	]*:|$(TM).md/\1.po $(TM).md/\1.go $(TM).md/&|' <$(DEPFILE).tmp > $(DEPFILE)
	@$(RM) -f $(DEPFILE).tmp

#
# Define the main targets.
#

$(TM).md/$(NAME).o	: $(OBJS)
	$(RM) -f $(.TARGET)
	$(LD) -r $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)
$(TM).md/$(NAME).go	: $(GOBJS)
	$(RM) -f $(.TARGET)
	$(LD) -r $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)
default			: $(TM).md/$(NAME).o
debug			: $(TM).md/$(NAME).go

clean tidy		:: .NOEXPORT
	$(RM) -f $(CLEANOBJS) $(CLEANOBJS:S/.o$/.po/g) \
		$(CLEANOBJS:S/.o$/.go/g) $(TM).md/$(NAME).o \
		$(TM).md/$(NAME).po *~ $(TM).md/*~

DEPFILE = $(TM).md/dependencies.mk
depend			: $(DEPFILE)
$(DEPFILE)		! $(SRCS:M*.c) $(SRCS:M*.s) MAKEDEPEND

install			:: installobj installhdrs
#if !empty(TM:Mds3100) || !empty(TM:Mds5000)
install			:: instlint 
#endif

installhdrs		::
	@$(UPDATE) -t -n $(PUBHDRS) $(INCLUDEDIR)
#if !empty(MDPUBHDRS)
	@-$(MKDIR) $(INCLUDEDIR)/$(TM).md > /dev/null 2>&1
	@$(UPDATE) -t -n $(MDPUBHDRS) $(INCLUDEDIR)/$(TM).md
#endif
instlint		:: $(LINTINSTALLDIR)/$(NAME).ln
installobj		:: $(LIBDIR)/$(NAME).o tags TAGS
installdbg		:: $(LIBDIR)/$(NAME).go

$(LIBDIR)/$(NAME).o	: $(TM).md/$(NAME).o
	$(UPDATE) -l -n $(TM).md/$(NAME).o $(LIBDIR)
$(LIBDIR)/$(NAME).go	: $(TM).md/$(NAME).go
	$(UPDATE) -l -n $(TM).md/$(NAME).go $(LIBDIR)

lint			: $(TM).md/lint
$(TM).md/lint		: $(LINTLIBS) $(CSRCS) 
	$(RM) -f $(.TARGET)
	$(LINT) $(LINTFLAGS) $(CFLAGS:M-[IDU]*) $(.ALLSRC) \
		> $(.TARGET) 2>& 1
$(TM).md/llib-l$(NAME).ln	: $(CSRCS) $(HDRS)
	$(RM) -f $(.TARGET)
	$(RM) -f llib-l$(NAME).ln
	$(LINT) -C$(NAME) $(CFLAGS:M-[IDU]*) $(LINTFLAGS) $(.ALLSRC:M*.c)
	$(MV) llib-l$(NAME).ln $(.TARGET)
$(LINTINSTALLDIR)/$(NAME).ln	: $(TM).md/llib-l$(NAME).ln
	$(RM) -f $(.TARGET)
	$(CP) $(TM).md/llib-l$(NAME).ln $(.TARGET)

mkmf			!
	mkmf

newtm			! .SILENT
	if $(TEST) -d $(TM).md; then
	    true
	else
	    mkdir $(TM).md;
	    chmod 775 $(TM).md;
	    mkmf -m$(TM)
	fi

profile			:: $(TM).md/$(NAME).po
$(TM).md/$(NAME).po	: $(POBJS)
	$(RM) -f $(.TARGET)
	$(LD) -r $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

tags			:: $(ALLCSRCS) $(HDRS)
	$(CTAGS) $(CTFLAGS) $(ALLCSRCS) $(HDRS)

TAGS			:: $(ALLCSRCS) $(HDRS)
	$(ETAGS) $(ALLCSRCS)

DISTFILES	?=

dist            !
#if defined(DISTDIR) && !empty(DISTDIR)
	for i in Makefile local.mk $(TM).md/md.mk \
	    $(DISTFILES) $(SRCS) $(HDRS) $(SACREDOBJS)
	do
	if $(TEST) -e $${i}; then
	    $(UPDATE)  -l -n $${i} $(DISTDIR)/$${i} ;else true; fi
	done
#else
	@echo "Sorry, no distribution directory defined"
#endif

#include	<all.mk>

# For rdist, take the standard kernel makefile.
# Allow the user to specify extra flags (like -v) for the rdist via
# the DISTFLAGS variable.

DISTFILE 	?= /sprite/lib/misc/distfile.kernel
DISTFLAGS	?= 

Rdist		:: 
	$(RDIST) $(DISTFLAGS) -f $(DISTFILE) -d DIR=`pwd`

.MAKEFLAGS	: -C		# No compatibility needed

update		::
	$(SCVS) update

SNAPFLAGS	?= 

snapshot		::
#ifdef SNAPVERSION
	if $(TEST) -f $(SNAPDIR)/$(SNAPVERSION)/$(NAME)/.ssdone; then
	    true
	else
	    for i in $(ALLSRCS) $(ALLHDRS) 
	    do 
	 $(UPDATE) $(SNAPFLAGS) $${i} $(SNAPDIR)/$(SNAPVERSION)/$(NAME)/$${i} 
	    done
	    $(TOUCH) $(SNAPDIR)/$(SNAPVERSION)/$(NAME)/.ssdone
	fi
#else
	@echo "You must specify a snapshot version number via SNAPVERSION
#endif

listsrcs:
#ifdef LISTFILE
	@echo $(ALLSRCS) $(ALLHDRS) | tr ' ' '\012' >> $(LISTFILE)
#else
	@echo $(ALLSRCS) $(ALLHDRS)
#endif

