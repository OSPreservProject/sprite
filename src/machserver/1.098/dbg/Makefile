#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Fri Aug  2 17:42:46 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= ds3100.md/dbgAsm.s ds3100.md/dbgDis.c ds3100.md/dbgIP.c ds3100.md/dbgMain.c ds3100.md/dbgMainDbx.c ds5000.md/dbgAsm.s ds5000.md/dbgDis.c ds5000.md/dbgIP.c ds5000.md/dbgMain.c ds5000.md/dbgMainDbx.c sun3.md/dbgIP.c sun3.md/dbgMain.c sun3.md/dbgTrap.s sun4.md/dbgIP.c sun4.md/dbgMain.c sun4c.md/dbgIP.c sun4c.md/dbgMain.c sun4c2.md/dbgIP.c sun4c2.md/dbgMain.c sun4nw.md/dbgIP.c sun4nw.md/dbgMain.c symm.md/dbgMain.c symm.md/dbgStub.c
LINTLIBS	= dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= dbg
PUBHDRS		= 
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/dbg

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
