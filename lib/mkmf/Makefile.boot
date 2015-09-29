#
# Prototype Makefile for boot/* directories, the bootstrap
# programs that load kernel images.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from @(TEMPLATE)
# @(DATE)
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.boot,v 1.7 92/06/10 13:04:48 jhh Exp $ SPRITE (Berkeley)
#
# Allow mkmf

#
# Initialize machine independent things
#
MACHINES	= @(MACHINES)
MAKEFILE	= @(MAKEFILE)
MANPAGES	= @(MANPAGES)
NAME		= @(NAME)
SYSMAKEFILE	= boot.mk
TYPE		= @(TYPE)
DISTDIR        ?= @(DISTDIR)
#include	<settm.mk>


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
