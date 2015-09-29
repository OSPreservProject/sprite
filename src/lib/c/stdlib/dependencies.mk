# DO NOT DELETE THIS LINE -- make depend depends on it.

MemChunkAlloc.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h stdio.h
MemChunkAlloc.o: sys/types.h sys/sysmacros.h
MemDoTrace.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
MemInit.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
MemPanic.o: sprite.h status.h sys.h proc.h time.h sig.h sigMach.h
Mem_Bin.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_DumpTrace.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_PrintConfig.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_PrintInUse.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_PrintStats.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_SetPrintProc.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_SetTraceSizes.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
Mem_Size.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
abort.o: sprite.h status.h proc.h time.h sig.h sigMach.h
atof.o: ctype.h
atoi.o: ctype.h
atol.o: ctype.h
calloc.o: bstring.h stdlib.h
div.o: stdlib.h
exit.o: sprite.h status.h proc.h time.h sig.h sigMach.h
free.o: memInt.h syncMonitor.h sprite.h status.h stdlib.h
ldiv.o: stdlib.h
malloc.o: stdlib.h memInt.h syncMonitor.h sprite.h status.h
rand.o: stdio.h
realloc.o: bstring.h stdlib.h
setenv.o: stdlib.h
strtod.o: ctype.h
strtol.o: ctype.h
strtoul.o: ctype.h
system.o: signal.h sys/wait.h
