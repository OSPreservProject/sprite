#
# This Makefile is included by several other system Makefiles.  It
# sets up variables that depend on the particular target machine
# being compiled for, such as compiler flags.  One variable should
# be defined before including this file:
#
# TM		Target machine for which code is to be generated (e.g.
#		sun2, sun3, spur, etc.)
#
# This file will create a variable TMCFLAGS, which, when passed to CC,
# will ensure that an object file is generated for machines of type TM.
# It will also generate a variable TMAFLAGS, which will serve the
# same purpose for the assembler.  Finally, it will set program names
# like AS and LD to values appropriate for the machine type.
#
# The only flags that should be defined here are those that are required 
# for successful compilation on that machine.  Optional flags such as
# optimization should be defined in local modules.  
#
# $Header: /sprite/lib/pmake/RCS/tm.mk,v 1.55 91/11/06 18:35:03 kupfer Exp $
# 

# Down at the bottom we edit $MACHINES so that "make all" will behave
# sensibly.  For the error messages below, though, we want to use the
# real $MACHINES.  So, we define a new variable for use in the error
# messages.

SUPPORTED_MACHINES	:= $(MACHINES)

# What follows is a big long if/elif chain, keyed on TM, to set flags,
# CC, etc.  Each top-level branch also verifies that TM is in MACHINES.
# It would be nice if we could do this in just one place, but pmake
# doesn't like the construct "empty(MACHINES:M$(TM))".

#if !empty(TM:Msun3)
TMCFLAGS		= -msun3 -Dsun3 -Dsprite
TMAFLAGS		= -m68020
LDFLAGS			= -msun3
#if empty(MACHINES:Msun3) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "supported machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif

#elif !empty(TM:Mspur)
TMCFLAGS		= -mspur -mlong-jumps -DLOCKREG 
TMAFLAGS		=
AS			= /sprite/cmds.$(MACHINE)/sas
LD			= /sprite/cmds.$(MACHINE)/sld
RANLIB			= /sprite/cmds.$(MACHINE)/sranlib
XLD			= /sprite/cmds.$(MACHINE)/xld
NOOPTIMIZATION		=
LDFLAGS			= 
#if empty(MACHINES:Mspur) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif

#elif !empty(TM:Msun4)
TMCFLAGS		= -msun4 -Dsprite -Dsun4
TMAFLAGS		= -msparc
LDFLAGS			= -msun4
#if empty(MACHINES:Msun4) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif

#elif !empty(TM:Mcleansun4)
TMCFLAGS		= -msun4 -Dsprite -Dsun4 -DCLEAN -DCLEAN_LOCK
TMAFLAGS		= -msparc
LDFLAGS			= -msun4
#if empty(MACHINES:Mcleansun4) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif

#elif !empty(TM:Msun4c)
TMCFLAGS		= -msun4 -Dsprite -Dsun4 -Dsun4c
TMAFLAGS		= -msparc
LDFLAGS			= -msun4
#if empty(MACHINES:Msun4c) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif

#elif !empty(TM:Mcleansun4c)
TMCFLAGS		= -msun4 -Dsprite -Dsun4 -Dsun4c -DCLEAN -DCLEAN_LOCK
TMAFLAGS		= -msparc
LDFLAGS			= -msun4
#if empty(MACHINES:Mcleansun4c) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif 

#elif !empty(TM:Mcleansun3) 
TMCFLAGS		= -msun3 -Dsun3 -Dsprite -DCLEAN -DCLEAN_LOCK
TMAFLAGS		= -m68020
LDFLAGS			= 
#if empty(MACHINES:Mcleansun3) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif

#elif !empty(TM:Mcleands3100)
TMCFLAGS	= -Dds3100 -Dsprite -DCLEAN -DCLEAN_LOCK -Uultrix
TMAFLAGS	= -Dds3100 -Dsprite -Uultrix
LDFLAGS		= -L/sprite/lib/ds3100.md
#if empty(MACHINES:Mcleands3100) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif

#elif !empty(TM:Mds3100)
TMCFLAGS	= -Dds3100 -Dsprite -Uultrix
TMAFLAGS	= -Dds3100 -Dsprite -Uultrix
LDFLAGS		= -L/sprite/lib/ds3100.md
#if empty(MACHINES:Mds3100) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(CC:Mgcc)
TMCFLAGS	+= -Dmips -DLANGUAGE_C
#endif
LINTFLAGS	+= -Dmips -DLANGUAGE_C

