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
# Mon Jun  8 14:34:53 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/memPC.s MemData.c MemDoTrace.c Mem_Bin.c MemInit.c MemPanic.c Mem_DumpTrace.c cfree.c Mem_Size.c abort.c bsearch.c calloc.c realloc.c Mem_PrintConfig.c Mem_PrintInUse.c Mem_PrintStats.c Mem_SetPrintProc.c Mem_SetTraceSizes.c abs.c div.c setenv.c atexit.c atof.c malloc.c atoi.c atol.c exit.c free.c getenv.c labs.c ldiv.c qsort.c rand.c strtod.c strtol.c strtoul.c system.c unsetenv.c MemChunkAlloc.c multibyte.c on_exit.c putenv.c
HDRS		= memInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/memPC.o sun3.md/MemData.o sun3.md/MemDoTrace.o sun3.md/Mem_Bin.o sun3.md/MemInit.o sun3.md/MemPanic.o sun3.md/Mem_DumpTrace.o sun3.md/cfree.o sun3.md/Mem_Size.o sun3.md/abort.o sun3.md/bsearch.o sun3.md/calloc.o sun3.md/realloc.o sun3.md/Mem_PrintConfig.o sun3.md/Mem_PrintInUse.o sun3.md/Mem_PrintStats.o sun3.md/Mem_SetPrintProc.o sun3.md/Mem_SetTraceSizes.o sun3.md/abs.o sun3.md/div.o sun3.md/setenv.o sun3.md/atexit.o sun3.md/atof.o sun3.md/malloc.o sun3.md/atoi.o sun3.md/atol.o sun3.md/exit.o sun3.md/free.o sun3.md/getenv.o sun3.md/labs.o sun3.md/ldiv.o sun3.md/qsort.o sun3.md/rand.o sun3.md/strtod.o sun3.md/strtol.o sun3.md/strtoul.o sun3.md/system.o sun3.md/unsetenv.o sun3.md/MemChunkAlloc.o sun3.md/multibyte.o sun3.md/on_exit.o sun3.md/putenv.o
CLEANOBJS	= sun3.md/memPC.o sun3.md/MemData.o sun3.md/MemDoTrace.o sun3.md/Mem_Bin.o sun3.md/MemInit.o sun3.md/MemPanic.o sun3.md/Mem_DumpTrace.o sun3.md/cfree.o sun3.md/Mem_Size.o sun3.md/abort.o sun3.md/bsearch.o sun3.md/calloc.o sun3.md/realloc.o sun3.md/Mem_PrintConfig.o sun3.md/Mem_PrintInUse.o sun3.md/Mem_PrintStats.o sun3.md/Mem_SetPrintProc.o sun3.md/Mem_SetTraceSizes.o sun3.md/abs.o sun3.md/div.o sun3.md/setenv.o sun3.md/atexit.o sun3.md/atof.o sun3.md/malloc.o sun3.md/atoi.o sun3.md/atol.o sun3.md/exit.o sun3.md/free.o sun3.md/getenv.o sun3.md/labs.o sun3.md/ldiv.o sun3.md/qsort.o sun3.md/rand.o sun3.md/strtod.o sun3.md/strtol.o sun3.md/strtoul.o sun3.md/system.o sun3.md/unsetenv.o sun3.md/MemChunkAlloc.o sun3.md/multibyte.o sun3.md/on_exit.o sun3.md/putenv.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
