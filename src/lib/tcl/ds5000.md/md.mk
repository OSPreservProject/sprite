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
# Fri Mar 20 09:07:09 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= panic.c regexp.c tclAssem.c tclBasic.c tclCkalloc.c tclCmdAH.c tclCmdIL.c tclCmdMZ.c tclEnv.c tclExpr.c tclGet.c tclGlob.c tclHash.c tclHistory.c tclParse.c tclProc.c tclUnixAZ.c tclUnixStr.c tclUnixUtil.c tclUtil.c tclVar.c
HDRS		= limits.h regexp.h tcl.h tclHash.h tclInt.h tclUnix.h
MDPUBHDRS	= 
OBJS		= ds3100.md/panic.o ds3100.md/regexp.o ds3100.md/tclAssem.o ds3100.md/tclBasic.o ds3100.md/tclCkalloc.o ds3100.md/tclCmdAH.o ds3100.md/tclCmdIL.o ds3100.md/tclCmdMZ.o ds3100.md/tclEnv.o ds3100.md/tclExpr.o ds3100.md/tclGet.o ds3100.md/tclGlob.o ds3100.md/tclHash.o ds3100.md/tclHistory.o ds3100.md/tclParse.o ds3100.md/tclProc.o ds3100.md/tclUnixAZ.o ds3100.md/tclUnixStr.o ds3100.md/tclUnixUtil.o ds3100.md/tclUtil.o ds3100.md/tclVar.o
CLEANOBJS	= ds3100.md/panic.o ds3100.md/regexp.o ds3100.md/tclAssem.o ds3100.md/tclBasic.o ds3100.md/tclCkalloc.o ds3100.md/tclCmdAH.o ds3100.md/tclCmdIL.o ds3100.md/tclCmdMZ.o ds3100.md/tclEnv.o ds3100.md/tclExpr.o ds3100.md/tclGet.o ds3100.md/tclGlob.o ds3100.md/tclHash.o ds3100.md/tclHistory.o ds3100.md/tclParse.o ds3100.md/tclProc.o ds3100.md/tclUnixAZ.o ds3100.md/tclUnixStr.o ds3100.md/tclUnixUtil.o ds3100.md/tclUtil.o ds3100.md/tclVar.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
