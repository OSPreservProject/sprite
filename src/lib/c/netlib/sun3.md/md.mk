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
# Mon Jun  8 21:24:26 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Net_AddrNetMask.c Net_AddrNetNum.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrHostNum.c Net_InetHdrChecksum.c Net_MakeInetAddr.c Net_NetToHostInt.c Net_NetToHostShort.c Net_StringToEtherAddr.c Net_StringToInetAddr.c Net_StringToNetNum.c Net_InetAddrToString.c Net_InetChecksum.c Net_SetAddress.c Net_AddrCmp.c Net_AddrToString.c Net_FDDIAddrToString.c Net_StringToAddr.c Net_StringToFDDIAddr.c Net_StringToUltraAddr.c Net_UltraAddrToString.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/Net_AddrNetMask.o sun3.md/Net_AddrNetNum.o sun3.md/Net_EtherAddrToString.o sun3.md/Net_HostToNetInt.o sun3.md/Net_HostToNetShort.o sun3.md/Net_InetAddrHostNum.o sun3.md/Net_InetHdrChecksum.o sun3.md/Net_MakeInetAddr.o sun3.md/Net_NetToHostInt.o sun3.md/Net_NetToHostShort.o sun3.md/Net_StringToEtherAddr.o sun3.md/Net_StringToInetAddr.o sun3.md/Net_StringToNetNum.o sun3.md/Net_InetAddrToString.o sun3.md/Net_InetChecksum.o sun3.md/Net_SetAddress.o sun3.md/Net_AddrCmp.o sun3.md/Net_AddrToString.o sun3.md/Net_FDDIAddrToString.o sun3.md/Net_StringToAddr.o sun3.md/Net_StringToFDDIAddr.o sun3.md/Net_StringToUltraAddr.o sun3.md/Net_UltraAddrToString.o
CLEANOBJS	= sun3.md/Net_AddrNetMask.o sun3.md/Net_AddrNetNum.o sun3.md/Net_EtherAddrToString.o sun3.md/Net_HostToNetInt.o sun3.md/Net_HostToNetShort.o sun3.md/Net_InetAddrHostNum.o sun3.md/Net_InetHdrChecksum.o sun3.md/Net_MakeInetAddr.o sun3.md/Net_NetToHostInt.o sun3.md/Net_NetToHostShort.o sun3.md/Net_StringToEtherAddr.o sun3.md/Net_StringToInetAddr.o sun3.md/Net_StringToNetNum.o sun3.md/Net_InetAddrToString.o sun3.md/Net_InetChecksum.o sun3.md/Net_SetAddress.o sun3.md/Net_AddrCmp.o sun3.md/Net_AddrToString.o sun3.md/Net_FDDIAddrToString.o sun3.md/Net_StringToAddr.o sun3.md/Net_StringToFDDIAddr.o sun3.md/Net_StringToUltraAddr.o sun3.md/Net_UltraAddrToString.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile tags
SACREDOBJS	= 
