OBJS =		cccp.o cexp.o version.o

CFLAGS =	-O -I. -I../gcc

../bin/cpp: $(OBJS)
	$(CC) $(OBJS) -o ../bin/cpp

clean:
	rm -f *.o ../bin/cpp
