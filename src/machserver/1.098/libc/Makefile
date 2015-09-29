#
# Prototype Makefile for kernel modules.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.kernel
# Mon Jul 29 18:36:42 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.kernel,v 1.18 91/04/11 12:55:14 kupfer Exp $ SPRITE (Berkeley)
#
# Allow mkmf

ALLSRCS		= List_Init.c List_Insert.c List_Move.c List_Remove.c MemData.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_InetHdrChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c Quad_AddUns.c Quad_AddUnsLong.c Quad_CompareUns.c Stdio_Setup.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c atof.c atoi.c bcmp.c bzero.c ctypeBits.c errno.c fclose.c fflush.c fgetc.c fgets.c fmt.c fprintf.c fputc.c frexp.c fscanf.c isascii.c isatty.c isinf.c isnan.c isspace.c iszero.c rename.c sprintf.c sscanf.c status.c strcasecmp.c strcat.c strchr.c strcmp.c strcpy.c strerror.c strlen.c strncmp.c strncpy.c strtoul.c timeConstants.c ttyDriver.c ungetc.c unlink.c vfprintf.c vfscanf.c ds3100.md/bcopy.c ds5000.md/bcopy.c sun3.md/Faintd.s sun3.md/_adddf3.s sun3.md/_addsf3.s sun3.md/_ashlsi3.s sun3.md/_ashrsi3.s sun3.md/_cmpdf2.s sun3.md/_cmpdi2.s sun3.md/_cmpsf2.s sun3.md/_divdf3.s sun3.md/_divsf3.s sun3.md/_divsi3.s sun3.md/_extendsfdf2.s sun3.md/_fixdfdi.s sun3.md/_fixdfsi.s sun3.md/_fixunsdfdi.s sun3.md/_fixunsdfsi.s sun3.md/_floatdidf.s sun3.md/_floatsidf.s sun3.md/_lshlsi3.s sun3.md/_lshrsi3.s sun3.md/_modsi3.s sun3.md/_muldf3.s sun3.md/_mulsf3.s sun3.md/_mulsi3.s sun3.md/_negdf2.s sun3.md/_negsf2.s sun3.md/_subdf3.s sun3.md/_subsf3.s sun3.md/_truncdfsf2.s sun3.md/_ucmpdi2.s sun3.md/_udivsi3.s sun3.md/_umodsi3.s sun3.md/_umulsi3.s sun3.md/bcopy.c sun3.md/compatMapCode.c sun3.md/compatSig.c sun3.md/fp_globals.s sun3.md/modf.s sun4.md/_dtou.s sun4.md/_fixunsdfsi.s sun4.md/alloca.s sun4.md/compatMapCode.c sun4.md/compatSig.c sun4.md/divide.s sun4.md/modf.s sun4.md/multiply.s sun4.md/ptr_call.s sun4.md/rem.s sun4.md/stret1.s sun4.md/stret2.s sun4.md/stret4.s sun4.md/umultiply.s sun4.md/varargs.s sun4c.md/_dtou.s sun4c.md/_fixunsdfsi.s sun4c.md/alloca.s sun4c.md/compatMapCode.c sun4c.md/compatSig.c sun4c.md/divide.s sun4c.md/modf.s sun4c.md/multiply.s sun4c.md/ptr_call.s sun4c.md/rem.s sun4c.md/stret1.s sun4c.md/stret2.s sun4c.md/stret4.s sun4c.md/umultiply.s sun4c.md/varargs.s sun4c2.md/_dtou.s sun4c2.md/_fixunsdfsi.s sun4c2.md/alloca.s sun4c2.md/compatMapCode.c sun4c2.md/compatSig.c sun4c2.md/divide.s sun4c2.md/modf.s sun4c2.md/multiply.s sun4c2.md/ptr_call.s sun4c2.md/rem.s sun4c2.md/stret1.s sun4c2.md/stret2.s sun4c2.md/stret4.s sun4c2.md/umultiply.s sun4c2.md/varargs.s sun4nw.md/_dtou.s sun4nw.md/_fixunsdfsi.s sun4nw.md/alloca.s sun4nw.md/compatMapCode.c sun4nw.md/compatSig.c sun4nw.md/divide.s sun4nw.md/modf.s sun4nw.md/multiply.s sun4nw.md/ptr_call.s sun4nw.md/rem.s sun4nw.md/stret1.s sun4nw.md/stret2.s sun4nw.md/stret4.s sun4nw.md/umultiply.s sun4nw.md/varargs.s
LINTLIBS	= dbg.ln dev.ln fs.ln fscache.ln fsconsist.ln fsdm.ln fsio.ln \
		  fslcl.ln fspdev.ln fsprefix.ln fsrmt.ln fsutil.ln \
		  main.ln mem.ln net.ln proc.ln prof.ln mach.ln rpc.ln \
		  sched.ln sys.ln sync.ln timer.ln utils.ln vm.ln 

MACHINES	= ds3100 ds5000 sun3 sun4 sun4c sun4c2 sun4nw symm
MAKEFILE	= Makefile
NAME		= libc
PUBHDRS		= 
SYSMAKEFILE	= kernel.mk
#include	<settm.mk>
DISTDIR         ?= /dist/sprite/src/kernel/libc

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
