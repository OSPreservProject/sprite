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
# Sat Jun  9 18:04:42 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Net_AddrNetMask.c Net_AddrNetNum.c Net_AddrToString.c Net_EtherAddrToString.c Net_HostToNetInt.c Net_HostToNetShort.c Net_InetAddrHostNum.c Net_InetAddrToString.c Net_InetChecksum.c Net_InetHdrChecksum.c Net_MakeInetAddr.c Net_NetToHostInt.c Net_NetToHostShort.c Net_StringToAddr.c Net_StringToEtherAddr.c Net_StringToInetAddr.c Net_StringToNetNum.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/Net_AddrNetMask.o sun3.md/Net_AddrNetNum.o sun3.md/Net_AddrToString.o sun3.md/Net_EtherAddrToString.o sun3.md/Net_HostToNetInt.o sun3.md/Net_HostToNetShort.o sun3.md/Net_InetAddrHostNum.o sun3.md/Net_InetAddrToString.o sun3.md/Net_InetChecksum.o sun3.md/Net_InetHdrChecksum.o sun3.md/Net_MakeInetAddr.o sun3.md/Net_NetToHostInt.o sun3.md/Net_NetToHostShort.o sun3.md/Net_StringToAddr.o sun3.md/Net_StringToEtherAddr.o sun3.md/Net_StringToInetAddr.o sun3.md/Net_StringToNetNum.o
CLEANOBJS	= sun3.md/Net_AddrNetMask.o sun3.md/Net_AddrNetNum.o sun3.md/Net_AddrToString.o sun3.md/Net_EtherAddrToString.o sun3.md/Net_HostToNetInt.o sun3.md/Net_HostToNetShort.o sun3.md/Net_InetAddrHostNum.o sun3.md/Net_InetAddrToString.o sun3.md/Net_InetChecksum.o sun3.md/Net_InetHdrChecksum.o sun3.md/Net_MakeInetAddr.o sun3.md/Net_NetToHostInt.o sun3.md/Net_NetToHostShort.o sun3.md/Net_StringToAddr.o sun3.md/Net_StringToEtherAddr.o sun3.md/Net_StringToInetAddr.o sun3.md/Net_StringToNetNum.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
