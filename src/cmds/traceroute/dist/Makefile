INCL =
CFLAGS = -O
LIBS =

traceroute: traceroute.c
	cc $(CFLAGS) $(INCL) -o traceroute traceroute.c $(LIBS)

install: traceroute
	install -o root -g bin -m 4555 traceroute ${DESTDIR}/etc/traceroute 

lint:
	lint -b -h -x $(INCL) traceroute.c | \
	  grep -v 'possible pointer alignment problem'
