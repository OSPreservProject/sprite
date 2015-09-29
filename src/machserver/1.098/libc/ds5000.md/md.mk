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
# Tue Jul  2 13:43:23 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/bcopy.c List_Init.c List_Insert.c List_Move.c List_Remove.c MemData.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c bcmp.c bzero.c fmt.c isinf.c sprintf.c status.c strcat.c strchr.c strcmp.c strcpy.c strlen.c strncmp.c strncpy.c strtoul.c timeConstants.c ttyDriver.c frexp.c isascii.c Stdio_Setup.c ctypeBits.c fflush.c fputc.c isspace.c vfprintf.c atof.c atoi.c fgetc.c sscanf.c ungetc.c vfscanf.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrToString.c Net_InetChecksum.c Net_InetHdrChecksum.c Net_NetToHostInt.c Net_NetToHostShort.c fclose.c fgets.c fprintf.c fscanf.c isatty.c isnan.c iszero.c rename.c strcasecmp.c unlink.c errno.c strerror.c Quad_AddUns.c Quad_AddUnsLong.c Quad_CompareUns.c
HDRS		= fileInt.h memInt.h
MDPUBHDRS	= 
OBJS		= ds5000.md/List_Init.o ds5000.md/List_Insert.o ds5000.md/List_Move.o ds5000.md/List_Remove.o ds5000.md/MemData.o ds5000.md/Net_AddrToString.o ds5000.md/Net_EtherAddrToString.o ds5000.md/Net_HostToNetInt.o ds5000.md/Net_HostToNetShort.o ds5000.md/Net_InetAddrToString.o ds5000.md/Net_InetChecksum.o ds5000.md/Net_InetHdrChecksum.o ds5000.md/Net_NetToHostInt.o ds5000.md/Net_NetToHostShort.o ds5000.md/Quad_AddUns.o ds5000.md/Quad_AddUnsLong.o ds5000.md/Quad_CompareUns.o ds5000.md/Stdio_Setup.o ds5000.md/Time_Add.o ds5000.md/Time_Divide.o ds5000.md/Time_Multiply.o ds5000.md/Time_Subtract.o ds5000.md/Time_ToAscii.o ds5000.md/Time_ToParts.o ds5000.md/atof.o ds5000.md/atoi.o ds5000.md/bcmp.o ds5000.md/bcopy.o ds5000.md/bzero.o ds5000.md/ctypeBits.o ds5000.md/errno.o ds5000.md/fclose.o ds5000.md/fflush.o ds5000.md/fgetc.o ds5000.md/fgets.o ds5000.md/fmt.o ds5000.md/fprintf.o ds5000.md/fputc.o ds5000.md/frexp.o ds5000.md/fscanf.o ds5000.md/isascii.o ds5000.md/isatty.o ds5000.md/isinf.o ds5000.md/isnan.o ds5000.md/isspace.o ds5000.md/iszero.o ds5000.md/modf.o ds5000.md/rename.o ds5000.md/sprintf.o ds5000.md/sscanf.o ds5000.md/status.o ds5000.md/strcasecmp.o ds5000.md/strcat.o ds5000.md/strchr.o ds5000.md/strcmp.o ds5000.md/strcpy.o ds5000.md/strerror.o ds5000.md/strlen.o ds5000.md/strncmp.o ds5000.md/strncpy.o ds5000.md/strtoul.o ds5000.md/timeConstants.o ds5000.md/ttyDriver.o ds5000.md/ungetc.o ds5000.md/unlink.o ds5000.md/vfprintf.o ds5000.md/vfscanf.o
CLEANOBJS	= ds5000.md/bcopy.o ds5000.md/List_Init.o ds5000.md/List_Insert.o ds5000.md/List_Move.o ds5000.md/List_Remove.o ds5000.md/MemData.o ds5000.md/Time_Add.o ds5000.md/Time_Divide.o ds5000.md/Time_Multiply.o ds5000.md/Time_Subtract.o ds5000.md/Time_ToAscii.o ds5000.md/Time_ToParts.o ds5000.md/bcmp.o ds5000.md/bzero.o ds5000.md/fmt.o ds5000.md/isinf.o ds5000.md/sprintf.o ds5000.md/status.o ds5000.md/strcat.o ds5000.md/strchr.o ds5000.md/strcmp.o ds5000.md/strcpy.o ds5000.md/strlen.o ds5000.md/strncmp.o ds5000.md/strncpy.o ds5000.md/strtoul.o ds5000.md/timeConstants.o ds5000.md/ttyDriver.o ds5000.md/frexp.o ds5000.md/isascii.o ds5000.md/Stdio_Setup.o ds5000.md/ctypeBits.o ds5000.md/fflush.o ds5000.md/fputc.o ds5000.md/isspace.o ds5000.md/vfprintf.o ds5000.md/atof.o ds5000.md/atoi.o ds5000.md/fgetc.o ds5000.md/sscanf.o ds5000.md/ungetc.o ds5000.md/vfscanf.o ds5000.md/Net_AddrToString.o ds5000.md/Net_EtherAddrToString.o ds5000.md/Net_HostToNetInt.o ds5000.md/Net_HostToNetShort.o ds5000.md/Net_InetAddrToString.o ds5000.md/Net_InetChecksum.o ds5000.md/Net_InetHdrChecksum.o ds5000.md/Net_NetToHostInt.o ds5000.md/Net_NetToHostShort.o ds5000.md/fclose.o ds5000.md/fgets.o ds5000.md/fprintf.o ds5000.md/fscanf.o ds5000.md/isatty.o ds5000.md/isnan.o ds5000.md/iszero.o ds5000.md/rename.o ds5000.md/strcasecmp.o ds5000.md/unlink.o ds5000.md/errno.o ds5000.md/strerror.o ds5000.md/Quad_AddUns.o ds5000.md/Quad_AddUnsLong.o ds5000.md/Quad_CompareUns.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= ds5000.md/modf.o
