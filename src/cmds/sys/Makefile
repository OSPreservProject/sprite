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
# Tue Oct  3 16:59:52 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.command,v 1.7 89/08/18 11:21:38 douglis Exp Locker: rab $ SPRITE (Berkeley)
#
# Allow mkmf

MACHINES	= sun3
MAKEFILE	= Makefile
MANPAGES	= 
NAME		= sys
SYSMAKEFILE	= command.mk
TM     	       ?= sun3
TYPE		= sprite
DISTDIR        ?= /dist/sprite/src/cmds/sys

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
