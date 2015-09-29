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
# Tue Jul  2 13:44:31 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/compatMapCode.c sun4c.md/compatSig.c sun4c.md/divide.s sun4c.md/modf.s sun4c.md/multiply.s sun4c.md/rem.s sun4c.md/varargs.s sun4c.md/_dtou.s sun4c.md/_fixunsdfsi.s sun4c.md/alloca.s sun4c.md/ptr_call.s sun4c.md/stret1.s sun4c.md/stret2.s sun4c.md/stret4.s sun4c.md/umultiply.s List_Init.c List_Insert.c List_Move.c List_Remove.c MemData.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c bcmp.c bzero.c fmt.c isinf.c sprintf.c status.c strcat.c strchr.c strcmp.c strcpy.c strlen.c strncmp.c strncpy.c strtoul.c timeConstants.c ttyDriver.c frexp.c isascii.c Stdio_Setup.c ctypeBits.c fflush.c fputc.c isspace.c vfprintf.c atof.c atoi.c fgetc.c sscanf.c ungetc.c vfscanf.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_InetHdrChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c fclose.c fgets.c fprintf.c fscanf.c isatty.c isnan.c iszero.c rename.c strcasecmp.c unlink.c errno.c strerror.c Quad_AddUns.c Quad_AddUnsLong.c Quad_CompareUns.c
HDRS		= sun4c.md/compatSig.h fileInt.h memInt.h
MDPUBHDRS	= 
OBJS		= sun4c.md/List_Init.o sun4c.md/List_Insert.o sun4c.md/List_Move.o sun4c.md/List_Remove.o sun4c.md/MemData.o sun4c.md/Net_AddrToString.o sun4c.md/Net_EtherAddrToString.o sun4c.md/Net_HostToNetInt.o sun4c.md/Net_HostToNetShort.o sun4c.md/Net_InetAddrToString.o sun4c.md/Net_InetChecksum.o sun4c.md/Net_InetHdrChecksum.o sun4c.md/Net_NetToHostInt.o sun4c.md/Net_NetToHostShort.o sun4c.md/Quad_AddUns.o sun4c.md/Quad_AddUnsLong.o sun4c.md/Quad_CompareUns.o sun4c.md/Stdio_Setup.o sun4c.md/Time_Add.o sun4c.md/Time_Divide.o sun4c.md/Time_Multiply.o sun4c.md/Time_Subtract.o sun4c.md/Time_ToAscii.o sun4c.md/Time_ToParts.o sun4c.md/_dtou.o sun4c.md/_fixunsdfsi.o sun4c.md/alloca.o sun4c.md/atof.o sun4c.md/atoi.o sun4c.md/bcmp.o sun4c.md/bzero.o sun4c.md/compatMapCode.o sun4c.md/compatSig.o sun4c.md/ctypeBits.o sun4c.md/divide.o sun4c.md/errno.o sun4c.md/fclose.o sun4c.md/fflush.o sun4c.md/fgetc.o sun4c.md/fgets.o sun4c.md/fmt.o sun4c.md/fprintf.o sun4c.md/fputc.o sun4c.md/frexp.o sun4c.md/fscanf.o sun4c.md/isascii.o sun4c.md/isatty.o sun4c.md/isinf.o sun4c.md/isnan.o sun4c.md/isspace.o sun4c.md/iszero.o sun4c.md/modf.o sun4c.md/multiply.o sun4c.md/ptr_call.o sun4c.md/rem.o sun4c.md/rename.o sun4c.md/sprintf.o sun4c.md/sscanf.o sun4c.md/status.o sun4c.md/strcasecmp.o sun4c.md/strcat.o sun4c.md/strchr.o sun4c.md/strcmp.o sun4c.md/strcpy.o sun4c.md/strerror.o sun4c.md/stret1.o sun4c.md/stret2.o sun4c.md/stret4.o sun4c.md/strlen.o sun4c.md/strncmp.o sun4c.md/strncpy.o sun4c.md/strtoul.o sun4c.md/timeConstants.o sun4c.md/ttyDriver.o sun4c.md/umultiply.o sun4c.md/ungetc.o sun4c.md/unlink.o sun4c.md/varargs.o sun4c.md/vfprintf.o sun4c.md/vfscanf.o
CLEANOBJS	= sun4c.md/compatMapCode.o sun4c.md/compatSig.o sun4c.md/divide.o sun4c.md/modf.o sun4c.md/multiply.o sun4c.md/rem.o sun4c.md/varargs.o sun4c.md/_dtou.o sun4c.md/_fixunsdfsi.o sun4c.md/alloca.o sun4c.md/ptr_call.o sun4c.md/stret1.o sun4c.md/stret2.o sun4c.md/stret4.o sun4c.md/umultiply.o sun4c.md/List_Init.o sun4c.md/List_Insert.o sun4c.md/List_Move.o sun4c.md/List_Remove.o sun4c.md/MemData.o sun4c.md/Time_Add.o sun4c.md/Time_Divide.o sun4c.md/Time_Multiply.o sun4c.md/Time_Subtract.o sun4c.md/Time_ToAscii.o sun4c.md/Time_ToParts.o sun4c.md/bcmp.o sun4c.md/bzero.o sun4c.md/fmt.o sun4c.md/isinf.o sun4c.md/sprintf.o sun4c.md/status.o sun4c.md/strcat.o sun4c.md/strchr.o sun4c.md/strcmp.o sun4c.md/strcpy.o sun4c.md/strlen.o sun4c.md/strncmp.o sun4c.md/strncpy.o sun4c.md/strtoul.o sun4c.md/timeConstants.o sun4c.md/ttyDriver.o sun4c.md/frexp.o sun4c.md/isascii.o sun4c.md/Stdio_Setup.o sun4c.md/ctypeBits.o sun4c.md/fflush.o sun4c.md/fputc.o sun4c.md/isspace.o sun4c.md/vfprintf.o sun4c.md/atof.o sun4c.md/atoi.o sun4c.md/fgetc.o sun4c.md/sscanf.o sun4c.md/ungetc.o sun4c.md/vfscanf.o sun4c.md/Net_AddrToString.o sun4c.md/Net_EtherAddrToString.o sun4c.md/Net_HostToNetInt.o sun4c.md/Net_HostToNetShort.o sun4c.md/Net_InetAddrToString.o sun4c.md/Net_InetChecksum.o sun4c.md/Net_InetHdrChecksum.o sun4c.md/Net_NetToHostInt.o sun4c.md/Net_NetToHostShort.o sun4c.md/fclose.o sun4c.md/fgets.o sun4c.md/fprintf.o sun4c.md/fscanf.o sun4c.md/isatty.o sun4c.md/isnan.o sun4c.md/iszero.o sun4c.md/rename.o sun4c.md/strcasecmp.o sun4c.md/unlink.o sun4c.md/errno.o sun4c.md/strerror.o sun4c.md/Quad_AddUns.o sun4c.md/Quad_AddUnsLong.o sun4c.md/Quad_CompareUns.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
