#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Sun Aug 11 17:48:06 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= devBlockDevice.c devConsoleCmd.c devDiskStats.c devExabyteTape.c devNet.c devNull.c devQueue.c devRawBlockDev.c devSCSI.c devSCSIDisk.c devSCSIHBA.c devSCSITape.c devScsiDevice.c devSyslog.c devTty.c ds3100.md/devConfig.c ds3100.md/devConsole.c ds3100.md/devDC7085.c ds3100.md/devFsOpTable.c ds3100.md/devGraphics.c ds3100.md/devInit.c ds3100.md/devSII.c ds3100.md/devScsiTapeConfig.c ds3100.md/devTtyAttach.c ds3100.md/font.c ds5000.md/devConfig.c ds5000.md/devConsole.c ds5000.md/devDC7085.c ds5000.md/devFsOpTable.c ds5000.md/devGraphics.c ds5000.md/devInit.c ds5000.md/devSCSIC90.c ds5000.md/devSCSIC90Mach.c ds5000.md/devScsiTapeConfig.c ds5000.md/devStdFB.c ds5000.md/devTtyAttach.c ds5000.md/font.c sun3.md/devConfig.c sun3.md/devConsole.c sun3.md/devEmulexTape.c sun3.md/devFsOpTable.c sun3.md/devGraphics.c sun3.md/devInit.c sun3.md/devJaguarHBA.c sun3.md/devMouse.c sun3.md/devSCSI0.c sun3.md/devSCSI3.c sun3.md/devScsiTapeConfig.c sun3.md/devSysgenTape.c sun3.md/devTMR.c sun3.md/devTtyAttach.c sun3.md/devVidSun3.s sun3.md/devXylogics450.c sun3.md/devZ8530.c sun4.md/devConfig.c sun4.md/devConsole.c sun4.md/devEmulexTape.c sun4.md/devFsOpTable.c sun4.md/devGraphics.c sun4.md/devInit.c sun4.md/devJaguarHBA.c sun4.md/devMouse.c sun4.md/devSCSI0.c sun4.md/devSCSI3.c sun4.md/devScsiTapeConfig.c sun4.md/devSysgenTape.c sun4.md/devTMR.c sun4.md/devTtyAttach.c sun4.md/devVidSun4.s sun4.md/devXylogics450.c sun4.md/devZ8530.c sun4c.md/devConfig.c sun4c.md/devConsole.c sun4c.md/devEmulexTape.c sun4c.md/devFsOpTable.c sun4c.md/devGraphics.c sun4c.md/devInit.c sun4c.md/devMouse.c sun4c.md/devSCSIC90.c sun4c.md/devSCSIC90Mach.c sun4c.md/devScsiTapeConfig.c sun4c.md/devTtyAttach.c sun4c.md/devVidSun4.s sun4c.md/devZ8530.c sun4c2.md/devConfig.c sun4c2.md/devConsole.c sun4c2.md/devEmulexTape.c sun4c2.md/devFsOpTable.c sun4c2.md/devGraphics.c sun4c2.md/devInit.c sun4c2.md/devMouse.c sun4c2.md/devSCSIC90.c sun4c2.md/devSCSIC90Mach.c sun4c2.md/devScsiTapeConfig.c sun4c2.md/devTtyAttach.c sun4c2.md/devVidSun4.s sun4c2.md/devZ8530.c symm.md/devConf.c symm.md/devConfScedDisk.c symm.md/devConfig.c symm.md/devConsole.c symm.md/devDynixBlockDevice.c symm.md/devFsOpTable.c symm.md/devIOConf.c symm.md/devSced.c symm.md/devScedConsole.c symm.md/devScedDisk.c symm.md/devScedTape.c symm.md/devStub.c symm.md/devTtyAttach.c symm.md/devVid.c symm.md/devZDCConf.c symm.md/devZDCDisk.c symm.md/devZDCInit.c
LINTLIBS	= dbg.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln libc.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 symm
MAKEFILE	= Makefile
NAME		= dev
PUBHDRS		= dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/dev

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
