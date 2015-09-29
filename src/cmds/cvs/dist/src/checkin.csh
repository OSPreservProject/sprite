#!/bin/csh
#
# $Id: checkin.csh,v 1.8.1.1 91/01/18 12:06:34 berliner Exp $
#
#   Copyright (c) 1989, Brian Berliner
#
#   You may distribute under the terms of the GNU General Public License
#   as specified in the README file that comes with the CVS 1.0 kit.
#
#############################################################################
#									    #
# This script is used to check in sources from vendors.  		    #
#									    #
#	Usage: checkin repository Vendor_Tag Vendor_Release_Tag		    #
#									    #
# The repository is the directory where the sources should		    #
# be deposited, the Vendor_Tag is the symbolic tag for the 		    #
# vendor branch of the RCS release tree, and the Vendor_Release_Tag	    #
# is the symbolic tag for this release.  				    #
#									    #
# checkin traverses the current directory, ensuring that an 		    #
# identical directory structure exists in the repository directory.  It	    #
# then checks the files in in the following manner:			    #
#									    #
#		1) If the file doesn't yet exist, check it in 		    #
#			as revision 1.1					    #
#		2) Tag branch 1.1.1 with the vendor tag			    #
#		3) Check the file into the vendor branch, 		    #
#			labeling it with the Vendor_Release_Tag		    #
#		4) If the file didn't previously exist, 		    #
#			make the vendor branch the default branch	    #
#									    #
# The script also is somewhat verbose in letting the user know what is	    #
# going on.  It prints a diagnostic when it creates a new file, or updates  #
# a file that has been modified on the trunk.		   		    #
#									    #
#############################################################################

set vbose = 0
set message = ""
set cvsbin = /usr/local/bin
set rcsbin = /usr/local/bin
set grep = /bin/grep
set message_file = /tmp/checkin.$$
set got_one = 0

