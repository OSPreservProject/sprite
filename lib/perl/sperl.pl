#
# sperl.pl
#
# Perl library for sosp tracing.  Use with "sperl".  Sperl has the following
# built-in routines:
#
# Routines for manipulating trace files:
#
# $FD = &TraceOpenFile($filename)
#	
#	$filename is the name of the file to open.  $FD is a handle used
#	internally by sperl and is passed as an argument to other 
#	routines.  Don't screw up and pass something else, otherwise
#	sperl will likely dump core.  It's ok to open lots of trace files,
#	though.  $FD is set to undefined if the file isn't opened.
#
# $hdr = &TraceReadHeader($FD)
#
#	Reads a header from the file.  The header is returned as a string,
#	but it is byte-swapped into the correct order for your machine.
#	$hdr is set to undefined if there is an error.
#
#
# @fields = &TraceParseHeader($hdr)
#
#	Takes a header from TraceReadHeader and splits it up into an array.
#	You could also use "unpack" to do this, but this routine is faster.
#	$fields[0] is set to undefined if there is an error.
#
# $record = &TraceReadRecord($FD, [$event])
#
#	Just like TraceReadHeader but reads a record instead. If you add
#	a second argument "event", it will be set to the event of the
#	so you can avoid parsing records for events you don't care about.
#
# @fields = &TraceParseRecord($record)
#
#	Same as TraceParseHeader but parses a record instead.
#
# &TraceCloseFile($FD)
#
#	Closes a trace file.
#
# Misc routines:
#
# ($seconds, $microseconds) = &TraceGetTime($bootSecs, $bootUsecs, 
#					$diffSecs, $diffUsecs)
#
# 	Takes the boot time and adds it to the time in the record to get the
# 	absolute time that the event occurred.
#
# $string = &TraceTimeString($seconds, $microseconds)
#
#	Takes a time of the form found in the trace files and produces
#	a string you can print.

#
# Name of events.
#
$TypeNames[1] = "OPEN";
$TypeNames[2] = "DELETE";
$TypeNames[3] = "CREATE";
$TypeNames[4] = "MKLINK";
$TypeNames[5] = "SET_ATTR";
$TypeNames[6] = "GET_ATTR";
$TypeNames[7] = "LSEEK";
$TypeNames[8] = "CLOSE";
$TypeNames[9] = "MIGRATE";
$TypeNames[10] = "TRUNCATE";
$TypeNames[11] = "CONSIST_CHANGE";
$TypeNames[12] = "READ";
$TypeNames[13] = "LOOKUP";
$TypeNames[14] = "CONSIST_ACTION";
$TypeNames[15] = "PREFIX";
$TypeNames[16] = "LOOKUP_OP";
$TypeNames[17] = "DELETE_DESC";

#
# @fields = &TraceGetHeader($FD)
#
#	Reads and parses a header all in one shot.
#
sub TraceGetHeader {
    if ($#_ != 0) {
	printf("Usage: &TraceGetHeader($FD)\n");
	return ();
    } else {
	local($hdr) = &TraceReadHeader($_[0]);
	if (!defined($hdr)) {
	    return ();
	}
	&TraceParseHeader($hdr);
    }
}

#
# @fields = &TraceGetRecord($FD)
#
#	Reads and parses a record all in one shot.
#
sub TraceGetRecord {
    if ($#_ != 0) {
	printf("Usage: &TraceGetRecord($FD)\n");
	return ();
    } else {
	local($record) = &TraceReadRecord($_[0]);
	if (!defined($record)) {
	    return ();
	}
	&TraceParseRecord($record);
    }
}

#
# @time = &TraceSubtractTime(@previous, @current)
#
#   Each array contains two element: 0 is the seconds, 1 is the microseconds.
#   Returns the current time minus the previous time.
#
sub TraceSubtraceTime {
    local(@result);
    $result[0] = $_[2] - $_[0];
    $result[1] = $_[3] - $_[1];
    if ($result[1] < 0) {
	$result[0]--;
	$result[1] += 1000000;
    }
    @result;
}

#
# $string = &TraceTimeString2(@time)
#
#	The original TraceTimeString should be called "TraceDateString"
#	since it interprets the time as universal time.  This routine
#	prints out the time in terms of days, hours, minutes, seconds
# 	and microseconds.
#
sub TraceTimeString2 {
    local($string);
    local($seconds) = $_[0];
    local($days) = 0;
    local($hours) = 0;
    local($minutes) = 0;
    printf("seconds = %d\n", $seconds);
    if ($seconds > 86400) {
	$days = $seconds / 86400;
	printf("%d\n", $days * 86400);
	$seconds -= int($days) * 86400;
    }
    printf("seconds = %d\n", $seconds);
    if ($seconds > 3600) {
	$hours = $seconds / 3600;
	$seconds -= int($hours) * 3600;
    }
    printf("seconds = %d\n", $seconds);
    if ($seconds > 60) { 
	$minutes = $seconds / 60;
	$seconds -= int($minutes) * 60;
    }
    printf("seconds = %d\n", $seconds);
    $string = sprintf("%2d+%02d:%02d:%02d.%3.3d\n", $days, $hours,
		    $minutes, $seconds, $_[1] / 1000);
    return $string;
}


