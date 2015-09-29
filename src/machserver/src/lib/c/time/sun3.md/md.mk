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
# Wed Oct  2 23:02:41 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= timeConstants.c Time_Add.c Time_Divide.c Time_Multiply.c Time_Normalize.c Time_Subtract.c Time_ToAscii.c Time_ToParts.c time.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/timeConstants.o sun3.md/Time_Add.o sun3.md/Time_Divide.o sun3.md/Time_Multiply.o sun3.md/Time_Normalize.o sun3.md/Time_Subtract.o sun3.md/Time_ToAscii.o sun3.md/Time_ToParts.o sun3.md/time.o
CLEANOBJS	= sun3.md/timeConstants.o sun3.md/Time_Add.o sun3.md/Time_Divide.o sun3.md/Time_Multiply.o sun3.md/Time_Normalize.o sun3.md/Time_Subtract.o sun3.md/Time_ToAscii.o sun3.md/Time_ToParts.o sun3.md/time.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
