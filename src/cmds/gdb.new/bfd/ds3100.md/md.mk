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
# Mon Sep  9 18:07:53 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= srec.c aout32.c aout64.c archive.c archures.c bfd.c bout.c cache.c core.c demo64.c ecoff.c filemode.c format.c host-aout.c i386coff.c icoff.c ieee.c libbfd.c m68kcoff.c m88k-bcs.c newsos3.c oasys.c opncls.c reloc.c section.c sunos.c syms.c targets.c trad-core.c
HDRS		= aoutf1.h aoutx.h coffcode.h libaout.h libbfd.h libcoff.h libieee.h liboasys.h malloc.h trad-core.h
MDPUBHDRS	= 
OBJS		= ds3100.md/srec.o ds3100.md/aout32.o ds3100.md/aout64.o ds3100.md/archive.o ds3100.md/archures.o ds3100.md/bfd.o ds3100.md/bout.o ds3100.md/cache.o ds3100.md/core.o ds3100.md/demo64.o ds3100.md/ecoff.o ds3100.md/filemode.o ds3100.md/format.o ds3100.md/host-aout.o ds3100.md/i386coff.o ds3100.md/icoff.o ds3100.md/ieee.o ds3100.md/libbfd.o ds3100.md/m68kcoff.o ds3100.md/m88k-bcs.o ds3100.md/newsos3.o ds3100.md/oasys.o ds3100.md/opncls.o ds3100.md/reloc.o ds3100.md/section.o ds3100.md/sunos.o ds3100.md/syms.o ds3100.md/targets.o ds3100.md/trad-core.o
CLEANOBJS	= ds3100.md/srec.o ds3100.md/aout32.o ds3100.md/aout64.o ds3100.md/archive.o ds3100.md/archures.o ds3100.md/bfd.o ds3100.md/bout.o ds3100.md/cache.o ds3100.md/core.o ds3100.md/demo64.o ds3100.md/ecoff.o ds3100.md/filemode.o ds3100.md/format.o ds3100.md/host-aout.o ds3100.md/i386coff.o ds3100.md/icoff.o ds3100.md/ieee.o ds3100.md/libbfd.o ds3100.md/m68kcoff.o ds3100.md/m88k-bcs.o ds3100.md/newsos3.o ds3100.md/oasys.o ds3100.md/opncls.o ds3100.md/reloc.o ds3100.md/section.o ds3100.md/sunos.o ds3100.md/syms.o ds3100.md/targets.o ds3100.md/trad-core.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
