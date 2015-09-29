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
# Tue Jul  2 14:04:54 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/rpcDelays.c rpcClient.c rpcTest.c rpcInit.c rpcDaemon.c rpcDebug.c rpcTrace.c rpcOutput.c rpcCall.c rpcServer.c rpcStubs.c rpcSrvStat.c rpcHistogram.c rpcByteSwap.c rpcCltStat.c rpcDispatch.c
HDRS		= rpc.h rpcCall.h rpcClient.h rpcCltStat.h rpcHistogram.h rpcInt.h rpcPacket.h rpcServer.h rpcSrvStat.h rpcTrace.h rpcTypes.h
MDPUBHDRS	= 
OBJS		= ds3100.md/rpcByteSwap.o ds3100.md/rpcCall.o ds3100.md/rpcClient.o ds3100.md/rpcCltStat.o ds3100.md/rpcDaemon.o ds3100.md/rpcDebug.o ds3100.md/rpcDelays.o ds3100.md/rpcDispatch.o ds3100.md/rpcHistogram.o ds3100.md/rpcInit.o ds3100.md/rpcOutput.o ds3100.md/rpcServer.o ds3100.md/rpcSrvStat.o ds3100.md/rpcStubs.o ds3100.md/rpcTest.o ds3100.md/rpcTrace.o
CLEANOBJS	= ds3100.md/rpcDelays.o ds3100.md/rpcClient.o ds3100.md/rpcTest.o ds3100.md/rpcInit.o ds3100.md/rpcDaemon.o ds3100.md/rpcDebug.o ds3100.md/rpcTrace.o ds3100.md/rpcOutput.o ds3100.md/rpcCall.o ds3100.md/rpcServer.o ds3100.md/rpcStubs.o ds3100.md/rpcSrvStat.o ds3100.md/rpcHistogram.o ds3100.md/rpcByteSwap.o ds3100.md/rpcCltStat.o ds3100.md/rpcDispatch.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
