#!/sprite/cmds/sh
#
trap "" 1 15
if test -t 2>&1  ; then
	echo "Sending output to 'nohup.out'"
	exec /sprite/cmds/nice -5 $* >>nohup.out 2>&1
else
	exec /sprite/cmds/nice -5 $* 2>&1
fi
