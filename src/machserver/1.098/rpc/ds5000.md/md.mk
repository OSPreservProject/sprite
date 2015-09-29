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
# Tue Jul  2 14:05:00 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/rpcDelays.c rpcClient.c rpcTest.c rpcInit.c rpcDaemon.c rpcDebug.c rpcTrace.c rpcOutput.c rpcCall.c rpcServer.c rpcStubs.c rpcSrvStat.c rpcHistogram.c rpcByteSwap.c rpcCltStat.c rpcDispatch.c
HDRS		= rpc.h rpcCall.h rpcClient.h rpcCltStat.h rpcHistogram.h rpcInt.h rpcPacket.h rpcServer.h rpcSrvStat.h rpcTrace.h rpcTypes.h
MDPUBHDRS	= 
OBJS		= ds5000.md/rpcByteSwap.o ds5000.md/rpcCall.o ds5000.md/rpcClient.o ds5000.md/rpcCltStat.o ds5000.md/rpcDaemon.o ds5000.md/rpcDebug.o ds5000.md/rpcDelays.o ds5000.md/rpcDispatch.o ds5000.md/rpcHistogram.o ds5000.md/rpcInit.o ds5000.md/rpcOutput.o ds5000.md/rpcServer.o ds5000.md/rpcSrvStat.o ds5000.md/rpcStubs.o ds5000.md/rpcTest.o ds5000.md/rpcTrace.o
CLEANOBJS	= ds5000.md/rpcDelays.o ds5000.md/rpcClient.o ds5000.md/rpcTest.o ds5000.md/rpcInit.o ds5000.md/rpcDaemon.o ds5000.md/rpcDebug.o ds5000.md/rpcTrace.o ds5000.md/rpcOutput.o ds5000.md/rpcCall.o ds5000.md/rpcServer.o ds5000.md/rpcStubs.o ds5000.md/rpcSrvStat.o ds5000.md/rpcHistogram.o ds5000.md/rpcByteSwap.o ds5000.md/rpcCltStat.o ds5000.md/rpcDispatch.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
