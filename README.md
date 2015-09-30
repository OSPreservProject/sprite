# The Sprite Operating System

This respository includes the source code, executables, documentation
and research papers for the Sprite network operating system.  Sprite
is a distributed systems research project led by Prof. John Ousterhout
in the Computer Science Department of the University of California
at Berkeley.  Besides Prof. Ousterhout, the research team has
included Brent Welch, Andrew Cherenson, Fred Douglis, Mike Nelson,
Adam de Boor, Mendel Rosenblum, Mary Baker, Bob Bruce, John Hartman,
Ken Shirriff, Mike Kupfer, Jim Mott-Smith, Geoff Voelker, and Matt
Secor.  Many others have also contributed to the system.

Nothing on this repository is guaranteed to work.  This collection of
source, executables, documentation and papers was put together in
a great hurry.  While we were able to test some aspects, much was
left untested.  Make what use of it you can, but we will not be
available to fix problems.

Right now sprite only supports two venerable architectures, the sun4
series (but not the sun4m, necessary for qemu support) and the DECstation
(supported by gxemul). Initial attempts to get Sprite running in emulation
should probably revolve around booting the ds5000.bt image in gxemul.

The correct invocation to boot the DECstation image in gexemul is:

> gxemul -X -e 3max -M128 -d ds5000.bt -j vmsprite -o ''

---

This repository contains the following files and directories:

<<<<<<< HEAD
file    | directory
----    | ---------
=======
>>>>>>> fee8cbbe0803ab2d8d855b0e2d0b1ca0f1c74fd1
admin	|          directory for administrative programs, scripts and information
boot.txt	|       file with information about booting the Sprite kernel images
docs	|           directory for documentation
ds5000.bt	|      boot image for Sprite on a  DECstation 5000/200
sun4.bt	|        boot image for Sprite on a SparcStation 2
lib	|            directory for library files other than source files
filename.lst    A list of all the files on the disc
ls_lR	|          output from ls -lR, a recursive list of the files on the disc
man	|            directory for manual pages
papers	|         directory for research papers about Sprite
readme.txt	|     file that describes this CD-ROM
sprite.txt	|     file with a history of the Sprite project
src	|            directory for source code for kernel, commands, libraries, etc
