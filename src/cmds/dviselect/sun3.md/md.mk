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
# Thu Dec 17 17:03:06 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= dviselect.c conv.c dviclass.c error.c findpost.c fio.c font.c font_subr.c gfclass.c gffont.c gripes.c magfactor.c pkfont.c pxlfont.c rotate.c scaletfm.c scanpost.c search.c seek.c split.c strsave.c tfm.c tfmfont.c
HDRS		= arith.h binding.h box.h conv.h dvi.h dviclass.h dvicodes.h error.h fio.h font.h gfclass.h gfcodes.h imPcodes.h imagen.h io.h num.h postamble.h search.h str.h tfm.h types.h verser.h
MDPUBHDRS	= 
OBJS		= sun3.md/dviselect.o sun3.md/conv.o sun3.md/dviclass.o sun3.md/error.o sun3.md/findpost.o sun3.md/fio.o sun3.md/font.o sun3.md/font_subr.o sun3.md/gfclass.o sun3.md/gffont.o sun3.md/gripes.o sun3.md/magfactor.o sun3.md/pkfont.o sun3.md/pxlfont.o sun3.md/rotate.o sun3.md/scaletfm.o sun3.md/scanpost.o sun3.md/search.o sun3.md/seek.o sun3.md/split.o sun3.md/strsave.o sun3.md/tfm.o sun3.md/tfmfont.o
CLEANOBJS	= sun3.md/dviselect.o sun3.md/conv.o sun3.md/dviclass.o sun3.md/error.o sun3.md/findpost.o sun3.md/fio.o sun3.md/font.o sun3.md/font_subr.o sun3.md/gfclass.o sun3.md/gffont.o sun3.md/gripes.o sun3.md/magfactor.o sun3.md/pkfont.o sun3.md/pxlfont.o sun3.md/rotate.o sun3.md/scaletfm.o sun3.md/scanpost.o sun3.md/search.o sun3.md/seek.o sun3.md/split.o sun3.md/strsave.o sun3.md/tfm.o sun3.md/tfmfont.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
