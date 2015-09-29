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
# Sun Apr 21 22:44:50 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Time_Divide.c Time_GetTime.c Time_Multiply.c Time_Normalize.c Time_Subtract.c timeConstants.c times.c Time_Add.c ctime.c time.c timezone.c Time_ToAscii.c Time_ToParts.c mktime.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/Time_Add.o ds3100.md/Time_Divide.o ds3100.md/Time_Multiply.o ds3100.md/Time_Normalize.o ds3100.md/Time_Subtract.o ds3100.md/Time_ToAscii.o ds3100.md/Time_ToParts.o ds3100.md/ctime.o ds3100.md/mktime.o ds3100.md/time.o ds3100.md/timeConstants.o ds3100.md/times.o ds3100.md/timezone.o ds3100.md/Time_GetTime.o
CLEANOBJS	= ds3100.md/Time_Divide.o ds3100.md/Time_GetTime.o ds3100.md/Time_Multiply.o ds3100.md/Time_Normalize.o ds3100.md/Time_Subtract.o ds3100.md/timeConstants.o ds3100.md/times.o ds3100.md/Time_Add.o ds3100.md/ctime.o ds3100.md/time.o ds3100.md/timezone.o ds3100.md/Time_ToAscii.o ds3100.md/Time_ToParts.o ds3100.md/mktime.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile TAGS
SACREDOBJS	= 
