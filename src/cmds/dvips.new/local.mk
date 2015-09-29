#
# local.mk for dvips
#
TFMDIR	= 
TFMPATH = /sprite/lib/tex/tfm:/sprite/lib/tex/ps-tfm
PKPATH	= /sprite/lib/fonts/pk
VFPATH	= /tic/tex/lib-fonts/ps-vf
#
DVIPSDIR	= /sprite/lib/dvips
CONFIGPATH	= .:$(DVIPSDIR)
CONFIGFILE	= config.all
HEADERPATH	= .:$(DVIPSDIR)
#
TEXMACRODIR	= /sprite/lib/tex/inputs
FIGPATH		= .:..:/sprite/lib/tex/inputs
#
PATHS = -DTFMPATH=\"$(TFMPATH)\" \
	-DPKPATH=\"$(PKPATH)\" \
	-DVFPATH=\"$(VFPATH)\" \
	-DHEADERPATH=\"$(HEADERPATH)\" \
	-DCONFIGPATH=\"$(CONFIGPATH)\" \
	-DCONFIGFILE=\"$(CONFIGFILE)\" \
	-DFIGPATH=\"$(FIGPATH)\"
#
####################
#
# add -DDEBUG to turn on debugging capability
# add -DTPIC for tpic support
## if the default resolution is not 300 dpi,
# add -DDEFRES=400 or whatever is required
# add -DFONTLIB to search font libraries
# add -DSEARCH_SUBDIRECTORIES to search the FONTSUBDIRPATH.
# add -DHAVE_GETCWD if you have getcwd (relevant only for subdir searching)
DEFS	= -DTPIC -DDEBUG
LIBS	+= -lm
CFLAGS += $(DEFS) $(PATHS)

#include	<$(SYSMAKEFILE)>

# 
# Note: check out Makefile.orig for the commands to create *.pro from *.lpro
# using "squeeze".  This stuff doesn't fit into the sprite mkmf format well
# so only the install commands are being included in local.mk
#
install :: $(TM).md/afm2tfm $(TM).md/squeeze tex.pro texc.pro texps.pro\
	   special.pro finclude.pro
	$(UPDATE) -m 644 tex.pro $(DVIPSDIR)
	$(UPDATE) -m 644 texc.pro $(DVIPSDIR)
	$(UPDATE) -m 644 texps.pro $(DVIPSDIR)
	$(UPDATE) -m 644 special.pro $(DVIPSDIR)
	$(UPDATE) -m 644 config.ps $(DVIPSDIR)/$(CONFIGFILE)
	$(UPDATE) -m 644 psfonts.map $(DVIPSDIR)
	$(UPDATE) -m 644 epsf.tex $(TEXMACRODIR)
	$(UPDATE) -m 644 rotate.tex $(TEXMACRODIR)
	$(UPDATE) -m 644 rotate.sty $(TEXMACRODIR)

$(TM).md/afm2tfm: afm2tfm.c-
	mv afm2tfm.c- afm2tfm.c
	$(CC) $(CFLAGS) -o $(TM).md/afm2tfm afm2tfm.c -lm
	mv afm2tfm.c afm2tfm.c-

$(TM).md/squeeze: squeeze.c-
	mv squeeze.c- squeeze.c
	$(CC) $(CFLAGS) -o $(TM).md/squeeze squeeze.c
	mv squeeze.c squeeze.c-

tex.pro : tex.lpro $(TM).md/squeeze
	$(TM).md/squeeze <tex.lpro > tex.pro

texc.pro: texc.lpro $(TM).md/squeeze
	$(TM).md/squeeze <texc.lpro >texc.pro

texc.lpro: texc.script tex.lpro
	./texc.script tex.lpro texc.lpro

texps.pro : texps.lpro $(TM).md/squeeze
	$(TM).md/squeeze <texps.lpro >texps.pro

special.pro : special.lpro $(TM).md/squeeze
	$(TM).md/squeeze <special.lpro >special.pro

finclude.pro : finclude.lpro $(TM).md/squeeze
	$(TM).md/squeeze <finclude.lpro >finclude.pro
