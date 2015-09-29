##############################################
# Benchmark  to exercise a file system       #
# Distribution Version: 14 June 1985         #
#					     #
#	M. Satyanarayanan		     #
#	Information Technology Center	     #
#	Carnegie-Mellon University	     #
#					     #
#	Modified to be machine-indepedent by #
#	John Ousterhout			     #
#	University of California at Berkeley #
#	Current version: February 4, 1990    #
##############################################

# $Log:	Makefile,v $
# Revision 1.3  92/07/16  20:54:46  kupfer
# Use loadavg instead of rup, so that it works with the Sprite server.
# 
# Revision 1.2  92/05/10  17:41:32  kupfer
# Add commands to put local and server loads in the log before and after
# the run.  Add a command to put the system version in the log.  Pass
# "make" flags to the Phase V make.
# 

# To run this benchmark, do the following:
# 1. Make a subdirectory of this directory called "tmp", if it doesn't
#    exist already.
# 2. You may need to modify Makefile (in this directory) to set the ORIGINAL,
#    W, PS, and TIMESTAMP variables differently (the default settings should
#    work on most UNIX machines).
# 3. Cd to tmp
# 4. Type "make -f ../Makefile >& ../logfile".
#    (For Sprite, use "make -X -f ../Makefile >& ../logfile" to disable
#    process migration.)
# 4. The benchmark will run and leave its output in logfile in this
#    directory (not tmp;  leaving the log file there can cause strange
#    problems if you run the benchmark repeatedly).  Look through the
#    log file for timestamp lines corresponding to the different benchmark
#    phases.
# 7. Type "rm -rf tmp" to get rid of the junk created by the benchmark.

ORIGINAL = ..
# W = w; uptime
W = uptime; loadavg -a | egrep "allspice|lust"
OS_VERSION = sysstat -v
PS = ps -au
TIMESTAMP = date
MACHINEID = hostname
WHERE = pwd

benchmark:
	$(MACHINEID)
	$(OS_VERSION)
	$(WHERE)
	$(W)
#	$(PS)
	@echo "********* Timing begins here *********"
	$(TIMESTAMP)

# Directory creation
	@echo "Phase I: Creating directories"
	mkdir testfs1 testfs1/include testfs1/include/sys testfs1/include/netinet
	mkdir testfs2 testfs2/include testfs2/include/sys testfs2/include/netinet
	mkdir testfs3 testfs3/include testfs3/include/sys testfs3/include/netinet
	mkdir testfs4 testfs4/include testfs4/include/sys testfs4/include/netinet
	mkdir testfs5 testfs5/include testfs5/include/sys testfs5/include/netinet
	$(TIMESTAMP)

