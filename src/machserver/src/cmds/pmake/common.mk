#
#  Makefile for the parallel Make program.
#  Note that this Makefile is formatted for PMake itself, rather than make.
#
# This file is to be included by the makefiles in unix, customs and sprite.
#
# Use "make" and "makefile" to compile it the first time...
#
#  $Id: common.mk,v 1.4 89/11/14 17:11:04 adam Exp $ SPRITE (Berkeley)
#
# No makemake.

#
# Path variables:
# 	DEST			where pmake should be installed
#	DESTLIB			place to store pmake's canned makefiles
#				(e.g. system.mk -- DEFSYSPATH in config.h)
#	SYSTEM_MK    	    	Actual name for the system makefile. I
#				use system.mk.
#	LIBDIR			where the pmake libraries are stored:
#		SPRITEDIR	where the Sprite->UNIX spriteibility
#				library is located.
#		LSTDIR		location of linked-list library
#		INCLUDE		where the sprite include files are
#
DEST		= /usr/public
DESTLIB		= /usr/public/lib/pmake
SYSTEM_MK	= system.mk

LIBDIR		= ../lib
SPRITEDIR	= $(LIBDIR)/sprite
LSTDIR		= $(LIBDIR)/lst
INCLUDE		= $(LIBDIR)/include
MKDIR		= $(LIBDIR)/mk

#if make(prof) && !defined(sun)
LISTLIB		= -llst_p
SPRITELIB	= -lsprite_p
#else
LISTLIB		= -llst
SPRITELIB	= -lsprite
#endif

#
# FLAG DEFINITIONS -- Flag variables for the various compilers, syntax
# checkers and whatnots
# CCFLAGS are for defining additional flags on the command line...
#
COFLAGS 	=
LDFLAGS		= $(.LIBS) 
LNFLAGS		= -b
XCFLAGS		=
CFLAGS		= $(.INCLUDES) $(XCFLAGS)

#
# SPECIAL TARGETS AND TRANSFORMATION RULES
# Note that the transformation rules from RCS files to standard ones includes
# the removal of any created .o file as well as the checked-out file.
# If you don't like this, nuke it.
#
.SUFFIXES	: .ln
	

.PATH.h		: # Clear out .h path
.PATH.h		: ../src $(LSTDIR) $(INCLUDE)
.PATH.a		: # Clear out .a path
.PATH.a		: $(LSTDIR) 
.PATH.ln	: $(LSTDIR) 
.PATH.c		: ../src

.DEFAULT	:
	co $(.TARGET)

.c.o		: .EXPORTSAME
	$(CC) $(CFLAGS) -c $(.IMPSRC)

#if	!defined(VARS_ONLY)

#
# Alter egos you want to support. smake and vmake both do System V
# impersonations, while just make pretends to be Make
#
ALIASES		= make smake vmake

#
# SOURCE DEFINITIONS -- All Sources for this program:
#
HDRS		= make.h nonints.h ar.h config.h
OBJS		= arch.o compat.o cond.o dir.o make.o job.o main.o parse.o \
		suff.o targ.o rmt.o str.o var.o
SRCS		= arch.c compat.c cond.c dir.c make.c job.c main.c parse.c \
		suff.c targ.c rmt.c str.c var.c
#ifdef SYSV
OBJS		+= utimes.o
SRCS		+= utimes.c
#endif SYSV

LLIBS		= llib-llst.ln
LIBS		= -llst

MAKEFILES	= linksprite.mk po.mk makelib.mk makelint.mk shx.mk \
                  $(SYSTEM_MK)

.MAIN		: pmake

#
# The actual installation of pmake itself is assumed to be done by the
# including makefile, but we need to set up the aliases and install the canned
# makefiles.
#
install		:: .NOEXPORT
	for i in $(MAKEFILES); do
	    install -c -m 444 $(MKDIR)/$i $(DESTLIB)
	done
	for in in $(ALIASES); do
	    rm -f $(DEST)/$i
	    ln -s pmake $(DEST)/$i
	done

nonints		: $(SRCS) .NOTMAIN
	rm -f ../src/nonints.h
	$(LIBDIR)/findni $(.ALLSRC) > /tmp/mni$$
	sed -e 's/(.*)/()/' /tmp/mni$$ > ../src/nonints.h
	chmod 444 ../src/nonints.h
	rm -f /tmp/mni$$

clean		:: .NOTMAIN .NOEXPORT
	rm -f $(OBJS)


tags		: $(HDRS) $(SRCS) .NOTMAIN
	@ctags -u $(.OODATE)

lint		:: $(SRCS) $(LLIBS) .NOTMAIN
	lint $(LNFLAGS) $(CFLAGS) $(.ALLSRC) > FLUFF 2>&1

co		: $(HDRS) $(SRCS) .NOTMAIN
	@chmod 664 $(.OODATE)

ci		: .NOTMAIN
	@ci $(SRCS) $(HDRS) < /dev/tty > /dev/tty 2>&1

#endif

DEPFILE		?= Makefile
#include	<makedepend.mk>

#ifdef HOST
rdist		: $(HDRS) $(SRCS) .NOTMAIN
	rdist -cyw $(SRCS) $(HDRS) $(LSTDIR) $(WILDDIR) \
		$(HOST):/sprite/src/cmds/pmake
#endif HOST
