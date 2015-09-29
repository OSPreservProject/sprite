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
# Mon Jun  8 14:31:18 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Mig_ConfirmIdle.c MigHostCache.c Mig_Done.c Mig_Evict.c MigOpenPdev.c Mig_DeleteHost.c Mig_GetInfo.c Mig_GetStats.c Mig_ReturnHosts.c Mig_GetAllInfo.c Mig_GetIdleNode.c Mig_GetPdevName.c Mig_RequestIdleHosts.c migAlarm.c
HDRS		= migInt.h
MDPUBHDRS	= 
OBJS		= symm.md/Mig_ConfirmIdle.o symm.md/MigHostCache.o symm.md/Mig_Done.o symm.md/Mig_Evict.o symm.md/MigOpenPdev.o symm.md/Mig_DeleteHost.o symm.md/Mig_GetInfo.o symm.md/Mig_GetStats.o symm.md/Mig_ReturnHosts.o symm.md/Mig_GetAllInfo.o symm.md/Mig_GetIdleNode.o symm.md/Mig_GetPdevName.o symm.md/Mig_RequestIdleHosts.o symm.md/migAlarm.o
CLEANOBJS	= symm.md/Mig_ConfirmIdle.o symm.md/MigHostCache.o symm.md/Mig_Done.o symm.md/Mig_Evict.o symm.md/MigOpenPdev.o symm.md/Mig_DeleteHost.o symm.md/Mig_GetInfo.o symm.md/Mig_GetStats.o symm.md/Mig_ReturnHosts.o symm.md/Mig_GetAllInfo.o symm.md/Mig_GetIdleNode.o symm.md/Mig_GetPdevName.o symm.md/Mig_RequestIdleHosts.o symm.md/migAlarm.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
