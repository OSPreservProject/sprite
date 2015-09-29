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
# Mon Jun  8 14:35:13 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= MemData.c MemDoTrace.c Mem_Bin.c MemInit.c MemPanic.c Mem_DumpTrace.c cfree.c Mem_Size.c abort.c bsearch.c calloc.c realloc.c Mem_PrintConfig.c Mem_PrintInUse.c Mem_PrintStats.c Mem_SetPrintProc.c Mem_SetTraceSizes.c abs.c div.c setenv.c atexit.c atof.c malloc.c atoi.c atol.c exit.c free.c getenv.c labs.c ldiv.c qsort.c rand.c strtod.c strtol.c strtoul.c system.c unsetenv.c MemChunkAlloc.c multibyte.c on_exit.c putenv.c
HDRS		= memInt.h
MDPUBHDRS	= 
OBJS		= symm.md/MemData.o symm.md/MemDoTrace.o symm.md/Mem_Bin.o symm.md/MemInit.o symm.md/MemPanic.o symm.md/Mem_DumpTrace.o symm.md/cfree.o symm.md/Mem_Size.o symm.md/abort.o symm.md/bsearch.o symm.md/calloc.o symm.md/realloc.o symm.md/Mem_PrintConfig.o symm.md/Mem_PrintInUse.o symm.md/Mem_PrintStats.o symm.md/Mem_SetPrintProc.o symm.md/Mem_SetTraceSizes.o symm.md/abs.o symm.md/div.o symm.md/setenv.o symm.md/atexit.o symm.md/atof.o symm.md/malloc.o symm.md/atoi.o symm.md/atol.o symm.md/exit.o symm.md/free.o symm.md/getenv.o symm.md/labs.o symm.md/ldiv.o symm.md/qsort.o symm.md/rand.o symm.md/strtod.o symm.md/strtol.o symm.md/strtoul.o symm.md/system.o symm.md/unsetenv.o symm.md/MemChunkAlloc.o symm.md/multibyte.o symm.md/on_exit.o symm.md/putenv.o
CLEANOBJS	= symm.md/MemData.o symm.md/MemDoTrace.o symm.md/Mem_Bin.o symm.md/MemInit.o symm.md/MemPanic.o symm.md/Mem_DumpTrace.o symm.md/cfree.o symm.md/Mem_Size.o symm.md/abort.o symm.md/bsearch.o symm.md/calloc.o symm.md/realloc.o symm.md/Mem_PrintConfig.o symm.md/Mem_PrintInUse.o symm.md/Mem_PrintStats.o symm.md/Mem_SetPrintProc.o symm.md/Mem_SetTraceSizes.o symm.md/abs.o symm.md/div.o symm.md/setenv.o symm.md/atexit.o symm.md/atof.o symm.md/malloc.o symm.md/atoi.o symm.md/atol.o symm.md/exit.o symm.md/free.o symm.md/getenv.o symm.md/labs.o symm.md/ldiv.o symm.md/qsort.o symm.md/rand.o symm.md/strtod.o symm.md/strtol.o symm.md/strtoul.o symm.md/system.o symm.md/unsetenv.o symm.md/MemChunkAlloc.o symm.md/multibyte.o symm.md/on_exit.o symm.md/putenv.o
INSTFILES	= symm.md/md.mk Makefile
SACREDOBJS	= 
