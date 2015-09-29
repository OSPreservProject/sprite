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
# Thu Dec 17 17:02:15 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= dopage.c dosection.c dospecial.c download.c dpicheck.c drawPS.c dviinput.c dvips.c finclude.c flib.c fontdef.c header.c loadfont.c makefont.c output.c prescan.c repack.c resident.c scalewidth.c scanpage.c search.c skippage.c tfmload.c unpack.c virtualfont.c squeeze.c afm2tfm.c
HDRS		= debug.h paths.h structures.h
MDPUBHDRS	= 
OBJS		= ds3100.md/dopage.o ds3100.md/dosection.o ds3100.md/dospecial.o ds3100.md/download.o ds3100.md/dpicheck.o ds3100.md/drawPS.o ds3100.md/dviinput.o ds3100.md/dvips.o ds3100.md/finclude.o ds3100.md/flib.o ds3100.md/fontdef.o ds3100.md/header.o ds3100.md/loadfont.o ds3100.md/makefont.o ds3100.md/output.o ds3100.md/prescan.o ds3100.md/repack.o ds3100.md/resident.o ds3100.md/scalewidth.o ds3100.md/scanpage.o ds3100.md/search.o ds3100.md/skippage.o ds3100.md/tfmload.o ds3100.md/unpack.o ds3100.md/virtualfont.o ds3100.md/squeeze.o ds3100.md/afm2tfm.o
CLEANOBJS	= ds3100.md/dopage.o ds3100.md/dosection.o ds3100.md/dospecial.o ds3100.md/download.o ds3100.md/dpicheck.o ds3100.md/drawPS.o ds3100.md/dviinput.o ds3100.md/dvips.o ds3100.md/finclude.o ds3100.md/flib.o ds3100.md/fontdef.o ds3100.md/header.o ds3100.md/loadfont.o ds3100.md/makefont.o ds3100.md/output.o ds3100.md/prescan.o ds3100.md/repack.o ds3100.md/resident.o ds3100.md/scalewidth.o ds3100.md/scanpage.o ds3100.md/search.o ds3100.md/skippage.o ds3100.md/tfmload.o ds3100.md/unpack.o ds3100.md/virtualfont.o ds3100.md/squeeze.o ds3100.md/afm2tfm.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
