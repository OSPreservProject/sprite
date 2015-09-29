#!/sprite/cmds/csh -f
#
# A script to generate (or regenerate) a source (cmds) directory  Makefile
# from a prototype Makefile.  If ./Makefile.proto exists, use it, else
# use a common prototype.
#
# We assume we were invoked from mkmf, thus we don't need to alter the
# path, and MKMFDIR is in the environment to tell us where to find prototype
# makefiles, etc.
#
# Parameters passed in from mkmf as environment variables:
#
#	DOMACHINES	names of machines we are supposed to run mkmf on
#	MKMFDIR		directory containing prototype makefiles
#	MKMFFLAGS	arguments to all mkmfs run recursively
#	MACHINES	list of machine names (e.g. "sun2 sun3"), for
#			which there are machine-dependent subdirectories
#			(sun2.md, sun3.md) to hold the object files and
#			any machine-specific source files to use when
#			compiling for that machine
#	MAKEFILE	name of makefile to create
#	SUBTYPE		information about what type of command this is:
#			used to figure out where to install things.
#
# Several of these environment variables must be copied to local shell
# variables before use, because shell variables can be used in some places
# where environment variables can't.

#
# Argument processing.  (Generalized form, even though just one flag so far.)
#
while ($#argv >= 1)
    if ("$1" == '-x') then
	set echo
    endif
    shift
end

set nonomatch
set srcs =(*.{h,c,s,l,y,cc} *.md/*.{h,c,s,l,y,cc,o})
set mds = (*.md)
set manPages = (*.man)
if ("$mds" == "*.md") then
    set mds = ()
endif
if ("$manPages" == "*.man") then
    set manPages = ()
endif
#
# Check to see if there were any sources.  The first check (size == 2, the
# number of strings that would be there if there were no matches)
# is only necessary because the second check will cause an error if
# srcs contains more than 1024 bytes.  If no sources, then assume that
# this directory contains only a shell script (and eliminate any
# machine-dependent subdirectories that Pmake might have created).
#
if ($#srcs == 2) then
    if ("$srcs" == "*.{h,c,s,l,y,cc} *.md/*.{h,c,s,l,y,cc,o}") unset srcs
endif
unset nonomatch
if (! $?srcs) then
    echo "No sources, assuming shell script."
    if ("$mds" != "") then
	echo "Deleting extraneous subdirectories $mds."
	rmdir $mds
	if ($status != 0) then
		echo "Error removing directories: not empty."
		exit 1
	endif
    endif
    $MKMFDIR/mkmf.script $*
    exit $status
endif

set subtype=$SUBTYPE
set prog=$cwd:t
set machines=($MACHINES)
set domachines = ($DOMACHINES)
set makefile=($MAKEFILE)
set distdir=($DISTDIR)

if (-e $makefile.proto) then
	set proto=$makefile.proto
else
	set proto="${MKMFDIR}/Makefile.command"
endif

echo "Generating $makefile for $prog using $proto"


cat $proto | sed \
	-e "s,@(DATE),`date`,g" \
	-e "s,@(MACHINES),$machines,g" \
	-e "s,@(MAKEFILE),$makefile,g" \
	-e "s,@(MANPAGES),$manPages,g" \
	-e "s,@(NAME),$prog,g" \
	-e "s,@(TEMPLATE),$proto,g" \
	-e "s,@(TYPE),$subtype,g" \
	-e "s,@(DISTDIR),$distdir,g" \
	> $makefile

setenv PARENTDIR $cwd
foreach i ($domachines)
	(cd $i.md; mkmf $MKMFFLAGS -f md.mk)
end
