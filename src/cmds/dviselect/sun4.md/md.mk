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
# Thu Dec 17 17:03:13 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= dviselect.c conv.c dviclass.c error.c findpost.c fio.c font.c font_subr.c gfclass.c gffont.c gripes.c magfactor.c pkfont.c pxlfont.c rotate.c scaletfm.c scanpost.c search.c seek.c split.c strsave.c tfm.c tfmfont.c
HDRS		= arith.h binding.h box.h conv.h dvi.h dviclass.h dvicodes.h error.h fio.h font.h gfclass.h gfcodes.h imPcodes.h imagen.h io.h num.h postamble.h search.h str.h tfm.h types.h verser.h
MDPUBHDRS	= 
OBJS		= sun4.md/conv.o sun4.md/dviclass.o sun4.md/dviselect.o sun4.md/error.o sun4.md/findpost.o sun4.md/fio.o sun4.md/font.o sun4.md/font_subr.o sun4.md/gfclass.o sun4.md/gffont.o sun4.md/gripes.o sun4.md/magfactor.o sun4.md/pkfont.o sun4.md/pxlfont.o sun4.md/rotate.o sun4.md/scaletfm.o sun4.md/scanpost.o sun4.md/search.o sun4.md/seek.o sun4.md/split.o sun4.md/strsave.o sun4.md/tfm.o sun4.md/tfmfont.o
CLEANOBJS	= sun4.md/dviselect.o sun4.md/conv.o sun4.md/dviclass.o sun4.md/error.o sun4.md/findpost.o sun4.md/fio.o sun4.md/font.o sun4.md/font_subr.o sun4.md/gfclass.o sun4.md/gffont.o sun4.md/gripes.o sun4.md/magfactor.o sun4.md/pkfont.o sun4.md/pxlfont.o sun4.md/rotate.o sun4.md/scaletfm.o sun4.md/scanpost.o sun4.md/search.o sun4.md/seek.o sun4.md/split.o sun4.md/strsave.o sun4.md/tfm.o sun4.md/tfmfont.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
