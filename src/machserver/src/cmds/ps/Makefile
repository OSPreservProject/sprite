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
# Thu Feb 13 18:46:01 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.command,v 1.9 91/04/11 12:55:33 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

MACHINES	= ds3100
MAKEFILE	= Makefile
MANPAGES	= 
NAME		= ps
SYSMAKEFILE	= command.mk
#include	<settm.mk>
TYPE		= sprite
DISTDIR        ?= 

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