# Large-volume data movement
	@echo "********* Phase II: Copying files *********"
	cp $(ORIGINAL)/fscript/DrawString.c testfs1/DrawString.c
	cp $(ORIGINAL)/fscript/MakeLoadable.c testfs1/MakeLoadable.c
	cp $(ORIGINAL)/fscript/ProgramName.c testfs1/ProgramName.c
	cp $(ORIGINAL)/fscript/RasterFile.h testfs1/RasterFile.h
	cp $(ORIGINAL)/fscript/UserDrawS.c testfs1/UserDrawS.c
	cp $(ORIGINAL)/fscript/arbitrator.c testfs1/arbitrator.c
	cp $(ORIGINAL)/fscript/bres.s testfs1/bres.s
	cp $(ORIGINAL)/fscript/cmr.c testfs1/cmr.c
	cp $(ORIGINAL)/fscript/cvtalto.c testfs1/cvtalto.c
	cp $(ORIGINAL)/fscript/cvtfont.c testfs1/cvtfont.c
	cp $(ORIGINAL)/fscript/cvtv1v2.c testfs1/cvtv1v2.c
	cp $(ORIGINAL)/fscript/display.h testfs1/display.h
	cp $(ORIGINAL)/fscript/font.c testfs1/font.c
	cp $(ORIGINAL)/fscript/font.h testfs1/font.h
	cp $(ORIGINAL)/fscript/font.v0.h testfs1/font.v0.h
	cp $(ORIGINAL)/fscript/font.v1.h testfs1/font.v1.h
	cp $(ORIGINAL)/fscript/fontmanip.c testfs1/fontmanip.c
	cp $(ORIGINAL)/fscript/fontmanip.h testfs1/fontmanip.h
	cp $(ORIGINAL)/fscript/fontnaming.c testfs1/fontnaming.c
	cp $(ORIGINAL)/fscript/framebuf.h testfs1/framebuf.h
	cp $(ORIGINAL)/fscript/gacha12.c testfs1/gacha12.c
	cp $(ORIGINAL)/fscript/graphicops.c testfs1/graphicops.c
	cp $(ORIGINAL)/fscript/gxfind.c testfs1/gxfind.c
	cp $(ORIGINAL)/fscript/keymap.h testfs1/keymap.h
	cp $(ORIGINAL)/fscript/logo.c testfs1/logo.c
	cp $(ORIGINAL)/fscript/makefile testfs1/makefile
	cp $(ORIGINAL)/fscript/menu.c testfs1/menu.c
	cp $(ORIGINAL)/fscript/menu.h testfs1/menu.h
	cp $(ORIGINAL)/fscript/mkfont.c testfs1/mkfont.c
	cp $(ORIGINAL)/fscript/mksail7.c testfs1/mksail7.c
	cp $(ORIGINAL)/fscript/mouse.c testfs1/mouse.c
	cp $(ORIGINAL)/fscript/mousedd.c testfs1/mousedd.c
	cp $(ORIGINAL)/fscript/profile.c testfs1/profile.c
	cp $(ORIGINAL)/fscript/putenv.c testfs1/putenv.c
	cp $(ORIGINAL)/fscript/rasterop.c testfs1/rasterop.c
	cp $(ORIGINAL)/fscript/sail7.c testfs1/sail7.c
	cp $(ORIGINAL)/fscript/sun1bw.c testfs1/sun1bw.c
	cp $(ORIGINAL)/fscript/sun1color.c testfs1/sun1color.c
	cp $(ORIGINAL)/fscript/sunbitmap.c testfs1/sunbitmap.c
	cp $(ORIGINAL)/fscript/suncolor.c testfs1/suncolor.c
	cp $(ORIGINAL)/fscript/test.c testfs1/test.c
	cp $(ORIGINAL)/fscript/timetest.c testfs1/timetest.c
	cp $(ORIGINAL)/fscript/towindow.c testfs1/towindow.c
	cp $(ORIGINAL)/fscript/usergraphics.c testfs1/usergraphics.c
	cp $(ORIGINAL)/fscript/usergraphics.h testfs1/usergraphics.h
	cp $(ORIGINAL)/fscript/util.h testfs1/util.h
	cp $(ORIGINAL)/fscript/vec.c testfs1/vec.c
	cp $(ORIGINAL)/fscript/window.c testfs1/window.c
	cp $(ORIGINAL)/fscript/window.h testfs1/window.h
	cp $(ORIGINAL)/fscript/windowops.c testfs1/windowops.c
	cp $(ORIGINAL)/fscript/wm.c testfs1/wm.c
	cp $(ORIGINAL)/fscript/include/assert.h testfs1/include/assert.h
	cp $(ORIGINAL)/fscript/include/colorbuf.h testfs1/include/colorbuf.h
	cp $(ORIGINAL)/fscript/include/ctype.h testfs1/include/ctype.h
	cp $(ORIGINAL)/fscript/include/errno.h testfs1/include/errno.h
	cp $(ORIGINAL)/fscript/include/fcntl.h testfs1/include/fcntl.h
	cp $(ORIGINAL)/fscript/include/netdb.h testfs1/include/netdb.h
	cp $(ORIGINAL)/fscript/include/sgtty.h testfs1/include/sgtty.h
	cp $(ORIGINAL)/fscript/include/signal.h testfs1/include/signal.h
	cp $(ORIGINAL)/fscript/include/stdio.h testfs1/include/stdio.h
	cp $(ORIGINAL)/fscript/include/time.h testfs1/include/time.h
	cp $(ORIGINAL)/fscript/include/netinet/in.h testfs1/include/netinet/in.h
	cp $(ORIGINAL)/fscript/include/sys/dir.h testfs1/include/sys/dir.h
	cp $(ORIGINAL)/fscript/include/sys/ioctl.h testfs1/include/sys/ioctl.h
	cp $(ORIGINAL)/fscript/include/sys/socket.h testfs1/include/sys/socket.h
	cp $(ORIGINAL)/fscript/include/sys/stat.h testfs1/include/sys/stat.h
	cp $(ORIGINAL)/fscript/include/sys/ttychars.h testfs1/include/sys/ttychars.h
	cp $(ORIGINAL)/fscript/include/sys/ttydev.h testfs1/include/sys/ttydev.h
	cp $(ORIGINAL)/fscript/include/sys/types.h testfs1/include/sys/types.h
	cp $(ORIGINAL)/fscript/include/sys/wait.h testfs1/include/sys/wait.h
	$(TIMESTAMP)


# Intensive stat()ing
	@echo "********* Phase III: Recursive directory stats *********"
	find .  -print -exec ls -l {} \; 
	du -s *
#######	find .  -print -exec touch {} \; ####### ORIGINAL
	$(TIMESTAMP)

# Exercises proportional to length of file
	@echo "********* Phase IV: Scanning each file *********"
	find .  -exec grep kangaroo {} \;
	find . -exec wc {} \;
	$(TIMESTAMP)
	
# Computationally intensive stuff
	@echo "********* Phase V: Compilation *********"
	-cd testfs1; make $(MFLAGS)
	$(TIMESTAMP)

# Terminate
	@echo "********* Timing ends here *********"
	$(W)
#	$(PS)
