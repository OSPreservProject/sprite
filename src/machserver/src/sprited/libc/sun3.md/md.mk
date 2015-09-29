#
# Prototype Makefile for machine-dependent directories.
#
# A file of this form resides in each ".md" subdirectory of a
# command.  Its name is typically "md.mk".  During makes in the
# parent directory, this file (or a similar file in a sibling
# subdirectory) is included to define machine-specific things
# such as additional source and object files.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.md
# Fri May 22 14:49:49 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= List_Init.c List_Insert.c List_ListInsert.c List_Move.c List_Remove.c compatMapCode.c status.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c Rpc_GetName.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c ckalloc.c ctypeBits.c errno.c fmt.c strerror.c timeConstants.c compatSig.c strncpy.c strtoul.c ttyDriver.c Stdio_Setup.c fflush.c fgetc.c fgets.c fputc.c fprintf.c fputs.c fscanf.c isinf.c isnan.c iszero.c sprintf.c sscanf.c ungetc.c vfprintf.c vfscanf.c
HDRS		= compatInt.h compatSig.h fileInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/List_Init.o sun3.md/List_Insert.o sun3.md/List_ListInsert.o sun3.md/List_Move.o sun3.md/List_Remove.o sun3.md/Net_AddrToString.o sun3.md/Net_EtherAddrToString.o sun3.md/Net_HostToNetInt.o sun3.md/Net_HostToNetShort.o sun3.md/Net_InetAddrToString.o sun3.md/Net_InetChecksum.o sun3.md/Net_NetToHostInt.o sun3.md/Net_NetToHostShort.o sun3.md/Rpc_GetName.o sun3.md/Stdio_Setup.o sun3.md/Time_Add.o sun3.md/Time_Divide.o sun3.md/Time_Multiply.o sun3.md/Time_Subtract.o sun3.md/Time_ToAscii.o sun3.md/Time_ToParts.o sun3.md/ckalloc.o sun3.md/compatMapCode.o sun3.md/compatSig.o sun3.md/ctypeBits.o sun3.md/errno.o sun3.md/fflush.o sun3.md/fgetc.o sun3.md/fgets.o sun3.md/fmt.o sun3.md/fprintf.o sun3.md/fputc.o sun3.md/fputs.o sun3.md/fscanf.o sun3.md/isinf.o sun3.md/isnan.o sun3.md/iszero.o sun3.md/sprintf.o sun3.md/sscanf.o sun3.md/status.o sun3.md/strerror.o sun3.md/strncpy.o sun3.md/strtoul.o sun3.md/timeConstants.o sun3.md/ttyDriver.o sun3.md/ungetc.o sun3.md/vfprintf.o sun3.md/vfscanf.o
CLEANOBJS	= sun3.md/List_Init.o sun3.md/List_Insert.o sun3.md/List_ListInsert.o sun3.md/List_Move.o sun3.md/List_Remove.o sun3.md/compatMapCode.o sun3.md/status.o sun3.md/Net_AddrToString.o sun3.md/Net_EtherAddrToString.o sun3.md/Net_HostToNetInt.o sun3.md/Net_HostToNetShort.o sun3.md/Net_InetAddrToString.o sun3.md/Net_InetChecksum.o sun3.md/Net_NetToHostInt.o sun3.md/Net_NetToHostShort.o sun3.md/Rpc_GetName.o sun3.md/Time_Add.o sun3.md/Time_Divide.o sun3.md/Time_Multiply.o sun3.md/Time_Subtract.o sun3.md/Time_ToAscii.o sun3.md/Time_ToParts.o sun3.md/ckalloc.o sun3.md/ctypeBits.o sun3.md/errno.o sun3.md/fmt.o sun3.md/strerror.o sun3.md/timeConstants.o sun3.md/compatSig.o sun3.md/strncpy.o sun3.md/strtoul.o sun3.md/ttyDriver.o sun3.md/Stdio_Setup.o sun3.md/fflush.o sun3.md/fgetc.o sun3.md/fgets.o sun3.md/fputc.o sun3.md/fprintf.o sun3.md/fputs.o sun3.md/fscanf.o sun3.md/isinf.o sun3.md/isnan.o sun3.md/iszero.o sun3.md/sprintf.o sun3.md/sscanf.o sun3.md/ungetc.o sun3.md/vfprintf.o sun3.md/vfscanf.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
