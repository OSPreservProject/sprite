# $Header: /sprite/src/cmds/pmake/doc/RCS/Makefile,v 1.3 90/12/10 17:06:47 kupfer Exp $

LOCALS		= tutorial.ms tutorial.psc \
                  pmake.mansp pmake.psc \
		  Lst.mansp Lst.psc Makefile makefile tmac.ansp \
                  cctrl.8 customs.8 importquota.8 reginfo.1

SUBDIRS		= index ex1 ex2 customs


tutorial.t	: index/tmac.index tutorial.ms 
	ditroff -Plw -ms -t $(.ALLSRC) > tut.temp
	(cd index; make index)
	-index/index -Plw -t tut.temp > $(.TARGET)
	rm -f tut.temp index/index

print 		: tutorial.t
	lpr -Plw -h -n tutorial.t

package		::
	if [ ! -d $(PKGDIR) ]; then
		mkdir $(PKGDIR)
	fi
	cp $(LOCALS) $(PKGDIR)
	for i in $(SUBDIRS); do
		(cd $i; $(MAKE) "PKGDIR=$(PKGDIR)/$i" package)
	done

# This was the entry for "package" in "makefile".  Don't ask me what
# the difference is between this one and the other "package".  All I
# know is that having both a Makefile and a makefile is seriously
# demented.	-kupfer

package_other	: 
	-mkdir $(PKGDIR)
	-mkdir $(PKGDIR)/index
	cp *.mansp *.ansp *.ms *.psc Makefile makefile $(PKGDIR)
	cp index/tmac.index index/index.c index/Makefile index/README \
            $(PKGDIR)/index
