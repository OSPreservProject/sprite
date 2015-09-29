#
# Prototype makefile for libraries with multiple source directories
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.biglib
# Fri Aug 10 16:25:12 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.biglib,v 1.15 89/10/09 21:28:28 rab Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= Sync_SlowLock.c Sync_SlowWait.c ds3100.md/Sync_GetLock.s ds3100.md/Sync_Unlock.s spur.md/Sync_GetLock.s spur.md/Sync_Unlock.s sun3.md/Sync_GetLock.s sun3.md/Sync_Unlock.s sun4.md/Sync_GetLock.s sun4.md/Sync_Unlock.s symm.md/Sync_GetLock.s symm.md/Sync_Unlock.s
LINTSRCS	= 
MACHINES	= ds3100 spur sun3 sun4 symm
MAKEFILE	= Makefile
MANPAGES	= Sync.man Sync_Broadcast.man Sync_Wait.man Sync_WaitTime.man
NAME		= c
PUBHDRS		= 
SUBDIR		= sync
TM     	        ?= $(MACHINE)
TYPE		= sprite
DISTDIR         ?= /dist/sprite/src/lib/c/sync

SYSMAKEFILE	= biglib.mk

#if exists($(TM).md/md.mk)
#include	"$(TM).md/md.mk"
#endif

#if exists(local.mk)
#include 	"local.mk"
#else
#include	<$(SYSMAKEFILE)>
#endif

#if exists($(DEPFILE))
#include	"$(DEPFILE)"
#endif
