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
# Tue Jul  2 13:43:05 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/bcopy.c List_Init.c List_Insert.c List_Move.c List_Remove.c MemData.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c bcmp.c bzero.c fmt.c isinf.c sprintf.c status.c strcat.c strchr.c strcmp.c strcpy.c strlen.c strncmp.c strncpy.c strtoul.c timeConstants.c ttyDriver.c frexp.c isascii.c Stdio_Setup.c ctypeBits.c fflush.c fputc.c isspace.c vfprintf.c atof.c atoi.c fgetc.c sscanf.c ungetc.c vfscanf.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_InetHdrChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c fclose.c fgets.c fprintf.c fscanf.c isatty.c isnan.c iszero.c rename.c strcasecmp.c unlink.c errno.c strerror.c Quad_AddUns.c Quad_AddUnsLong.c Quad_CompareUns.c
HDRS		= fileInt.h memInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/List_Init.o ds3100.md/List_Insert.o ds3100.md/List_Move.o ds3100.md/List_Remove.o ds3100.md/MemData.o ds3100.md/Net_AddrToString.o ds3100.md/Net_EtherAddrToString.o ds3100.md/Net_HostToNetInt.o ds3100.md/Net_HostToNetShort.o ds3100.md/Net_InetAddrToString.o ds3100.md/Net_InetChecksum.o ds3100.md/Net_InetHdrChecksum.o ds3100.md/Net_NetToHostInt.o ds3100.md/Net_NetToHostShort.o ds3100.md/Quad_AddUns.o ds3100.md/Quad_AddUnsLong.o ds3100.md/Quad_CompareUns.o ds3100.md/Stdio_Setup.o ds3100.md/Time_Add.o ds3100.md/Time_Divide.o ds3100.md/Time_Multiply.o ds3100.md/Time_Subtract.o ds3100.md/Time_ToAscii.o ds3100.md/Time_ToParts.o ds3100.md/atof.o ds3100.md/atoi.o ds3100.md/bcmp.o ds3100.md/bcopy.o ds3100.md/bzero.o ds3100.md/ctypeBits.o ds3100.md/errno.o ds3100.md/fclose.o ds3100.md/fflush.o ds3100.md/fgetc.o ds3100.md/fgets.o ds3100.md/fmt.o ds3100.md/fprintf.o ds3100.md/fputc.o ds3100.md/frexp.o ds3100.md/fscanf.o ds3100.md/isascii.o ds3100.md/isatty.o ds3100.md/isinf.o ds3100.md/isnan.o ds3100.md/isspace.o ds3100.md/iszero.o ds3100.md/modf.o ds3100.md/rename.o ds3100.md/sprintf.o ds3100.md/sscanf.o ds3100.md/status.o ds3100.md/strcasecmp.o ds3100.md/strcat.o ds3100.md/strchr.o ds3100.md/strcmp.o ds3100.md/strcpy.o ds3100.md/strerror.o ds3100.md/strlen.o ds3100.md/strncmp.o ds3100.md/strncpy.o ds3100.md/strtoul.o ds3100.md/timeConstants.o ds3100.md/ttyDriver.o ds3100.md/ungetc.o ds3100.md/unlink.o ds3100.md/vfprintf.o ds3100.md/vfscanf.o
CLEANOBJS	= ds3100.md/bcopy.o ds3100.md/List_Init.o ds3100.md/List_Insert.o ds3100.md/List_Move.o ds3100.md/List_Remove.o ds3100.md/MemData.o ds3100.md/Time_Add.o ds3100.md/Time_Divide.o ds3100.md/Time_Multiply.o ds3100.md/Time_Subtract.o ds3100.md/Time_ToAscii.o ds3100.md/Time_ToParts.o ds3100.md/bcmp.o ds3100.md/bzero.o ds3100.md/fmt.o ds3100.md/isinf.o ds3100.md/sprintf.o ds3100.md/status.o ds3100.md/strcat.o ds3100.md/strchr.o ds3100.md/strcmp.o ds3100.md/strcpy.o ds3100.md/strlen.o ds3100.md/strncmp.o ds3100.md/strncpy.o ds3100.md/strtoul.o ds3100.md/timeConstants.o ds3100.md/ttyDriver.o ds3100.md/frexp.o ds3100.md/isascii.o ds3100.md/Stdio_Setup.o ds3100.md/ctypeBits.o ds3100.md/fflush.o ds3100.md/fputc.o ds3100.md/isspace.o ds3100.md/vfprintf.o ds3100.md/atof.o ds3100.md/atoi.o ds3100.md/fgetc.o ds3100.md/sscanf.o ds3100.md/ungetc.o ds3100.md/vfscanf.o ds3100.md/Net_AddrToString.o ds3100.md/Net_EtherAddrToString.o ds3100.md/Net_HostToNetInt.o ds3100.md/Net_HostToNetShort.o ds3100.md/Net_InetAddrToString.o ds3100.md/Net_InetChecksum.o ds3100.md/Net_InetHdrChecksum.o ds3100.md/Net_NetToHostInt.o ds3100.md/Net_NetToHostShort.o ds3100.md/fclose.o ds3100.md/fgets.o ds3100.md/fprintf.o ds3100.md/fscanf.o ds3100.md/isatty.o ds3100.md/isnan.o ds3100.md/iszero.o ds3100.md/rename.o ds3100.md/strcasecmp.o ds3100.md/unlink.o ds3100.md/errno.o ds3100.md/strerror.o ds3100.md/Quad_AddUns.o ds3100.md/Quad_AddUnsLong.o ds3100.md/Quad_CompareUns.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= ds3100.md/modf.o
