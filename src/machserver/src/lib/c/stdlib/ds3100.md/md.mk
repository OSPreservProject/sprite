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
# Tue Apr 14 16:47:06 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= exit.c rand.c atexit.c atoi.c MemData.c MemDoTrace.c MemInit.c Mem_Bin.c Mem_DumpTrace.c Mem_PrintConfig.c Mem_PrintInUse.c Mem_PrintStats.c Mem_SetPrintProc.c Mem_SetTraceSizes.c Mem_Size.c abort.c abs.c atof.c malloc.c strtoul.c MemChunkAlloc.c atol.c bsearch.c calloc.c cfree.c div.c free.c getenv.c labs.c ldiv.c qsort.c realloc.c setenv.c strtod.c strtol.c system.c unsetenv.c
HDRS		= memInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/MemChunkAlloc.o ds3100.md/MemData.o ds3100.md/MemDoTrace.o ds3100.md/MemInit.o ds3100.md/Mem_Bin.o ds3100.md/Mem_DumpTrace.o ds3100.md/Mem_PrintConfig.o ds3100.md/Mem_PrintInUse.o ds3100.md/Mem_PrintStats.o ds3100.md/Mem_SetPrintProc.o ds3100.md/Mem_SetTraceSizes.o ds3100.md/Mem_Size.o ds3100.md/abort.o ds3100.md/abs.o ds3100.md/atexit.o ds3100.md/atof.o ds3100.md/atoi.o ds3100.md/atol.o ds3100.md/bsearch.o ds3100.md/calloc.o ds3100.md/cfree.o ds3100.md/div.o ds3100.md/exit.o ds3100.md/free.o ds3100.md/getenv.o ds3100.md/labs.o ds3100.md/ldiv.o ds3100.md/malloc.o ds3100.md/qsort.o ds3100.md/rand.o ds3100.md/realloc.o ds3100.md/setenv.o ds3100.md/strtod.o ds3100.md/strtol.o ds3100.md/strtoul.o ds3100.md/unsetenv.o ds3100.md/system.o
CLEANOBJS	= ds3100.md/exit.o ds3100.md/rand.o ds3100.md/atexit.o ds3100.md/atoi.o ds3100.md/MemData.o ds3100.md/MemDoTrace.o ds3100.md/MemInit.o ds3100.md/Mem_Bin.o ds3100.md/Mem_DumpTrace.o ds3100.md/Mem_PrintConfig.o ds3100.md/Mem_PrintInUse.o ds3100.md/Mem_PrintStats.o ds3100.md/Mem_SetPrintProc.o ds3100.md/Mem_SetTraceSizes.o ds3100.md/Mem_Size.o ds3100.md/abort.o ds3100.md/abs.o ds3100.md/atof.o ds3100.md/malloc.o ds3100.md/strtoul.o ds3100.md/MemChunkAlloc.o ds3100.md/atol.o ds3100.md/bsearch.o ds3100.md/calloc.o ds3100.md/cfree.o ds3100.md/div.o ds3100.md/free.o ds3100.md/getenv.o ds3100.md/labs.o ds3100.md/ldiv.o ds3100.md/qsort.o ds3100.md/realloc.o ds3100.md/setenv.o ds3100.md/strtod.o ds3100.md/strtol.o ds3100.md/system.o ds3100.md/unsetenv.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
