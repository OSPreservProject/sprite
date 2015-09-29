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
# Sat Nov  2 22:42:20 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/rpcDelays.c rpcDebug.c rpcTest.c rpcStubs.c rpcCall.c rpcClient.c rpcDaemon.c rpcDispatch.c rpcInit.c rpcOutput.c rpcServer.c rpcHistogram.c rpcTrace.c rpcByteSwap.c rpcCltStat.c rpcSrvStat.c
HDRS		= rpc.h rpcCall.h rpcClient.h rpcCltStat.h rpcHistogram.h rpcInt.h rpcPacket.h rpcServer.h rpcSrvStat.h rpcTrace.h rpcTypes.h
MDPUBHDRS	= 
OBJS		= ds3100.md/rpcDelays.o ds3100.md/rpcDebug.o ds3100.md/rpcTest.o ds3100.md/rpcStubs.o ds3100.md/rpcCall.o ds3100.md/rpcClient.o ds3100.md/rpcDaemon.o ds3100.md/rpcDispatch.o ds3100.md/rpcInit.o ds3100.md/rpcOutput.o ds3100.md/rpcServer.o ds3100.md/rpcHistogram.o ds3100.md/rpcTrace.o ds3100.md/rpcByteSwap.o ds3100.md/rpcCltStat.o ds3100.md/rpcSrvStat.o
CLEANOBJS	= ds3100.md/rpcDelays.o ds3100.md/rpcDebug.o ds3100.md/rpcTest.o ds3100.md/rpcStubs.o ds3100.md/rpcCall.o ds3100.md/rpcClient.o ds3100.md/rpcDaemon.o ds3100.md/rpcDispatch.o ds3100.md/rpcInit.o ds3100.md/rpcOutput.o ds3100.md/rpcServer.o ds3100.md/rpcHistogram.o ds3100.md/rpcTrace.o ds3100.md/rpcByteSwap.o ds3100.md/rpcCltStat.o ds3100.md/rpcSrvStat.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
