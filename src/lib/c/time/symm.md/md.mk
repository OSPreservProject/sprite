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
# Mon Jun  8 14:38:10 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Time_Divide.c Time_GetTime.c Time_Multiply.c Time_Normalize.c Time_Subtract.c timeConstants.c times.c Time_Add.c ctime.c time.c timezone.c Time_ToAscii.c Time_ToParts.c mktime.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/Time_Divide.o symm.md/Time_GetTime.o symm.md/Time_Multiply.o symm.md/Time_Normalize.o symm.md/Time_Subtract.o symm.md/timeConstants.o symm.md/times.o symm.md/Time_Add.o symm.md/ctime.o symm.md/time.o symm.md/timezone.o symm.md/Time_ToAscii.o symm.md/Time_ToParts.o symm.md/mktime.o
CLEANOBJS	= symm.md/Time_Divide.o symm.md/Time_GetTime.o symm.md/Time_Multiply.o symm.md/Time_Normalize.o symm.md/Time_Subtract.o symm.md/timeConstants.o symm.md/times.o symm.md/Time_Add.o symm.md/ctime.o symm.md/time.o symm.md/timezone.o symm.md/Time_ToAscii.o symm.md/Time_ToParts.o symm.md/mktime.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile TAGS
SACREDOBJS	= 
