#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 18:51:00 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= profMigrate.c profProfil.c ds3100.md/_mcount.c ds3100.md/profStack.s ds3100.md/profSubr.c ds5000.md/_mcount.c ds5000.md/profStack.s ds5000.md/profSubr.c sun3.md/_mcount.c sun3.md/profStack.s sun3.md/profSubr.c sun4.md/_mcount.c sun4.md/mcount.s sun4.md/profStack.s sun4.md/profSubr.c sun4c.md/_mcount.c sun4c.md/mcount.s sun4c.md/profStack.s sun4c.md/profSubr.c sun4c2.md/_mcount.c sun4c2.md/mcount.s sun4c2.md/profStack.s sun4c2.md/profSubr.c sun4nw.md/_mcount.c sun4nw.md/profStack.s sun4nw.md/profSubr.c symm.md/profStub.c
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= prof
PUBHDRS		= prof.h
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/prof

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
