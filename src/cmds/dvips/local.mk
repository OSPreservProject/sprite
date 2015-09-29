#
# This file is included by Makefile.  Makefile is generated automatically
# by mkmf, and this file provides additional local personalization.  The
# variable SYSMAKEFILE is provided by Makefile;  it's a system Makefile
# that must be included to set up various compilation stuff.
#

# where the TFM files go
TFMDIR = /sprite/lib/fonts/tfm
# the default path to search for TFM files 
# (this usually is identical to TeX's defaultfontpath, which omits `.')
# (private fonts are given an explicit directory, which overrides the path)
# TFMPATH = /LocalLibrary/Fonts/TeXFonts/tfm:$(TFMDIR)
TFMPATH = $(TFMDIR)

# where the PK files go
# PKDIR = /usr/lib/tex/fonts/pk
PKDIR = /sprite/lib/fonts/pk
# the default path to search for PK files (usually omits `.')
# PKPATH = /LocalLibrary/Fonts/TeXFonts/pk:$(PKDIR)
PKPATH = $(PKDIR)

# where the VF files go (we don't have them)
# VFDIR = /usr/lib/tex/fonts/vf
VFDIR = /tic/tex/lib-fonts/vf
# the default path to search for VF files (usually omits `.')
# VFPATH = /LocalLibrary/Fonts/TeXFonts/vf:$(VFDIR)
VFPATH = $(VFDIR)

# where the config files go
# CONFIGDIR = /usr/lib/tex/ps
CONFIGDIR = /sprite/lib/ps

# the default path to search for config files
# CONFIGPATH = .:$(CONFIGDIR)
CONFIGPATH = .:$(CONFIGDIR):/sprite/src/cmds/dvips

# where the header PS files go
# HEADERDIR = /usr/lib/tex/ps
HEADERDIR = /sprite/lib/ps

# the default path to search for header files
# HEADERPATH = .:$(HEADERDIR)
HEADERPATH = .:$(HEADERDIR)

# where epsf.tex and rotate.tex go (usually the TeX macros directory)
# TEXMACRODIR = /usr/lib/tex/inputs
TEXMACRODIR = /sprite/lib/tex

# the default path to search for epsf and psfiles
# (usually the same as TeX's defaultinputpath)
# FIGPATH = .:..:/usr/lib/tex/inputs
FIGPATH = .:..:/sprite/lib/tex

# add -DDEBUG to turn on debugging capability
# add -DTPIC for tpic support
# - if the default resolution is not 300 dpi,
# add -DEFRES=400 or whatever is required
DEFS= -DTPIC -DDEBUG

# libraries to include
LIBS += -lm

PATHS = -DTFMPATH=\"$(TFMPATH)\" \
	-DPKPATH=\"$(PKPATH)\" \
	-DVFPATH=\"$(VFPATH)\" \
	-DHEADERPATH=\"$(HEADERPATH)\" \
	-DCONFIGPATH=\"$(CONFIGPATH)\" \
	-DFIGPATH=\"$(FIGPATH)\"

CFLAGS += $(DEFS) $(PATHS) 

#include	<$(SYSMAKEFILE)>


# 
# Note: check out Makefile.orig for the commands to create *.pro from *.lpro
# using "squeeze".  This stuff doesn't fit into the sprite mkmf format well
# so only the install commands are being included in local.mk
#
install ::
	$(UPDATE) -m 644 tex.pro $(HEADERDIR)
	$(UPDATE) -m 644 texc.pro $(HEADERDIR)
	$(UPDATE) -m 644 texps.pro $(HEADERDIR)
	$(UPDATE) -m 644 special.pro $(HEADERDIR)
	$(UPDATE) -m 644 config.ps $(CONFIGDIR)
	$(UPDATE) -m 644 psfonts.map $(CONFIGDIR)
	$(UPDATE) -m 644 epsf.tex $(TEXMACRODIR)
	$(UPDATE) -m 644 rotate.tex $(TEXMACRODIR)
