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
# Tue Jul  2 14:05:06 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/rpcDelays.c rpcClient.c rpcTest.c rpcInit.c rpcDaemon.c rpcDebug.c rpcTrace.c rpcOutput.c rpcCall.c rpcServer.c rpcStubs.c rpcSrvStat.c rpcHistogram.c rpcByteSwap.c rpcCltStat.c rpcDispatch.c
HDRS		= rpc.h rpcCall.h rpcClient.h rpcCltStat.h rpcHistogram.h rpcInt.h rpcPacket.h rpcServer.h rpcSrvStat.h rpcTrace.h rpcTypes.h
MDPUBHDRS	= 
OBJS		= sun3.md/rpcByteSwap.o sun3.md/rpcCall.o sun3.md/rpcClient.o sun3.md/rpcCltStat.o sun3.md/rpcDaemon.o sun3.md/rpcDebug.o sun3.md/rpcDelays.o sun3.md/rpcDispatch.o sun3.md/rpcHistogram.o sun3.md/rpcInit.o sun3.md/rpcOutput.o sun3.md/rpcServer.o sun3.md/rpcSrvStat.o sun3.md/rpcStubs.o sun3.md/rpcTest.o sun3.md/rpcTrace.o
CLEANOBJS	= sun3.md/rpcDelays.o sun3.md/rpcClient.o sun3.md/rpcTest.o sun3.md/rpcInit.o sun3.md/rpcDaemon.o sun3.md/rpcDebug.o sun3.md/rpcTrace.o sun3.md/rpcOutput.o sun3.md/rpcCall.o sun3.md/rpcServer.o sun3.md/rpcStubs.o sun3.md/rpcSrvStat.o sun3.md/rpcHistogram.o sun3.md/rpcByteSwap.o sun3.md/rpcCltStat.o sun3.md/rpcDispatch.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