if ( $#argv < 3 ) then
    echo "Usage: checkin [-v] [-m message] [-f message_file] repository"
    echo "	Vendor_Tag Vendor_Release_Tag [Vendor_Release_tag...]"
    exit 1
endif
while ( $#argv )
    switch ( $argv[1] )
        case -v:
            set vbose = 1
	    breaksw
	case -m:
	    shift
	    echo $argv[1] > $message_file
	    set got_one = 1
	    breaksw
	case -f:
	    shift
	    set message_file = $argv[1]
	    set got_one = 2
	    breaksw
	default:
	    break
    endsw
    shift
end
if ( $#argv < 3 ) then
    echo "Usage: checkin [-v] [-m message] [-f message_file] repository"
    echo "	Vendor_Tag Vendor_Release_Tag [Vendor_Release_tag...]"
    exit 1
endif
set repository = $argv[1]
shift
set vendor = $argv[1]
shift
set release = $argv[1]
shift
set extra_release = ( $argv )

if ( ! $?CVSROOT ) then
    echo "Please set the environmental variable CVSROOT to the root"
    echo "	of the tree you wish to update"
    exit 1
endif

if ( $got_one == 0 ) then
    echo "Please Edit this file to contain the RCS log information" >$message_file
    echo "to be associated with this file (please remove these lines)">>$message_file
    if ( $?EDITOR ) then
	$EDITOR $message_file > /dev/tty
    else
	/usr/ucb/vi $message_file > /dev/tty
    endif
    set got_one = 1
endif

umask 2

set update_dir = ${CVSROOT}/${repository}
if ( -d SCCS ) then
    sccs get SCCS/* >& /dev/null
endif
if ( -d RCS ) then
    $rcsbin/co RCS/* >& /dev/null
endif
foreach name ( * .[a-zA-Z0-9]* )
    if ( $name == SCCS ) then 
	continue
    endif
    if ( $name == RCS ) then 
	continue
    endif
    if ( $vbose ) then 
	echo "Updating ${repository}/${name}"
    endif
    if ( -d $name ) then
	if ( ! -d ${update_dir}/${name} ) then
	    echo "WARNING: Creating new directory ${repository}/${name}"
	    mkdir ${update_dir}/${name}
	    if ( $status ) then
		echo "ERROR: mkdir failed - aborting"
		exit 1
	    endif
	endif
	chdir $name
	if ( $status ) then
	    echo "ERROR: Couldn\'t chdir to $name - aborting" 
	    exit 1
	endif
	if ( $vbose ) then
	    $cvsbin/checkin -v -f $message_file ${repository}/${name} $vendor $release $extra_release
	else
	    $cvsbin/checkin -f $message_file ${repository}/${name} $vendor $release $extra_release
	endif
	if ( $status ) then 
	    exit 1
	endif
	chdir ..
    else
	if ( ! -f $name ) then
	    echo "WARNING: $name is neither a regular file" 
	    echo "	   nor a directory - ignored"
	    continue
	endif
	set file = ${update_dir}/${name},v
	set new = 0
	set comment = ""
	grep -s '\$Log.*\$' ${name}
	if ( $status == 0 ) then
	    set myext = ${name:e}
	    set knownext = 0
	    foreach xx ( "c" "csh" "e" "f" "h" "l" "mac" "me" "mm" "ms" "p" "r" "red" "s" "sh" "sl" "cl" "ml" "el" "tex" "y" "ye" "yr" "" )
		if ( "${myext}" == "${xx}" ) then
		    set knownext = 1
		    break
		endif
	    end
	    if ( $knownext == 0 ) then
		echo For file ${file}:
		grep '\$Log.*\$' ${name}
		echo -n "Please insert a comment leader for file ${name} > "
		set comment = $<
	    endif
	endif
	if ( ! -f $file ) then
	    if ( ! -f ${update_dir}/Attic/${name},v ) then
	        echo "WARNING: Creating new file ${repository}/${name}"
		if ( "${comment}" != "" ) then
		    $rcsbin/rcs -q -i -c"${comment}" -t/dev/null $file
		endif
	        $rcsbin/ci -q -u1.1 -t/dev/null $file 
	        if ( $status ) then
		    echo "ERROR: Initial check-in of $file failed - aborting"
		    exit 1
	        endif
	        set new = 1
	    else 
		set file = ${update_dir}/Attic/${name},v
		echo "WARNING: Updating ${repository}/Attic/${name}"
	        set headbranch = `sed -n '/^head/p; /^branch/p; 2q' $file`
	        if ( $#headbranch != 2 && $#headbranch != 4 ) then
		    echo "ERROR: corrupted RCS file $file - aborting"
	        endif
		set head = "$headbranch[2]"
		set branch = ""
		if ( $#headbranch == 4 ) then
		    set branch = "$headbranch[4]"
		endif
	        if ( "$head" == "1.1;" && "$branch" != "1.1.1;" ) then
		    ${rcsbin}/rcsdiff -q -r1.1 $file > /dev/null
		    if ( ! $status ) then
		        set new = 1
		    endif
	        else
	            if ( "$branch" != "1.1.1;" ) then
		        echo -n "WARNING: Updating locally modified file "
			echo    "${repository}/Attic/${name}"
	            endif
	        endif
	    endif
	else
	    set headbranch = `sed -n '/^head/p; /^branch/p; 2q' $file`
	    if ( $#headbranch != 2 && $#headbranch != 4 ) then
		echo "ERROR: corrupted RCS file $file - aborting"
	    endif
	    set head = "$headbranch[2]"
	    set branch = ""
	    if ( $#headbranch == 4 ) then
		set branch = "$headbranch[4]"
	    endif
	    if ( "$head" == "1.1;" && "$branch" != "1.1.1;" ) then
		${rcsbin}/rcsdiff -q -r1.1 $file > /dev/null
		if ( ! $status ) then
		    set new = 1
		endif
	    else
	        if ( "$branch" != "1.1.1;" ) then
		    echo -n "WARNING: Updating locally modified file "
		    echo    "${repository}/${name}"
	        endif
	    endif
	endif
	$rcsbin/rcs -q -N${vendor}:1.1.1 $file
	if ( $status ) then
	    echo "ERROR: Attempt to set Vendor_Tag in $file failed - aborting"
	    exit 1
	endif
	set lock_failed = 0
	$rcsbin/rcs -q -l${vendor} $file >& /dev/null
	if ( $status ) then
	    set lock_failed = 1
	endif
	if ( "${comment}" != "" ) then
	    $rcsbin/rcs -q -c"${comment}" $file
	endif
	$rcsbin/ci -q -f -u${vendor} -N${release} $file < $message_file 
	if ( $status ) then
	    echo "ERROR: Check-in of $file failed - aborting"
	    if ( ! $lock_failed ) then
	        $rcsbin/rcs -q -u${vendor} $file
	    endif
	    exit 1
	endif
	foreach tag ( $extra_release )
	    $rcsbin/rcs -q -N${tag}:${release} $file
	    if ( $status ) then
		echo "ERROR: Couldn't add tag $tag to file $file"
		continue
	    endif
	end
	if ( $new ) then
	    $rcsbin/rcs -q -b${vendor} $file
	    if ( $status ) then
		echo "ERROR: Attempt to change default branch failed - aborting"
		exit 1
	    endif
	endif
    endif
end
if ( $got_one == 1 ) rm $message_file
