# Makefile to generate wais index of sprite howto info
# Use "make -X" or else the nroff will get messed up by migration

WAISDIR = /local/src/cmds/waissearch/wais-sources

default: textfromman indexem

text = ChangeUltrixClient X11R3_colors X11R3_defaults X11R3_fonts \
    X11R3_getStarted addNewHost addNewUser bootFromUnix doAPort.txt \
    convertToGcc createNewMachineType deleteAnAccount doADump \
    doNeatStuffOnASymmetry fixPrinterQueue installKernels installProg \
    scvs.txt makeKernelChanges recoverWrongMxFile restoreAFile \
    setPromEnvironment sparcProm startX11R4 systemMachineDependencies \
    understandX11R5 understand_imake updateGcc server-info/allspice.debug.txt \
    server-info/assault.debug.txt

textfromman: $(text)

indexem:	howto.dct

howto.dct:	$(text)
	waisindex -d howto -t text $(text)

doAPort.txt: doAPort.me
	nroff -me doAPort.me | sed -e 's/_//g' > doAPort.txt

scvs.txt: scvs
	nroff -me scvs | sed -e 's/_//g' > scvs.txt

server-info/allspice.debug.txt:	server-info/allspice.debug
	nroff -me server-info/allspice.debug | sed -e 's/_//g' > server-info/allspice.debug.txt

server-info/assault.debug.txt:	server-info/assault.debug
	nroff -me server-info/assault.debug | sed -e 's/_//g' > server-info/assault.debug.txt

