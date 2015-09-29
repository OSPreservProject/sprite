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
# Mon May 11 14:06:32 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= lstAppend.c lstAtEnd.c lstAtFront.c lstClose.c lstConcat.c lstCur.c lstDatum.c lstDeQueue.c lstDestroy.c lstDupl.c lstEnQueue.c lstFake.c lstFind.c lstFindFrom.c lstFirst.c lstForEach.c lstForEachFrom.c lstIndex.c lstInit.c lstInsert.c lstIsAtEnd.c lstIsEmpty.c lstLast.c lstLength.c lstMember.c lstMove.c lstNext.c lstOpen.c lstPred.c lstPrev.c lstRemove.c lstReplace.c lstSetCirc.c lstSucc.c
HDRS		= lst.h lstInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/lstAppend.o ds3100.md/lstAtEnd.o ds3100.md/lstAtFront.o ds3100.md/lstClose.o ds3100.md/lstConcat.o ds3100.md/lstCur.o ds3100.md/lstDatum.o ds3100.md/lstDeQueue.o ds3100.md/lstDestroy.o ds3100.md/lstDupl.o ds3100.md/lstEnQueue.o ds3100.md/lstFake.o ds3100.md/lstFind.o ds3100.md/lstFindFrom.o ds3100.md/lstFirst.o ds3100.md/lstForEach.o ds3100.md/lstForEachFrom.o ds3100.md/lstIndex.o ds3100.md/lstInit.o ds3100.md/lstInsert.o ds3100.md/lstIsAtEnd.o ds3100.md/lstIsEmpty.o ds3100.md/lstLast.o ds3100.md/lstLength.o ds3100.md/lstMember.o ds3100.md/lstMove.o ds3100.md/lstNext.o ds3100.md/lstOpen.o ds3100.md/lstPred.o ds3100.md/lstPrev.o ds3100.md/lstRemove.o ds3100.md/lstReplace.o ds3100.md/lstSetCirc.o ds3100.md/lstSucc.o
CLEANOBJS	= ds3100.md/lstAppend.o ds3100.md/lstAtEnd.o ds3100.md/lstAtFront.o ds3100.md/lstClose.o ds3100.md/lstConcat.o ds3100.md/lstCur.o ds3100.md/lstDatum.o ds3100.md/lstDeQueue.o ds3100.md/lstDestroy.o ds3100.md/lstDupl.o ds3100.md/lstEnQueue.o ds3100.md/lstFake.o ds3100.md/lstFind.o ds3100.md/lstFindFrom.o ds3100.md/lstFirst.o ds3100.md/lstForEach.o ds3100.md/lstForEachFrom.o ds3100.md/lstIndex.o ds3100.md/lstInit.o ds3100.md/lstInsert.o ds3100.md/lstIsAtEnd.o ds3100.md/lstIsEmpty.o ds3100.md/lstLast.o ds3100.md/lstLength.o ds3100.md/lstMember.o ds3100.md/lstMove.o ds3100.md/lstNext.o ds3100.md/lstOpen.o ds3100.md/lstPred.o ds3100.md/lstPrev.o ds3100.md/lstRemove.o ds3100.md/lstReplace.o ds3100.md/lstSetCirc.o ds3100.md/lstSucc.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
