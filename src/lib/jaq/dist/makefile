# choose one: gcc, cc
CC = cc

# choose one: sunos, ultrix, hpux, OSF1
SYSTEM = ultrix

# add -DSYSV for hpux and other system-V atrocities
# add -DHASLIMITSH if your system has limits.h otherwise
#                  you'll have to define INT_MAX and SHORT_MAX in jaquith.h
# add -DHASSTDLIBH if your system has stdlib.h
# add -DHASSTRTOK if your system has the strtok procedure
# add -DMEMDEBUG to enable memory diagnostics
CFLAGS = -g -D$(SYSTEM) -DHASLIMITSH -DHASSTDLIBH -DHASSTRTOK
SPECIALCFLAGS = -DDRIVER
#
LDFLAGS = 

# uncomment these if you don't have strtod and strtol
#STDLIBSRC = stdlib.c
#STDLIBOBJ = stdlib.o

# uncomment these if you're using a Metrum robot on Ultrix
#ASACASRC = asaca.c
#ASACAOBJ = asaca.o

TCLLIBS = -L/usr/sww/X11/lib -L/usr/sww/lib
TCLINCLIBS = -I/usr/sww/include/tcl -I/usr/sww/include/tk

INSTALLBINDIR = /jaquith/cmds
INSTALLMANDIR = /jaquith/man

# ----------------------- end of options ------------------------

LIBSRC = admin.c dev.c hash.c indx.c lock.c mem.c option.c queue.c regexp.c sock.c str.c tbuf.c tlog.c ttime.c utils.c $(STDLIBSRC) $(ASACASRC)

LIBOBJ = admin.o dev.o hash.o indx.o lock.o mem.o option.o queue.o regexp.o sock.o str.o tbuf.o tlog.o ttime.o utils.o $(STDLIBOBJ) $(ASACAOBJ)

all: libjaq.a jaquith jupdate jfetch jclean jmgr jquery jls jput jget jstat jcopy jctrl jbuild

libjaq.a: $(LIBOBJ)
	ar r libjaq.a $(LIBOBJ)
	ranlib libjaq.a

# Special for Metrum robot on Ultrix
asaca.o: asaca.h asaca.c
	$(CC) $(CFLAGS) $(SPECIALCFLAGS) -c asaca.c

jaquith: jaquith.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jaquith jaquith.o libjaq.a

jmgr: jmgr.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jmgr jmgr.o libjaq.a

jclean: jclean.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jclean jclean.o libjaq.a

jupdate: jupdate.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jupdate jupdate.o libjaq.a

jfetch: jfetch.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jfetch jfetch.o libjaq.a

jquery: jquery.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jquery jquery.o libjaq.a

jls: jls.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jls jls.o libjaq.a

jput: jput.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jput jput.o libjaq.a

jget: jget.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jget jget.o libjaq.a

jstat: jstat.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jstat jstat.o libjaq.a

jcopy: jcopy.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jcopy jcopy.o libjaq.a

jctrl: jctrl.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jctrl jctrl.o libjaq.a

jbuild: jbuild.o libjaq.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o jbuild jbuild.o libjaq.a

clean:
	rm -f *.o *~ jaquith jmgr jquery jupdate jfetch jput jget jls jstat jclean jctrl jcopy jbuild

install: all
	csh -c "cp {jaquith,jmgr,jquery,jupdate,jfetch,jput,jget,jls,jstat,jclean,jctrl,jcopy,jbuild} $(INSTALLBINDIR)"
	csh -c "cp {jaquith,jmgr,jput,jget,jls,jstat,jclean,jctrl,jcopy,jbuild}.man $(INSTALLMANDIR)"
