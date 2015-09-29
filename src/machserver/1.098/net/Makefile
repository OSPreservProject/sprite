#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 18:44:47 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= netArp.c netCode.c netEther.c netRoute.c ds3100.md/netConfig.c ds3100.md/netLE.c ds3100.md/netLERecv.c ds3100.md/netLEXmit.c ds5000.md/netConfig.c ds5000.md/netLE.c ds5000.md/netLEMach.c ds5000.md/netLERecv.c ds5000.md/netLEXmit.c sun3.md/netConfig.c sun3.md/netIE.c sun3.md/netIECmd.c sun3.md/netIEMem.c sun3.md/netIERecv.c sun3.md/netIESubr.c sun3.md/netIEXmit.c sun3.md/netLE.c sun3.md/netLEMach.c sun3.md/netLERecv.c sun3.md/netLEXmit.c sun3.md/netUltra.c sun4.md/netConfig.c sun4.md/netIE.c sun4.md/netIECmd.c sun4.md/netIEMem.c sun4.md/netIERecv.c sun4.md/netIESubr.c sun4.md/netIEXmit.c sun4.md/netUltra.c sun4c.md/netConfig.c sun4c.md/netLE.c sun4c.md/netLEMach.c sun4c.md/netLERecv.c sun4c.md/netLEXmit.c sun4c2.md/netConfig.c sun4c2.md/netLE.c sun4c2.md/netLEMach.c sun4c2.md/netLERecv.c sun4c2.md/netLEXmit.c sun4nw.md/netConfig.c sun4nw.md/netIE.c sun4nw.md/netIECmd.c sun4nw.md/netIEMem.c sun4nw.md/netIERecv.c sun4nw.md/netIESubr.c sun4nw.md/netIEXmit.c symm.md/netConfig.c symm.md/netSE.c symm.md/netSEXmit.c symm.md/netScedEther.c
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= net
PUBHDRS		= net.h netArp.h netInterface.h netRoute.h netTypes.h
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/net

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
