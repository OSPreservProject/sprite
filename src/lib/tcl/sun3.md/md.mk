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
# Fri Mar 20 09:07:19 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= panic.c regexp.c tclAssem.c tclBasic.c tclCkalloc.c tclCmdAH.c tclCmdIL.c tclCmdMZ.c tclEnv.c tclExpr.c tclGet.c tclGlob.c tclHash.c tclHistory.c tclParse.c tclProc.c tclUnixAZ.c tclUnixStr.c tclUnixUtil.c tclUtil.c tclVar.c
HDRS		= limits.h regexp.h tcl.h tclHash.h tclInt.h tclUnix.h
MDPUBHDRS	= 
OBJS		= sun3.md/panic.o sun3.md/regexp.o sun3.md/tclAssem.o sun3.md/tclBasic.o sun3.md/tclCkalloc.o sun3.md/tclCmdAH.o sun3.md/tclCmdIL.o sun3.md/tclCmdMZ.o sun3.md/tclEnv.o sun3.md/tclExpr.o sun3.md/tclGet.o sun3.md/tclGlob.o sun3.md/tclHash.o sun3.md/tclHistory.o sun3.md/tclParse.o sun3.md/tclProc.o sun3.md/tclUnixAZ.o sun3.md/tclUnixStr.o sun3.md/tclUnixUtil.o sun3.md/tclUtil.o sun3.md/tclVar.o
CLEANOBJS	= sun3.md/panic.o sun3.md/regexp.o sun3.md/tclAssem.o sun3.md/tclBasic.o sun3.md/tclCkalloc.o sun3.md/tclCmdAH.o sun3.md/tclCmdIL.o sun3.md/tclCmdMZ.o sun3.md/tclEnv.o sun3.md/tclExpr.o sun3.md/tclGet.o sun3.md/tclGlob.o sun3.md/tclHash.o sun3.md/tclHistory.o sun3.md/tclParse.o sun3.md/tclProc.o sun3.md/tclUnixAZ.o sun3.md/tclUnixStr.o sun3.md/tclUnixUtil.o sun3.md/tclUtil.o sun3.md/tclVar.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
