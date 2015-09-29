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
# Wed May  8 17:22:05 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.command,v 1.9 91/04/11 12:55:33 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

MACHINES	= ds3100 sun3 sun4
MAKEFILE	= Makefile
MANPAGES	= indent.man
NAME		= indent
SYSMAKEFILE	= command.mk
#include	<settm.mk>
TYPE		= sprite
DISTDIR        ?= /dist/sprite/src/cmds/indent

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
