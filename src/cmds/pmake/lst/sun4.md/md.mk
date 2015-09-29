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
# Wed Jun 10 17:53:40 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= lstAppend.c lstAtEnd.c lstAtFront.c lstClose.c lstConcat.c lstCur.c lstDatum.c lstDeQueue.c lstDestroy.c lstDupl.c lstEnQueue.c lstFake.c lstFind.c lstFindFrom.c lstFirst.c lstForEach.c lstForEachFrom.c lstIndex.c lstInit.c lstInsert.c lstIsAtEnd.c lstIsEmpty.c lstLast.c lstLength.c lstMember.c lstMove.c lstNext.c lstOpen.c lstPred.c lstPrev.c lstRemove.c lstReplace.c lstSetCirc.c lstSucc.c
HDRS		= lst.h lstInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/lstAppend.o sun4.md/lstAtEnd.o sun4.md/lstAtFront.o sun4.md/lstClose.o sun4.md/lstConcat.o sun4.md/lstCur.o sun4.md/lstDatum.o sun4.md/lstDeQueue.o sun4.md/lstDestroy.o sun4.md/lstDupl.o sun4.md/lstEnQueue.o sun4.md/lstFake.o sun4.md/lstFind.o sun4.md/lstFindFrom.o sun4.md/lstFirst.o sun4.md/lstForEach.o sun4.md/lstForEachFrom.o sun4.md/lstIndex.o sun4.md/lstInit.o sun4.md/lstInsert.o sun4.md/lstIsAtEnd.o sun4.md/lstIsEmpty.o sun4.md/lstLast.o sun4.md/lstLength.o sun4.md/lstMember.o sun4.md/lstMove.o sun4.md/lstNext.o sun4.md/lstOpen.o sun4.md/lstPred.o sun4.md/lstPrev.o sun4.md/lstRemove.o sun4.md/lstReplace.o sun4.md/lstSetCirc.o sun4.md/lstSucc.o
CLEANOBJS	= sun4.md/lstAppend.o sun4.md/lstAtEnd.o sun4.md/lstAtFront.o sun4.md/lstClose.o sun4.md/lstConcat.o sun4.md/lstCur.o sun4.md/lstDatum.o sun4.md/lstDeQueue.o sun4.md/lstDestroy.o sun4.md/lstDupl.o sun4.md/lstEnQueue.o sun4.md/lstFake.o sun4.md/lstFind.o sun4.md/lstFindFrom.o sun4.md/lstFirst.o sun4.md/lstForEach.o sun4.md/lstForEachFrom.o sun4.md/lstIndex.o sun4.md/lstInit.o sun4.md/lstInsert.o sun4.md/lstIsAtEnd.o sun4.md/lstIsEmpty.o sun4.md/lstLast.o sun4.md/lstLength.o sun4.md/lstMember.o sun4.md/lstMove.o sun4.md/lstNext.o sun4.md/lstOpen.o sun4.md/lstPred.o sun4.md/lstPrev.o sun4.md/lstRemove.o sun4.md/lstReplace.o sun4.md/lstSetCirc.o sun4.md/lstSucc.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	=
