all: pslplain-b.fmt pslplain-h.fmt pslplain-n.fmt pslplain-p.fmt pslplain-t.fmt

SRCS=pslplain.tex lfonts.tex

pslplain-t.fmt: $(SRCS) fntchoice-t.tex
	ln fntchoice-t.tex fntchoice.tex
	ln pslplain.tex pslplain-t.tex
	initex pslplain-t '\dump'
	rm -f fntchoice.tex pslplain-t.tex

pslplain-b.fmt: $(SRCS) fntchoice-b.tex
	ln fntchoice-b.tex fntchoice.tex
	ln pslplain.tex pslplain-b.tex
	initex pslplain-b '\dump'
	rm -f fntchoice.tex pslplain-b.tex

pslplain-h.fmt: $(SRCS) fntchoice-h.tex
	ln fntchoice-h.tex fntchoice.tex
	ln pslplain.tex pslplain-h.tex
	initex pslplain-h '\dump'
	rm -f fntchoice.tex pslplain-h.tex

pslplain-n.fmt: $(SRCS) fntchoice-n.tex
	ln fntchoice-n.tex fntchoice.tex
	ln pslplain.tex pslplain-n.tex
	initex pslplain-n '\dump'
	rm -f fntchoice.tex pslplain-n.tex

pslplain-p.fmt: $(SRCS) fntchoice-p.tex
	ln fntchoice-p.tex fntchoice.tex
	ln pslplain.tex pslplain-p.tex
	initex pslplain-p '\dump'
	rm -f fntchoice.tex pslplain-p.tex

FMT=/usr/lib/tex/formats

install: $(FMT)/pslplain.fmt $(FMT)/pslplain-b.fmt $(FMT)/pslplain-h.fmt \
	 $(FMT)/pslplain-n.fmt $(FMT)/pslplain-p.fmt 

$(FMT)/pslplain.fmt: pslplain-t.fmt ; cp pslplain-t.fmt $(FMT)/pslplain.fmt
$(FMT)/pslplain-b.fmt: pslplain-b.fmt ; cp pslplain-b.fmt $(FMT)
$(FMT)/pslplain-h.fmt: pslplain-h.fmt ; cp pslplain-h.fmt $(FMT)
$(FMT)/pslplain-n.fmt: pslplain-n.fmt ; cp pslplain-n.fmt $(FMT)
$(FMT)/pslplain-p.fmt: pslplain-p.fmt ; cp pslplain-p.fmt $(FMT)

DERIVEDTFM =b-ligobl.tfm \
	    b-ligsc.tfm \
	    c-con.tfm \
	    h-red.tfm \
	    h-sc.tfm \
	    ncs-obl.tfm \
	    ncs-romsc.tfm \
	    p-romsc.tfm \
	    t-bolobl.tfm \
	    t-romsc.tfm
DERIVEDPL  =b-ligobl.pl \
	    b-ligsc.pl \
	    c-con.pl \
	    h-red.pl \
	    h-sc.pl \
	    ncs-obl.pl \
	    ncs-romsc.pl \
	    p-romsc.pl \
	    t-bolobl.pl \
	    t-romsc.pl
DERIVEDAFM =b-ligobl.afm \
	    b-ligsc.afm \
	    c-con.afm \
	    h-red.afm \
	    h-sc.afm \
	    ncs-obl.afm \
	    ncs-romsc.afm \
	    p-romsc.afm \
	    t-bolobl.afm \
	    t-romsc.afm

.SUFFIXES : .pl .tfm .afm .tex .dvi

.afm.pl :
	aftopl $*.afm >$*.pl

.pl.tfm :
	pltotf $*.pl $*.tfm

tfms : $(DERIVEDTFM)

$(DERIVEDTFM) : $(DERIVEDPL)

install-tfms: tfms
	cp $(DERIVEDTFM) /usr/lib/tex/fonts

.tex.dvi :
	pslatex $*

doc: pslatex.dvi fonts.dvi
	cp pslatex.tex pslatex.dvi /usr/lib/tex/Doc

pslatex.dvi: pslatex.tex

fonts.dvi: fonts.tex

DIST=README BUGS Makefile cnvfonts long2sh fntchoice-b.tex fntchoice-h.tex \
	fntchoice-n.tex fntchoice-p.tex fntchoice-t.tex \
	font-table fonts.tex lfonts.tex makefonts.ps pslatex.tex \
	pslplain.tex 

dist: $(DIST) $(DERIVEDPL)
	shar $(DIST) $(DERIVEDPL) >dist

#distb: $(DIST) 
#	tar cvhf - $(DIST) psfonts | compress | btoa >distb

