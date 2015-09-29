#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 19:04:23 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= vmBoot.c vmCOW.c vmList.c vmMap.c vmMigrate.c vmPage.c vmPrefetch.c vmSeg.c vmServer.c vmStack.c vmStubs.c vmSubr.c vmSwapDir.c vmSysCall.c vmTrace.c ds3100.md/vmPmax.c ds3100.md/vmPmaxAsm.s ds5000.md/vm3max.c ds5000.md/vm3maxAsm.s sun3.md/vmSun.c sun3.md/vmSunAsm.s sun4.md/vmSun.c sun4.md/vmSunAsm.s sun4c.md/vmSun.c sun4c.md/vmSunAsm.s sun4c2.md/vmSun.c sun4c2.md/vmSunAsm.s sun4nw.md/vmSun.c sun4nw.md/vmSunAsm.s symm.md/vmSymm.c symm.md/vmSymmAsm.s
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= vm
PUBHDRS		= vm.h vmHack.h vmSwapDir.h vmTrace.h vmUnixStubs.h
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/vm

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
