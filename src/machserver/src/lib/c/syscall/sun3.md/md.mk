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
# Sun Dec  8 18:12:40 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Fs_ReadVector.c Fs_Truncate.c Fs_WriteVector.c Ioc_Reposition.c Ioc_Truncate.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/Fs_ReadVector.o sun3.md/Fs_Truncate.o sun3.md/Fs_WriteVector.o sun3.md/Ioc_Reposition.o sun3.md/Ioc_Truncate.o
CLEANOBJS	= sun3.md/Fs_ReadVector.o sun3.md/Fs_Truncate.o sun3.md/Fs_WriteVector.o sun3.md/Ioc_Reposition.o sun3.md/Ioc_Truncate.o
INSTFILES	= Makefile
SACREDOBJS	= 
