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
# Thu Dec 17 17:02:59 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= dviselect.c conv.c dviclass.c error.c findpost.c fio.c font.c font_subr.c gfclass.c gffont.c gripes.c magfactor.c pkfont.c pxlfont.c rotate.c scaletfm.c scanpost.c search.c seek.c split.c strsave.c tfm.c tfmfont.c
HDRS		= arith.h binding.h box.h conv.h dvi.h dviclass.h dvicodes.h error.h fio.h font.h gfclass.h gfcodes.h imPcodes.h imagen.h io.h num.h postamble.h search.h str.h tfm.h types.h verser.h
MDPUBHDRS	= 
OBJS		= ds3100.md/conv.o ds3100.md/dviclass.o ds3100.md/dviselect.o ds3100.md/error.o ds3100.md/findpost.o ds3100.md/fio.o ds3100.md/font.o ds3100.md/font_subr.o ds3100.md/gfclass.o ds3100.md/gffont.o ds3100.md/gripes.o ds3100.md/magfactor.o ds3100.md/pkfont.o ds3100.md/pxlfont.o ds3100.md/rotate.o ds3100.md/scaletfm.o ds3100.md/scanpost.o ds3100.md/search.o ds3100.md/seek.o ds3100.md/split.o ds3100.md/strsave.o ds3100.md/tfm.o ds3100.md/tfmfont.o
CLEANOBJS	= ds3100.md/dviselect.o ds3100.md/conv.o ds3100.md/dviclass.o ds3100.md/error.o ds3100.md/findpost.o ds3100.md/fio.o ds3100.md/font.o ds3100.md/font_subr.o ds3100.md/gfclass.o ds3100.md/gffont.o ds3100.md/gripes.o ds3100.md/magfactor.o ds3100.md/pkfont.o ds3100.md/pxlfont.o ds3100.md/rotate.o ds3100.md/scaletfm.o ds3100.md/scanpost.o ds3100.md/search.o ds3100.md/seek.o ds3100.md/split.o ds3100.md/strsave.o ds3100.md/tfm.o ds3100.md/tfmfont.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
