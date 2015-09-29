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
# Fri Mar 20 09:07:28 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= panic.c regexp.c tclAssem.c tclBasic.c tclCkalloc.c tclCmdAH.c tclCmdIL.c tclCmdMZ.c tclEnv.c tclExpr.c tclGet.c tclGlob.c tclHash.c tclHistory.c tclParse.c tclProc.c tclUnixAZ.c tclUnixStr.c tclUnixUtil.c tclUtil.c tclVar.c
HDRS		= limits.h regexp.h tcl.h tclHash.h tclInt.h tclUnix.h
MDPUBHDRS	= 
OBJS		= sun4.md/panic.o sun4.md/regexp.o sun4.md/tclAssem.o sun4.md/tclBasic.o sun4.md/tclCkalloc.o sun4.md/tclCmdAH.o sun4.md/tclCmdIL.o sun4.md/tclCmdMZ.o sun4.md/tclEnv.o sun4.md/tclExpr.o sun4.md/tclGet.o sun4.md/tclGlob.o sun4.md/tclHash.o sun4.md/tclHistory.o sun4.md/tclParse.o sun4.md/tclProc.o sun4.md/tclUnixAZ.o sun4.md/tclUnixStr.o sun4.md/tclUnixUtil.o sun4.md/tclUtil.o sun4.md/tclVar.o
CLEANOBJS	= sun4.md/panic.o sun4.md/regexp.o sun4.md/tclAssem.o sun4.md/tclBasic.o sun4.md/tclCkalloc.o sun4.md/tclCmdAH.o sun4.md/tclCmdIL.o sun4.md/tclCmdMZ.o sun4.md/tclEnv.o sun4.md/tclExpr.o sun4.md/tclGet.o sun4.md/tclGlob.o sun4.md/tclHash.o sun4.md/tclHistory.o sun4.md/tclParse.o sun4.md/tclProc.o sun4.md/tclUnixAZ.o sun4.md/tclUnixStr.o sun4.md/tclUnixUtil.o sun4.md/tclUtil.o sun4.md/tclVar.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
