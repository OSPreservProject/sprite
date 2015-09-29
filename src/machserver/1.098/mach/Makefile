#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 17:55:01 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= ds3100.md/compatMapCode.c ds3100.md/compatSig.c ds3100.md/cvtStat.c ds3100.md/fcntl.c ds3100.md/ioctl.c ds3100.md/loMem.s ds3100.md/machAsm.s ds3100.md/machCode.c ds3100.md/machDis.c ds3100.md/machMigrate.c ds3100.md/machMon.c ds3100.md/machUNIXSyscall.c ds3100.md/signals.c ds3100.md/socket.c ds5000.md/compatMapCode.c ds5000.md/compatSig.c ds5000.md/cvtStat.c ds5000.md/ds3100.machAsm.s ds5000.md/ds3100.machCode.c ds5000.md/fcntl.c ds5000.md/ioctl.c ds5000.md/loMem.s ds5000.md/machAsm.s ds5000.md/machCode.c ds5000.md/machDis.c ds5000.md/machMigrate.c ds5000.md/machMon.c ds5000.md/machUNIXSyscall.c ds5000.md/signals.c ds5000.md/socket.c sun3.md/bootSysAsm.s sun3.md/machAsm.s sun3.md/machCode.c sun3.md/machEeprom.c sun3.md/machIntr.s sun3.md/machMigrate.c sun3.md/machMon.c sun3.md/machTrap.s sun3.md/machVector.s sun4.md/addsub.c sun4.md/bootSysAsm.s sun4.md/compare.c sun4.md/div.c sun4.md/fpu_simulator.c sun4.md/iu_simulator.c sun4.md/machAsm.s sun4.md/machCode.c sun4.md/machFPUGlue.c sun4.md/machIntr.s sun4.md/machMigrate.c sun4.md/machMon.c sun4.md/machTrace.s sun4.md/machTrap.s sun4.md/mul.c sun4.md/pack.c sun4.md/unpack.c sun4.md/utility.c sun4.md/uword.c sun4c.md/addsub.c sun4c.md/bootSysAsm.s sun4c.md/compare.c sun4c.md/div.c sun4c.md/fpu_simulator.c sun4c.md/iu_simulator.c sun4c.md/machAsm.s sun4c.md/machCode.c sun4c.md/machFPUGlue.c sun4c.md/machIntr.s sun4c.md/machMigrate.c sun4c.md/machMon.c sun4c.md/machTrap.s sun4c.md/mul.c sun4c.md/pack.c sun4c.md/unpack.c sun4c.md/utility.c sun4c.md/uword.c sun4c2.md/addsub.c sun4c2.md/bootSysAsm.s sun4c2.md/compare.c sun4c2.md/div.c sun4c2.md/fpu_simulator.c sun4c2.md/iu_simulator.c sun4c2.md/machAsm.s sun4c2.md/machCode.c sun4c2.md/machFPUGlue.c sun4c2.md/machIntr.s sun4c2.md/machMigrate.c sun4c2.md/machMon.c sun4c2.md/machTrap.s sun4c2.md/mul.c sun4c2.md/pack.c sun4c2.md/unpack.c sun4c2.md/utility.c sun4c2.md/uword.c symm.md/machArchdep.c symm.md/machAutoconf.c symm.md/machCode.c symm.md/machConsio.c symm.md/machCswitch.s symm.md/machGenAsmSymbols.c symm.md/machLocore.s symm.md/machMRegs.s symm.md/machMemory.c symm.md/machMigrate.c symm.md/machMisc.s symm.md/machMon.c symm.md/machMutex.s symm.md/machSlic.c symm.md/machStart.s symm.md/machStub.c symm.md/machSyscallTrace.c symm.md/machTrap.c
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 symm
MAKEFILE	= Makefile
NAME		= mach
PUBHDRS		= 
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/mach

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
