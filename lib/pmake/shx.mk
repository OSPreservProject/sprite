#
# This is a shell specification to have the bourne shell echo the commands
# just before executing them, rather than when it reads them. Useful if you
# want to see how variables are being expanded, etc.
#
.SHELL 	: path=/bin/sh \
	quiet="set -" \
	echo="set -x" \
	filter="+ set - " \
	echoFlag=x \
	errFlag=e \
	hasErrCtl=yes \
	check="set -e" \
	ignore="set +e"
