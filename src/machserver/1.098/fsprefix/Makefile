#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 18:25:56 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= fsprefixOps.c
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= fsprefix
PUBHDRS		= fsprefix.h
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/fsprefix

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
