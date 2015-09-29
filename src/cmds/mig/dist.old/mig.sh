#!/bin/sh
#
# Mach Operating System
# Copyright (c) 1991,1990 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon
# the rights to redistribute these changes.
#
#
# HISTORY
# $Log:	mig.sh,v $
# Revision 2.6  91/07/31  18:09:45  dbg
# 	Allow both -header and -sheader switches.
# 	Fix copyright.
# 	[91/07/30  17:14:28  dbg]
# 
# Revision 2.5  91/06/25  10:31:42  rpd
# 	Added -sheader.
# 	[91/05/23            rpd]
# 
# Revision 2.4  91/02/05  17:55:08  mrt
# 	Changed to new Mach copyright
# 	[91/02/01  17:54:47  mrt]
# 
# Revision 2.3  90/06/19  23:01:10  rpd
# 	The -i option takes an argument now.
# 	[90/06/03            rpd]
# 
# Revision 2.2  90/06/02  15:05:05  rpd
# 	For BobLand: changed /usr/cs/bin/wh to wh.
# 	[90/06/02            rpd]
# 
# 	Created for new IPC.
# 	[90/03/26  21:12:04  rpd]
# 
# 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
#	Created.
#

CPP=`wh -Lq cpp`
MIGCOM=`wh -Lq migcom`

cppflags=
migflags=
files=

until [ $# -eq 0 ]
do
    case $1 in
	-[qQvVtTrRsS] ) migflags="$migflags $1"; shift;;
	-i	) migflags="$migflags $1 $2"; shift; shift;;
	-user   ) user="$2"; migflags="$migflags $1 $2"; shift; shift;;
	-server ) server="$2"; migflags="$migflags $1 $2"; shift; shift;;
	-header ) header="$2"; migflags="$migflags $1 $2"; shift; shift;;
	-sheader ) sheader="$2"; migflags="$migflags $1 $2"; shift; shift;;
	-iheader ) iheader="$2"; migflags="$migflags $1 $2"; shift; shift;;
	-theader ) theader="$2"; migflags="$migflags $1 $2"; shift; shift;;

	-MD ) sawMD=1; cppflags="$cppflags $1"; shift;;
	-* ) cppflags="$cppflags $1"; shift;;
	* ) files="$files $1"; shift;;
    esac
done

for file in $files
do
    base="`/usr/bin/basename "$file" .defs`"
    $CPP $cppflags "$file" - ${sawMD+"$base"_defs.d~} | $MIGCOM $migflags || exit
    if [ $sawMD ]
    then
	ruser="${user-${base}User.c}"
	rserver="${server-${base}Server.c}"
	rheader="${header-${base}.h}"
	rsheader="${sheader-}"
	riheader="${iheader-}"
	rtheader="${theader-}"

	sed 's;^'"$base"'.o;'"$rheader"' '"$ruser"' '"$rserver"' '"$riheader"' '"$rtheader"' '"$rsheader"';' \
		< "$base"_defs.d~ > "$base"_defs.d
	rm -f "$base"_defs.d~
    fi
done

exit 0
