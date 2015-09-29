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
# Mon Jun  8 14:31:02 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Mig_ConfirmIdle.c MigHostCache.c Mig_Done.c Mig_Evict.c MigOpenPdev.c Mig_DeleteHost.c Mig_GetInfo.c Mig_GetStats.c Mig_ReturnHosts.c Mig_GetAllInfo.c Mig_GetIdleNode.c Mig_GetPdevName.c Mig_RequestIdleHosts.c migAlarm.c
HDRS		= migInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/MigHostCache.o ds3100.md/MigOpenPdev.o ds3100.md/Mig_ConfirmIdle.o ds3100.md/Mig_DeleteHost.o ds3100.md/Mig_Done.o ds3100.md/Mig_Evict.o ds3100.md/Mig_GetAllInfo.o ds3100.md/Mig_GetIdleNode.o ds3100.md/Mig_GetInfo.o ds3100.md/Mig_GetPdevName.o ds3100.md/Mig_GetStats.o ds3100.md/Mig_RequestIdleHosts.o ds3100.md/Mig_ReturnHosts.o ds3100.md/migAlarm.o
CLEANOBJS	= ds3100.md/Mig_ConfirmIdle.o ds3100.md/MigHostCache.o ds3100.md/Mig_Done.o ds3100.md/Mig_Evict.o ds3100.md/MigOpenPdev.o ds3100.md/Mig_DeleteHost.o ds3100.md/Mig_GetInfo.o ds3100.md/Mig_GetStats.o ds3100.md/Mig_ReturnHosts.o ds3100.md/Mig_GetAllInfo.o ds3100.md/Mig_GetIdleNode.o ds3100.md/Mig_GetPdevName.o ds3100.md/Mig_RequestIdleHosts.o ds3100.md/migAlarm.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
