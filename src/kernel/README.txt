
This directory contains the Sprite kernel sources. The repository for
the sources is the directory "Cvsroot". This directory is maintained
by the "scvs" program, which is a script for the "cvs" program, which
in turn is built on top of RCS. The kernel is composed of a bunch
of modules (the file Modules lists them). A copy of each module is 
checked out in this directory.  Originally there were symbolic links
throughout the sources for machine-dependent files that were shared
by more than one machine type. You can't have symbolic links on CD-ROM,
so they have been replaced by the file they pointed to. If you can
get scvs, cvs, and rcs working on your system and you point them at
the Cvsroot, you should be able to check out a copy of each module
with the symbolic links intact (scvs will create them). 

The sources include routines for the sun3, sun4, sun4c (SparcStation),
ds3100 (DecStation 3100), ds5000 (DecStation 5000/200) and SPUR 
(RISC multiprocessor done at Berkeley). The sun3, ds3100, and spur
are no longer supported but we kept the code for reference.

The kernel sources are not complete. We borrowed some floating-point
support routines from Ultrix and SunOS which we can't distribute 
for obvious reasons.  The kernel also needs to include several Unix
header files which we can't distribute.  Here is a list of the
files that are missing from the kernel sources. 

--------------- Missing files -----------------------------------

mach/ds5000.md/ultrixSignal.h :	Signal.h from Ultrix.

mach/ds5000.md/sysinfo.h : Sysinfo.h from Ultrix.

mach/sun3.md/machEeprom.h : A hacked up version of mon/eeprom.h for the sun3.
	You'll need to replace EEC with MACH_SLOT, plus a few other changes.

mach/sun4.md/addsub.c
mach/sun4.md/compare.c
mach/sun4.md/div.c
mach/sun4.md/fpu_simulator.c
mach/sun4.md/fpu_simulator.h
mach/sun4.md/globals.h
mach/sun4.md/ieee.h
mach/sun4.md/iu_simulator.c
mach/sun4.md/mul.c
mach/sun4.md/pack.c
mach/sun4.md/unpack.c
mach/sun4.md/utility.c
mach/sun4.md/uword.c
mach/sun4.md/sys/ieeefp.h
	Bunch of floating-point support routines for the sun4. 

mach/ds5000.md/softfp.o
	Floating point support for the ds5000.

------------------------------------------------------------------

The "sprite" directory is used to build kernels. 

If you don't want floating point you can build a ds5000 kernel without
it by defining NO_FLOATING_POINT in the mach module. Any program that
does fancy floating point operations will get an illegal instruction
exception.

