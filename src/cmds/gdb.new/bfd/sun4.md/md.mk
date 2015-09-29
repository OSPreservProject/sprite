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
# Mon Sep  9 18:08:01 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= srec.c aout32.c aout64.c archive.c archures.c bfd.c bout.c cache.c core.c demo64.c ecoff.c filemode.c format.c host-aout.c i386coff.c icoff.c ieee.c libbfd.c m68kcoff.c m88k-bcs.c newsos3.c oasys.c opncls.c reloc.c section.c sunos.c syms.c targets.c trad-core.c
HDRS		= aoutf1.h aoutx.h coffcode.h libaout.h libbfd.h libcoff.h libieee.h liboasys.h malloc.h trad-core.h
MDPUBHDRS	= 
OBJS		= sun4.md/srec.o sun4.md/aout32.o sun4.md/aout64.o sun4.md/archive.o sun4.md/archures.o sun4.md/bfd.o sun4.md/bout.o sun4.md/cache.o sun4.md/core.o sun4.md/demo64.o sun4.md/ecoff.o sun4.md/filemode.o sun4.md/format.o sun4.md/host-aout.o sun4.md/i386coff.o sun4.md/icoff.o sun4.md/ieee.o sun4.md/libbfd.o sun4.md/m68kcoff.o sun4.md/m88k-bcs.o sun4.md/newsos3.o sun4.md/oasys.o sun4.md/opncls.o sun4.md/reloc.o sun4.md/section.o sun4.md/sunos.o sun4.md/syms.o sun4.md/targets.o sun4.md/trad-core.o
CLEANOBJS	= sun4.md/srec.o sun4.md/aout32.o sun4.md/aout64.o sun4.md/archive.o sun4.md/archures.o sun4.md/bfd.o sun4.md/bout.o sun4.md/cache.o sun4.md/core.o sun4.md/demo64.o sun4.md/ecoff.o sun4.md/filemode.o sun4.md/format.o sun4.md/host-aout.o sun4.md/i386coff.o sun4.md/icoff.o sun4.md/ieee.o sun4.md/libbfd.o sun4.md/m68kcoff.o sun4.md/m88k-bcs.o sun4.md/newsos3.o sun4.md/oasys.o sun4.md/opncls.o sun4.md/reloc.o sun4.md/section.o sun4.md/sunos.o sun4.md/syms.o sun4.md/targets.o sun4.md/trad-core.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
