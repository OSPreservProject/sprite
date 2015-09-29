#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 19:01:54 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= timerClock.c timerQueue.c timerStubs.c ds3100.md/timerMC.c ds3100.md/timerTick.c ds5000.md/timerMC.c ds5000.md/timerTick.c sun3.md/timerIntersil.c sun3.md/timerTick.c sun4.md/timerIntersil.c sun4.md/timerTick.c sun4c.md/timerSun4c.c sun4c.md/timerTick.c sun4c2.md/timerSun4c.c sun4c2.md/timerTick.c sun4nw.md/timerIntersil.c sun4nw.md/timerTick.c symm.md/timerSymm.c symm.md/timerTick.c
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= timer
PUBHDRS		= timer.h timerUnixStubs.h
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/timer

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