#elif !empty(TM:Mds5000)
TMCFLAGS	= -Dds5000 -Dsprite -Uultrix
TMAFLAGS	= -Dds5000 -Dsprite -Uultrix
LDFLAGS		= -L/sprite/lib/ds5000.md
#if empty(MACHINES:Mds5000) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(CC:Mgcc)
TMCFLAGS	+= -Dmips -DLANGUAGE_C
#endif
LINTFLAGS	+= -Dmips -DLANGUAGE_C

#elif !empty(TM:Msun4nw)

#    Sun4 compiled not to use save/restore register window instructions.
TMCFLAGS		= -msun4 -Dsprite -Dsun4 -B/users/mendel/lib/$(MACHINE).md/ -mno-windows -DNOWINDOWS
TMAFLAGS		= -msparc
LDFLAGS			= -msun4
#if empty(MACHINES:Msun4nw) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif

#elif !empty(TM:Msymm)
TMCFLAGS	= -msymm -Dsprite -Dsymm
TMAFLAGS	= -msymm
LDFLAGS		= -msymm
#if empty(MACHINES:Msymm) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif

#elif !empty(TM:Msym)
TMCFLAGS	= -msym -Dsprite -Dsym
TMAFLAGS	= -msym
LDFLAGS		= -msym
#if empty(MACHINES:Msym) && !make(newtm)
.BEGIN::
	@echo "Sorry, the target machine ($(TM)) isn't in the list of"
	@echo "allowed machines ($(SUPPORTED_MACHINES))."
	exit 1
#endif
#if !empty(MACHINE:Mds3100) || !empty(MACHINE:Mds5000)
CC                      = gcc
AS                      = gas
LD                      = gld
#endif

#else

# (some random TM)

TMCFLAGS		?= -m$(TM)
TMAFLAGS		?= -m$(TM)
#endif

# (End of big if/elif chain)



# The line below makes "clean" machines types and people's private "machines"
# invisible under for purposes of commands like "make all": you have
# to ask for them explicitly with the TM= option.  Also make "spur"
# and "symm" disappear; spur is obsolete, and symm is practically unused.

#ifdef MACHINES
MACHINES	:= $(MACHINES:Nfd:Njhh:Ncleansun4:Ncleansun4c:Ncleansun3:Ncleands3100:Ncleands5000:Nspur:Nsun4nw:Nsymm)
#endif



# This next tangle of tests checks for bogus cross-compilations.  The
# allowable cross-compilations are:
#
# on/target	sun3	sun4		DECstation	symm
# sun3		ok	ok		unsupported	unsupported
# sun4		ok	ok		unsupported	unsupported
# DECstation	ok	ok w/o ld	ok		ok
# symm		unsup.	unsup.		unsupported	ok
#
# For example, suns and DECstations can build for a sun3, but only
# DECstations can build for DECstations.  
# You can build for a sun4 on a DECstation, but only if you don't need
# to use ld. -mdk 15-Sep-1991.
#
# Complications:
# 1. We make "make all" intelligent enough not to try an unsupported
#    cross-compilation.  This is done by hacking the list of allowed
#    target types (MACHINES)).
# 2. We try to recognize that gcc does know something about
#    cross-compiling (see the CC assignments above).
#
# The tests are done in a separate batch because we only want to do
# them if we're actually going to compile, assemble, or link
# something.

# First check if we're actually going to build something.

#if !make(clean) && !make(depend) && !make(mkmf) && !make(tidy) && \
    !make(installhdrs) && !make(mkmfall) && !make(cleanall) && \
    !make(dependall) && !make(tidyall) && !make(dist) && !make(distall)

# We are.  Check for compatibility according to the above table, using
# an if/elif chain based on MACHINE.

#if !empty(MACHINE:Msun3) || !empty(MACHINE:Msun4)

# This next expression is all on one line because the Symmetry pmake
# can't handle continued lines in the middle of a parenthesized
# expression. -mdk 15-May-1991.

#if (!empty(TM:Mds3100) || !empty(TM:Mcleands3100) || !empty(TM:Mds5000) || !empty(TM:Mcleands5000)) && empty(CC:Mgcc)
.BEGIN::
	@echo "you cannot compile for a DECstation on this machine"
	exit 1
#endif
#if !empty(TM:Msymm)
.BEGIN::
	@echo "you cannot compile for a Symmetry on this machine"
	exit 1
#endif
#ifdef MACHINES
MACHINES	:= $(MACHINES:Nds3100:Nds5000)
#endif

#elif !empty(MACHINE:Msymm)

#if empty(TM:Msymm)
.BEGIN::
	@echo "you can only compile for a Symmetry on this machine"
	exit 1
#endif
MACHINES	= symm

#endif /* MACHINE chain */
#endif /* !make(clean), !make(depend), ... */

LDFLAGS	?=
