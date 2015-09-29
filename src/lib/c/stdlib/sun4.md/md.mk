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
# Mon Jun  8 14:35:03 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/memPC.s MemData.c MemDoTrace.c Mem_Bin.c MemInit.c MemPanic.c Mem_DumpTrace.c cfree.c Mem_Size.c abort.c bsearch.c calloc.c realloc.c Mem_PrintConfig.c Mem_PrintInUse.c Mem_PrintStats.c Mem_SetPrintProc.c Mem_SetTraceSizes.c abs.c div.c setenv.c atexit.c atof.c malloc.c atoi.c atol.c exit.c free.c getenv.c labs.c ldiv.c qsort.c rand.c strtod.c strtol.c strtoul.c system.c unsetenv.c MemChunkAlloc.c multibyte.c on_exit.c putenv.c
HDRS		= memInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/memPC.o sun4.md/MemData.o sun4.md/MemDoTrace.o sun4.md/Mem_Bin.o sun4.md/MemInit.o sun4.md/MemPanic.o sun4.md/Mem_DumpTrace.o sun4.md/cfree.o sun4.md/Mem_Size.o sun4.md/abort.o sun4.md/bsearch.o sun4.md/calloc.o sun4.md/realloc.o sun4.md/Mem_PrintConfig.o sun4.md/Mem_PrintInUse.o sun4.md/Mem_PrintStats.o sun4.md/Mem_SetPrintProc.o sun4.md/Mem_SetTraceSizes.o sun4.md/abs.o sun4.md/div.o sun4.md/setenv.o sun4.md/atexit.o sun4.md/atof.o sun4.md/malloc.o sun4.md/atoi.o sun4.md/atol.o sun4.md/exit.o sun4.md/free.o sun4.md/getenv.o sun4.md/labs.o sun4.md/ldiv.o sun4.md/qsort.o sun4.md/rand.o sun4.md/strtod.o sun4.md/strtol.o sun4.md/strtoul.o sun4.md/system.o sun4.md/unsetenv.o sun4.md/MemChunkAlloc.o sun4.md/multibyte.o sun4.md/on_exit.o sun4.md/putenv.o
CLEANOBJS	= sun4.md/memPC.o sun4.md/MemData.o sun4.md/MemDoTrace.o sun4.md/Mem_Bin.o sun4.md/MemInit.o sun4.md/MemPanic.o sun4.md/Mem_DumpTrace.o sun4.md/cfree.o sun4.md/Mem_Size.o sun4.md/abort.o sun4.md/bsearch.o sun4.md/calloc.o sun4.md/realloc.o sun4.md/Mem_PrintConfig.o sun4.md/Mem_PrintInUse.o sun4.md/Mem_PrintStats.o sun4.md/Mem_SetPrintProc.o sun4.md/Mem_SetTraceSizes.o sun4.md/abs.o sun4.md/div.o sun4.md/setenv.o sun4.md/atexit.o sun4.md/atof.o sun4.md/malloc.o sun4.md/atoi.o sun4.md/atol.o sun4.md/exit.o sun4.md/free.o sun4.md/getenv.o sun4.md/labs.o sun4.md/ldiv.o sun4.md/qsort.o sun4.md/rand.o sun4.md/strtod.o sun4.md/strtol.o sun4.md/strtoul.o sun4.md/system.o sun4.md/unsetenv.o sun4.md/MemChunkAlloc.o sun4.md/multibyte.o sun4.md/on_exit.o sun4.md/putenv.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
