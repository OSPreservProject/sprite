#
# If your system has a system include file "alloca.h", make sure there
# is no file by that name in this directory (rename it to alloca.h.dummy).
# If your system doesn't define the include file "alloca.h", make sure a
# dummy version exists in this directory (raname alloca.h.dummy to
# alloca.h if alloca.h isn't here already).

OBJS =		gcc.o obstack.o version.o

CFLAGS =	-O

../bin/gcc:	$(OBJS)
	$(CC) $(OBJS) -o ../bin/gcc

clean:
	rm -f *.o ../bin/gcc
