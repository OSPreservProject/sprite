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
# Wed Jun  3 15:17:04 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= List_Init.c List_Insert.c List_ListInsert.c List_Move.c List_Remove.c compatMapCode.c status.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c Rpc_GetName.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c ckalloc.c ctypeBits.c errno.c fmt.c strerror.c timeConstants.c compatSig.c strncpy.c strtoul.c ttyDriver.c Stdio_Setup.c fflush.c fgetc.c fgets.c fputc.c fprintf.c fputs.c fscanf.c isinf.c isnan.c iszero.c sprintf.c sscanf.c ungetc.c vfprintf.c vfscanf.c
HDRS		= compatInt.h compatSig.h fileInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/List_Init.o ds3100.md/List_Insert.o ds3100.md/List_ListInsert.o ds3100.md/List_Move.o ds3100.md/List_Remove.o ds3100.md/Net_AddrToString.o ds3100.md/Net_EtherAddrToString.o ds3100.md/Net_HostToNetInt.o ds3100.md/Net_HostToNetShort.o ds3100.md/Net_InetAddrToString.o ds3100.md/Net_InetChecksum.o ds3100.md/Net_NetToHostInt.o ds3100.md/Net_NetToHostShort.o ds3100.md/Rpc_GetName.o ds3100.md/Stdio_Setup.o ds3100.md/Time_Add.o ds3100.md/Time_Divide.o ds3100.md/Time_Multiply.o ds3100.md/Time_Subtract.o ds3100.md/Time_ToAscii.o ds3100.md/Time_ToParts.o ds3100.md/ckalloc.o ds3100.md/compatMapCode.o ds3100.md/compatSig.o ds3100.md/ctypeBits.o ds3100.md/errno.o ds3100.md/fflush.o ds3100.md/fgetc.o ds3100.md/fgets.o ds3100.md/fmt.o ds3100.md/fprintf.o ds3100.md/fputc.o ds3100.md/fputs.o ds3100.md/fscanf.o ds3100.md/isinf.o ds3100.md/isnan.o ds3100.md/iszero.o ds3100.md/sprintf.o ds3100.md/sscanf.o ds3100.md/status.o ds3100.md/strerror.o ds3100.md/strncpy.o ds3100.md/strtoul.o ds3100.md/timeConstants.o ds3100.md/ttyDriver.o ds3100.md/ungetc.o ds3100.md/vfprintf.o ds3100.md/vfscanf.o
CLEANOBJS	= ds3100.md/List_Init.o ds3100.md/List_Insert.o ds3100.md/List_ListInsert.o ds3100.md/List_Move.o ds3100.md/List_Remove.o ds3100.md/compatMapCode.o ds3100.md/status.o ds3100.md/Net_AddrToString.o ds3100.md/Net_EtherAddrToString.o ds3100.md/Net_HostToNetInt.o ds3100.md/Net_HostToNetShort.o ds3100.md/Net_InetAddrToString.o ds3100.md/Net_InetChecksum.o ds3100.md/Net_NetToHostInt.o ds3100.md/Net_NetToHostShort.o ds3100.md/Rpc_GetName.o ds3100.md/Time_Add.o ds3100.md/Time_Divide.o ds3100.md/Time_Multiply.o ds3100.md/Time_Subtract.o ds3100.md/Time_ToAscii.o ds3100.md/Time_ToParts.o ds3100.md/ckalloc.o ds3100.md/ctypeBits.o ds3100.md/errno.o ds3100.md/fmt.o ds3100.md/strerror.o ds3100.md/timeConstants.o ds3100.md/compatSig.o ds3100.md/strncpy.o ds3100.md/strtoul.o ds3100.md/ttyDriver.o ds3100.md/Stdio_Setup.o ds3100.md/fflush.o ds3100.md/fgetc.o ds3100.md/fgets.o ds3100.md/fputc.o ds3100.md/fprintf.o ds3100.md/fputs.o ds3100.md/fscanf.o ds3100.md/isinf.o ds3100.md/isnan.o ds3100.md/iszero.o ds3100.md/sprintf.o ds3100.md/sscanf.o ds3100.md/ungetc.o ds3100.md/vfprintf.o ds3100.md/vfscanf.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
