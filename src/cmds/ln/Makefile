#
# Prototype Makefile for cmds/* directories.  It permits the command to
# be compiled for multiple target machines, with one subdirectory of
# the form "sun2.md" that holds the object files and machine-specific
# sources (if any) for each target machine.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.command
# Wed Jan 31 14:54:19 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.command,v 1.8 89/10/09 21:28:13 rab Exp $ SPRITE (Berkeley)
#
# Allow mkmf

MACHINES	= ds3100 spur sun3 sun4
MAKEFILE	= Makefile
MANPAGES	= ln.man
NAME		= ln
SYSMAKEFILE	= command.mk
TM     	       ?= $(MACHINE)
TYPE		= sprite
DISTDIR        ?= /dist/sprite/src/cmds/ln

#if exists($(TM).md/md.mk)
#include	"$(TM).md/md.mk"
#endif

#if exists(local.mk)
#include	"local.mk"
#else
#include	<$(SYSMAKEFILE)>
#endif

#if exists($(TM).md/dependencies.mk)
#include	"$(TM).md/dependencies.mk"
#endif
