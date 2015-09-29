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
# Tue Jul  2 13:44:11 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/_dtou.s sun4.md/alloca.s sun4.md/compatMapCode.c sun4.md/_fixunsdfsi.s sun4.md/compatSig.c sun4.md/ptr_call.s sun4.md/stret1.s sun4.md/stret2.s sun4.md/stret4.s sun4.md/umultiply.s sun4.md/modf.s sun4.md/divide.s sun4.md/multiply.s sun4.md/rem.s sun4.md/varargs.s List_Init.c List_Insert.c List_Move.c List_Remove.c MemData.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c bcmp.c bzero.c fmt.c isinf.c sprintf.c status.c strcat.c strchr.c strcmp.c strcpy.c strlen.c strncmp.c strncpy.c strtoul.c timeConstants.c ttyDriver.c frexp.c isascii.c Stdio_Setup.c ctypeBits.c fflush.c fputc.c isspace.c vfprintf.c atof.c atoi.c fgetc.c sscanf.c ungetc.c vfscanf.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_InetHdrChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c fclose.c fgets.c fprintf.c fscanf.c isatty.c isnan.c iszero.c rename.c strcasecmp.c unlink.c errno.c strerror.c Quad_AddUns.c Quad_AddUnsLong.c Quad_CompareUns.c
HDRS		= sun4.md/compatSig.h fileInt.h memInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/List_Init.o sun4.md/List_Insert.o sun4.md/List_Move.o sun4.md/List_Remove.o sun4.md/MemData.o sun4.md/Net_AddrToString.o sun4.md/Net_EtherAddrToString.o sun4.md/Net_HostToNetInt.o sun4.md/Net_HostToNetShort.o sun4.md/Net_InetAddrToString.o sun4.md/Net_InetChecksum.o sun4.md/Net_InetHdrChecksum.o sun4.md/Net_NetToHostInt.o sun4.md/Net_NetToHostShort.o sun4.md/Quad_AddUns.o sun4.md/Quad_AddUnsLong.o sun4.md/Quad_CompareUns.o sun4.md/Stdio_Setup.o sun4.md/Time_Add.o sun4.md/Time_Divide.o sun4.md/Time_Multiply.o sun4.md/Time_Subtract.o sun4.md/Time_ToAscii.o sun4.md/Time_ToParts.o sun4.md/_dtou.o sun4.md/_fixunsdfsi.o sun4.md/alloca.o sun4.md/atof.o sun4.md/atoi.o sun4.md/bcmp.o sun4.md/bzero.o sun4.md/compatMapCode.o sun4.md/compatSig.o sun4.md/ctypeBits.o sun4.md/divide.o sun4.md/errno.o sun4.md/fclose.o sun4.md/fflush.o sun4.md/fgetc.o sun4.md/fgets.o sun4.md/fmt.o sun4.md/fprintf.o sun4.md/fputc.o sun4.md/frexp.o sun4.md/fscanf.o sun4.md/isascii.o sun4.md/isatty.o sun4.md/isinf.o sun4.md/isnan.o sun4.md/isspace.o sun4.md/iszero.o sun4.md/modf.o sun4.md/multiply.o sun4.md/ptr_call.o sun4.md/rem.o sun4.md/rename.o sun4.md/sprintf.o sun4.md/sscanf.o sun4.md/status.o sun4.md/strcasecmp.o sun4.md/strcat.o sun4.md/strchr.o sun4.md/strcmp.o sun4.md/strcpy.o sun4.md/strerror.o sun4.md/stret1.o sun4.md/stret2.o sun4.md/stret4.o sun4.md/strlen.o sun4.md/strncmp.o sun4.md/strncpy.o sun4.md/strtoul.o sun4.md/timeConstants.o sun4.md/ttyDriver.o sun4.md/umultiply.o sun4.md/ungetc.o sun4.md/unlink.o sun4.md/varargs.o sun4.md/vfprintf.o sun4.md/vfscanf.o
CLEANOBJS	= sun4.md/_dtou.o sun4.md/alloca.o sun4.md/compatMapCode.o sun4.md/_fixunsdfsi.o sun4.md/compatSig.o sun4.md/ptr_call.o sun4.md/stret1.o sun4.md/stret2.o sun4.md/stret4.o sun4.md/umultiply.o sun4.md/modf.o sun4.md/divide.o sun4.md/multiply.o sun4.md/rem.o sun4.md/varargs.o sun4.md/List_Init.o sun4.md/List_Insert.o sun4.md/List_Move.o sun4.md/List_Remove.o sun4.md/MemData.o sun4.md/Time_Add.o sun4.md/Time_Divide.o sun4.md/Time_Multiply.o sun4.md/Time_Subtract.o sun4.md/Time_ToAscii.o sun4.md/Time_ToParts.o sun4.md/bcmp.o sun4.md/bzero.o sun4.md/fmt.o sun4.md/isinf.o sun4.md/sprintf.o sun4.md/status.o sun4.md/strcat.o sun4.md/strchr.o sun4.md/strcmp.o sun4.md/strcpy.o sun4.md/strlen.o sun4.md/strncmp.o sun4.md/strncpy.o sun4.md/strtoul.o sun4.md/timeConstants.o sun4.md/ttyDriver.o sun4.md/frexp.o sun4.md/isascii.o sun4.md/Stdio_Setup.o sun4.md/ctypeBits.o sun4.md/fflush.o sun4.md/fputc.o sun4.md/isspace.o sun4.md/vfprintf.o sun4.md/atof.o sun4.md/atoi.o sun4.md/fgetc.o sun4.md/sscanf.o sun4.md/ungetc.o sun4.md/vfscanf.o sun4.md/Net_AddrToString.o sun4.md/Net_EtherAddrToString.o sun4.md/Net_HostToNetInt.o sun4.md/Net_HostToNetShort.o sun4.md/Net_InetAddrToString.o sun4.md/Net_InetChecksum.o sun4.md/Net_InetHdrChecksum.o sun4.md/Net_NetToHostInt.o sun4.md/Net_NetToHostShort.o sun4.md/fclose.o sun4.md/fgets.o sun4.md/fprintf.o sun4.md/fscanf.o sun4.md/isatty.o sun4.md/isnan.o sun4.md/iszero.o sun4.md/rename.o sun4.md/strcasecmp.o sun4.md/unlink.o sun4.md/errno.o sun4.md/strerror.o sun4.md/Quad_AddUns.o sun4.md/Quad_AddUnsLong.o sun4.md/Quad_CompareUns.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
