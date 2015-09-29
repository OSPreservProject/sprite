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
# Tue Nov  7 22:28:57 PST 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= Ulog_ClearLogins.c Ulog_GetAllLogins.c Ulog_LastLogin.c Ulog_RecordLogin.c Ulog_RecordLogout.c
HDRS		= ulogInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Ulog_ClearLogins.o ds3100.md/Ulog_GetAllLogins.o ds3100.md/Ulog_LastLogin.o ds3100.md/Ulog_RecordLogin.o ds3100.md/Ulog_RecordLogout.o
CLEANOBJS	= ds3100.md/Ulog_ClearLogins.o ds3100.md/Ulog_GetAllLogins.o ds3100.md/Ulog_LastLogin.o ds3100.md/Ulog_RecordLogin.o ds3100.md/Ulog_RecordLogout.o
DISTDIR        ?= @(DISTDIR)
