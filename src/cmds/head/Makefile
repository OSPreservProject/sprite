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
# Tue Oct  3 16:55:58 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.command,v 1.7 89/08/18 11:21:38 douglis Exp Locker: rab $ SPRITE (Berkeley)
#
# Allow mkmf

MACHINES	= spur sun3 sun4
MAKEFILE	= Makefile
MANPAGES	= head.man
NAME		= head
SYSMAKEFILE	= command.mk
TM     	       ?= $(MACHINE)
TYPE		= sprite
DISTDIR        ?= /dist/sprite/src/cmds/head

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
