#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Fri Nov  9 13:11:36 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.17 90/03/12 23:28:19 jhh Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= bitvec.c bxor.c debugMem.c devDebug.c devPrint.c devRaid.c devRaidDisk.c devRaidHardInit.c devRaidIOC.c devRaidInitiate.c devRaidLock.c devRaidLog.c devRaidMap.c devRaidParityCheck.c devRaidReconstruct.c devRaidUtil.c semaphore.c strUtil.c
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= sun4
MAKEFILE	= Makefile
NAME		= raid
PUBHDRS		= raidExt.h
SYSMAKEFILE	= kernel.mk
TM     	        ?= sun4
DISTDIR         ?= /dist/sprite/src/kernel/raid

#if exists($(HOME)/kernel.mk)
#include "$(HOME)/kernel.mk"
#endif

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
