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
# Wed Jun 10 17:53:31 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= lstAppend.c lstAtEnd.c lstAtFront.c lstClose.c lstConcat.c lstCur.c lstDatum.c lstDeQueue.c lstDestroy.c lstDupl.c lstEnQueue.c lstFake.c lstFind.c lstFindFrom.c lstFirst.c lstForEach.c lstForEachFrom.c lstIndex.c lstInit.c lstInsert.c lstIsAtEnd.c lstIsEmpty.c lstLast.c lstLength.c lstMember.c lstMove.c lstNext.c lstOpen.c lstPred.c lstPrev.c lstRemove.c lstReplace.c lstSetCirc.c lstSucc.c
HDRS		= lst.h lstInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/lstAppend.o sun3.md/lstAtEnd.o sun3.md/lstAtFront.o sun3.md/lstClose.o sun3.md/lstConcat.o sun3.md/lstCur.o sun3.md/lstDatum.o sun3.md/lstDeQueue.o sun3.md/lstDestroy.o sun3.md/lstDupl.o sun3.md/lstEnQueue.o sun3.md/lstFake.o sun3.md/lstFind.o sun3.md/lstFindFrom.o sun3.md/lstFirst.o sun3.md/lstForEach.o sun3.md/lstForEachFrom.o sun3.md/lstIndex.o sun3.md/lstInit.o sun3.md/lstInsert.o sun3.md/lstIsAtEnd.o sun3.md/lstIsEmpty.o sun3.md/lstLast.o sun3.md/lstLength.o sun3.md/lstMember.o sun3.md/lstMove.o sun3.md/lstNext.o sun3.md/lstOpen.o sun3.md/lstPred.o sun3.md/lstPrev.o sun3.md/lstRemove.o sun3.md/lstReplace.o sun3.md/lstSetCirc.o sun3.md/lstSucc.o
CLEANOBJS	= sun3.md/lstAppend.o sun3.md/lstAtEnd.o sun3.md/lstAtFront.o sun3.md/lstClose.o sun3.md/lstConcat.o sun3.md/lstCur.o sun3.md/lstDatum.o sun3.md/lstDeQueue.o sun3.md/lstDestroy.o sun3.md/lstDupl.o sun3.md/lstEnQueue.o sun3.md/lstFake.o sun3.md/lstFind.o sun3.md/lstFindFrom.o sun3.md/lstFirst.o sun3.md/lstForEach.o sun3.md/lstForEachFrom.o sun3.md/lstIndex.o sun3.md/lstInit.o sun3.md/lstInsert.o sun3.md/lstIsAtEnd.o sun3.md/lstIsEmpty.o sun3.md/lstLast.o sun3.md/lstLength.o sun3.md/lstMember.o sun3.md/lstMove.o sun3.md/lstNext.o sun3.md/lstOpen.o sun3.md/lstPred.o sun3.md/lstPrev.o sun3.md/lstRemove.o sun3.md/lstReplace.o sun3.md/lstSetCirc.o sun3.md/lstSucc.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	=
