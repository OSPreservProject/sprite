#!/sprite/cmds/csh -f
#
# A script to generate the Makefile for a subdirectory that contains
# machine-dependent sources and objects.  If ./Makefile.proto exists,
# use it, else use a default prototype.
#
# We assume we were invoked from mkmf.  Parameters passed in from mkmf
# through environment variables:
#
# Parameters passed in from mkmf as environment variables:
#	MKMFDIR		directory containing prototype makefiles
#	MAKEFILE	name of makefile to create
#	PARENTDIR	name of parent directory (.. is fooled by symbolic
#			links)
#			
# Variables generated here:
#	dir		name of this subdirectory
#	machine		name of machine type for this directory (e.g. "sun3")
#	pref		prefix pattern that files must match to be included
#	makefile	name of the makefile to create
#	proto		name of prototype makefile to use to create $makefile
#

#
# Argument processing.  (Generalized form, even though just one flag so far.)
#
while ($#argv >= 1)
    if ("$1" == '-x') then
	set echo
    endif
    shift
end

set machine=$cwd:t
set machine=$machine:r
set pref='[0-9a-z_A-Z]'

if ($?MAKEFILE) then
	set makefile=$MAKEFILE
else
	set makefile=Makefile
endif

if (-e $makefile.proto) then
	set proto=$makefile.proto
else
	set proto="${MKMFDIR}/Makefile.md"
endif

set dir=$cwd:t
set machine=$dir:r
set parent=$cwd:h
set parent=$parent:t
echo "Generating $makefile for $parent/$dir using $proto"

if ($?PARENTDIR) then
	set parentdir=$PARENTDIR
else
	set parentdir=..
endif

#
# First figure out what's here by way of .c, .y, .l, .s, .p, .h
# and .o files.  For sources, look both in this directory and in
# the parent (machine-independent) directory.
# 
# If any one doesn't have any members, it'll contain the original
# pattern (b/c of nonomatch). We want it to be empty, though, so
# we reset it.
#
set nonomatch
set tmp=( ${pref}*.{c,y,l,s,p,cc} )
#
# Check to see if there were any sources.  The first check (size == 1)
# is only necessary because the second check will cause an error if
# allSrcs contains more than 1024 bytes.
#
if ($#tmp == 1) then
    if ("$tmp" == "${pref}*.{c,y,l,s,p,cc}") set tmp=()
endif
set srcs=()
foreach i ($tmp)
    set srcs=($srcs $dir/$i)
end
set tmp=( ${parentdir}/${pref}*.{c,y,l,s,p,cc} )
if ($#tmp == 1) then
    if ("$tmp" == "${parentdir}/${pref}*.{c,y,l,s,p,cc}") set tmp=()
endif

foreach i ($tmp:gt)
    if (! -e $i) then
	set srcs = ($srcs $i)
    endif
end

set tmp=( ${pref}*.h )
if ($#tmp == 1) then
    if ("$tmp" == "${pref}*.h") set tmp=()
endif
set hdrs=()
foreach i ($tmp)
	set hdrs=($hdrs $dir/$i)
end
set tmp=( ${parentdir}/${pref}*.h )
if ($#tmp == 1) then
    if ("$tmp" == "${parentdir}/${pref}*.h") set tmp=()
endif
foreach i ($tmp:gt)
    if (! -e $i) then
	set hdrs = ($hdrs $i)
    endif
end

set tmp=( ${parent}*.h )
if ($#tmp == 1) then
    if ("$tmp" == "${parent}*.h") set tmp=()
endif
set pubHdrs=()
foreach i ($tmp)
	set pubHdrs=($pubHdrs $dir/$i)
end
set pubHdrs=(`echo $pubHdrs | sed -e "s/[^ ]*Int.h//g"`)

#
# Find miscellaneous files needed for builds (Makefiles, etc).
#
set tmp=( md.mk md.mk.sed dependencies.mk )
set instfiles=()
foreach i ($tmp)
    if (-e $i) then
	set instfiles=($instfiles $dir/$i)
    endif
end
set tmp=( Makefile local.mk Makefile.sed Makefile.ex tags TAGS )
foreach i ($tmp)
    if (-e ${parentdir}/$i) then
	set instfiles = ($instfiles $i)
    endif
end


rm -f version.o
set tmp=( ${pref}*.o )
#
# Check to see if there were any object files.  The first check (size == 1)
# is only necessary because the second check will cause an error if
# tmp contains more than 1024 bytes.
#
if ($#tmp == 1) then
    if ("$tmp" == "${pref}*.o") set tmp=()
endif
set objs=()
foreach i ($tmp)
	set objs=($objs $dir/$i)
end
unset nonomatch

#
# Merge in any .o files that can be created from local source files but don't
# exist yet. In addition, figure out which .o files may be safely removed
# during a "make clean" and store them in RmOfiles.
# Also figure out if there are any generated .c files (like from lex and
# yacc) that should be removed.
#
set RmOfiles=""
set RmCfiles=""
if ($#srcs != 0) then
	foreach file ($srcs)
		set file=$file:t
		set file=$file:r.o
		set RmOfiles=($RmOfiles $dir/$file)
		if (! -e $file) set objs=($objs $dir/$file)
	end
	foreach file ($srcs)
		if (("$file:e" == "l") || ("$file:e" == "y")) then
		    set RmCfiles=($RmCfiles $file:r.c)
		endif
	end
endif

set sacredObjs=""
foreach file ($objs)
    set tmp = $file:r
    echo $RmOfiles | grep $file > /dev/null
    if ($status && ($tmp:t != $parentdir:t)) then
	set sacredObjs = ($sacredObjs $file)
	echo "WARNING: file $file does not have a source file"
    endif
end

#
# Use sed to substitute various interesting things into the prototype
# makefile. The code below is a bit tricky because some of the variables
# being substituted in can be very long:  if the substitution is passed
# to sed with "-e", the entire variable must fit in a single shell argument,
# with a limit of 1024 characters.  By generating a separate script file
# for the very long variables, the variables get passed through (to the
# script file) as many arguments, which gets around the length problem.
#

set rmfiles = ($RmOfiles $RmCfiles)
rm -f mkmf.tmp.sed
echo s,"@(SRCS)",$srcs,g > mkmf.tmp.sed
echo s,"@(OBJS)",$objs,g >> mkmf.tmp.sed
echo s,"@(CLEANOBJS)",$rmfiles,g >> mkmf.tmp.sed
echo s,"@(HDRS)",$hdrs,g >> mkmf.tmp.sed
echo s,"@(INSTFILES)",$instfiles,g >> mkmf.tmp.sed
echo s,"@(SACREDOBJS)",$sacredObjs,g >> mkmf.tmp.sed

cat $proto | sed -f mkmf.tmp.sed \
	-e "s,@(PUBHDRS),$pubHdrs,g" \
	-e "s,@(TEMPLATE),$proto,g" \
	-e "s,@(DATE),`date`,g" > $makefile
rm -f mkmf.tmp.sed
