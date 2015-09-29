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
# Mon Jun  8 14:31:13 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Mig_ConfirmIdle.c MigHostCache.c Mig_Done.c Mig_Evict.c MigOpenPdev.c Mig_DeleteHost.c Mig_GetInfo.c Mig_GetStats.c Mig_ReturnHosts.c Mig_GetAllInfo.c Mig_GetIdleNode.c Mig_GetPdevName.c Mig_RequestIdleHosts.c migAlarm.c
HDRS		= migInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/Mig_ConfirmIdle.o sun4.md/MigHostCache.o sun4.md/Mig_Done.o sun4.md/Mig_Evict.o sun4.md/MigOpenPdev.o sun4.md/Mig_DeleteHost.o sun4.md/Mig_GetInfo.o sun4.md/Mig_GetStats.o sun4.md/Mig_ReturnHosts.o sun4.md/Mig_GetAllInfo.o sun4.md/Mig_GetIdleNode.o sun4.md/Mig_GetPdevName.o sun4.md/Mig_RequestIdleHosts.o sun4.md/migAlarm.o
CLEANOBJS	= sun4.md/Mig_ConfirmIdle.o sun4.md/MigHostCache.o sun4.md/Mig_Done.o sun4.md/Mig_Evict.o sun4.md/MigOpenPdev.o sun4.md/Mig_DeleteHost.o sun4.md/Mig_GetInfo.o sun4.md/Mig_GetStats.o sun4.md/Mig_ReturnHosts.o sun4.md/Mig_GetAllInfo.o sun4.md/Mig_GetIdleNode.o sun4.md/Mig_GetPdevName.o sun4.md/Mig_RequestIdleHosts.o sun4.md/migAlarm.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
