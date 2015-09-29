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
# Wed Dec  4 21:26:14 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= exit.c rand.c atexit.c atoi.c MemChunkAlloc.c MemData.c MemDoTrace.c MemInit.c Mem_Bin.c Mem_DumpTrace.c Mem_PrintConfig.c Mem_PrintInUse.c Mem_PrintStats.c Mem_SetPrintProc.c Mem_SetTraceSizes.c Mem_Size.c abort.c atof.c malloc.c strtoul.c atol.c getenv.c setenv.c free.c
HDRS		= memInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/exit.o sun3.md/rand.o sun3.md/atexit.o sun3.md/atoi.o sun3.md/MemChunkAlloc.o sun3.md/MemData.o sun3.md/MemDoTrace.o sun3.md/MemInit.o sun3.md/Mem_Bin.o sun3.md/Mem_DumpTrace.o sun3.md/Mem_PrintConfig.o sun3.md/Mem_PrintInUse.o sun3.md/Mem_PrintStats.o sun3.md/Mem_SetPrintProc.o sun3.md/Mem_SetTraceSizes.o sun3.md/Mem_Size.o sun3.md/abort.o sun3.md/atof.o sun3.md/malloc.o sun3.md/strtoul.o sun3.md/atol.o sun3.md/getenv.o sun3.md/setenv.o sun3.md/free.o
CLEANOBJS	= sun3.md/exit.o sun3.md/rand.o sun3.md/atexit.o sun3.md/atoi.o sun3.md/MemChunkAlloc.o sun3.md/MemData.o sun3.md/MemDoTrace.o sun3.md/MemInit.o sun3.md/Mem_Bin.o sun3.md/Mem_DumpTrace.o sun3.md/Mem_PrintConfig.o sun3.md/Mem_PrintInUse.o sun3.md/Mem_PrintStats.o sun3.md/Mem_SetPrintProc.o sun3.md/Mem_SetTraceSizes.o sun3.md/Mem_Size.o sun3.md/abort.o sun3.md/atof.o sun3.md/malloc.o sun3.md/strtoul.o sun3.md/atol.o sun3.md/getenv.o sun3.md/setenv.o sun3.md/free.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
