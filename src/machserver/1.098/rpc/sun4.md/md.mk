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
# Tue Jul  2 14:05:12 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/rpcDelays.c rpcClient.c rpcTest.c rpcInit.c rpcDaemon.c rpcDebug.c rpcTrace.c rpcOutput.c rpcCall.c rpcServer.c rpcStubs.c rpcSrvStat.c rpcHistogram.c rpcByteSwap.c rpcCltStat.c rpcDispatch.c
HDRS		= rpc.h rpcCall.h rpcClient.h rpcCltStat.h rpcHistogram.h rpcInt.h rpcPacket.h rpcServer.h rpcSrvStat.h rpcTrace.h rpcTypes.h
MDPUBHDRS	= 
OBJS		= sun4.md/rpcByteSwap.o sun4.md/rpcCall.o sun4.md/rpcClient.o sun4.md/rpcCltStat.o sun4.md/rpcDaemon.o sun4.md/rpcDebug.o sun4.md/rpcDelays.o sun4.md/rpcDispatch.o sun4.md/rpcHistogram.o sun4.md/rpcInit.o sun4.md/rpcOutput.o sun4.md/rpcServer.o sun4.md/rpcSrvStat.o sun4.md/rpcStubs.o sun4.md/rpcTest.o sun4.md/rpcTrace.o
CLEANOBJS	= sun4.md/rpcDelays.o sun4.md/rpcClient.o sun4.md/rpcTest.o sun4.md/rpcInit.o sun4.md/rpcDaemon.o sun4.md/rpcDebug.o sun4.md/rpcTrace.o sun4.md/rpcOutput.o sun4.md/rpcCall.o sun4.md/rpcServer.o sun4.md/rpcStubs.o sun4.md/rpcSrvStat.o sun4.md/rpcHistogram.o sun4.md/rpcByteSwap.o sun4.md/rpcCltStat.o sun4.md/rpcDispatch.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
