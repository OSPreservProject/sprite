#!/sprite/cmds/perl 
# option.pl
# Implements the Sprite "Opt" package in Perl.  See the "Opt" man page.
# Example:
#
#	$t = 0;
#	$f = 1;
#	$s = "hello";
#	@bar = ('t', $OPT_TRUE, *t, 'true',
#		'f', $OPT_FALSE, *f, 'false');
#	&Opt_Parse(*ARGV, *bar, 0);
#	printf("$t $f\n");
#


$OPT_ALLOW_CLUSTERING 	= 0x1;
$OPT_OPTIONS_FIRST	= 0x2;
$OPT_NO_SPACE		= 0x4;

$OPT_NIL = " ";

$OPT_FALSE = 0;
$OPT_TRUE = 1;
$OPT_INT = 2;
$OPT_FLOAT = 3;
$OPT_STRING = 4;
$OPT_DOC = 5;
$OPT_REST = 6;
$OPT_FUNC = 7;
$OPT_GENFUNC = 8;


sub Opt_PrintUsage {
    while($name = shift) {
	$type = shift;
	local(*ptr) = shift;
	$doc = shift;
	if ($type != $OPT_DOC) {
	    printf(" -$name:\t$doc\n");
	    if (($type == $OPT_INT) || ($type == $OPT_FLOAT) || 
		($type == $OPT_STRING)) {
		printf("\t\tDefault value: \"$ptr\"\n");
	    }
	} else {
	    printf("$doc\n");
	}

    }
}

sub PrintUsage {
    local($option) = shift;
    local(*argv) = shift;
    &Opt_PrintUsage(@optHelp);
    @argv = ();
    exit(0);
}

sub Opt_Parse {
    local(*argv, *optArray, $flags) = @_;
#    local(*optArray) = @_;
#    local($flags) = pop(@optArray);
    local(%options, $name, %ptrs, $doeval, $type, $doc, @newargv, $rest);
    local(@targv) = *argv;

    @optHelp = @optArray;

    $types{"-help"} = $OPT_GENFUNC;
    $ptrs{"-help"} = *PrintUsage;
    $doc{"-help"} = "Print this message";
    $types{"-?"} = $OPT_GENFUNC;
    $ptrs{"-?"} = *PrintUsage;
    $doc{"-?"} = "Print this message";
    $reg{"-\\?"} = 1;
    while($name = shift(@optArray)) {
	$types{"-$name"} = shift(@optArray);
	$ptrs{"-$name"} = shift(@optArray);
	$doc{"-$name"} = shift(@optArray);
	$reg{"-$name"} = 1;
    }
    while($name = shift(@argv)) {
	$rest = "";
	$start = $name;
	if ($name !~ /-.*/) {
	    push(@newargv, $name);
	    if ($flags & $OPT_OPTIONS_FIRST) {
		push(@newargv, @argv);
		last;
	    } else {
		next;
	    }
	}
	while(1) {
	    if (defined($types{"$name"})) {
		local(*ptr) = $ptrs{"$name"};
		$doeval = 1;
		$type = $types{"$name"};
		if ($type == $OPT_TRUE) {
		    $value = 1;
		} elsif ($type == $OPT_FALSE) {
		    $value = 0;
		} elsif (($type == $OPT_INT) || ($type == $OPT_FLOAT) || 
		    ($type == $OPT_STRING)) {
		    $value = shift(@argv) || die("$name needs argument\n");
		} elsif ($type == $OPT_FUNC) {
		    if(&ptr($name, $argv[0])) {
			shift(@argv);
		    }
		    $doeval = 0;
		} elsif ($type == $OPT_GENFUNC) {
		    &ptr($name, *argv);
		    $doeval = 0;
		}
		if ($doeval == 1) {
		    $ptr = $value;
		}
		if (($flags & $OPT_ALLOW_CLUSTERING) && ($rest ne "")) {
		    $name = "-" . $rest;
		    $rest = "";
		} else {
		    last;
		}
	    } elsif ($flags & $OPT_ALLOW_CLUSTERING) {
		if (length($name) > 2) {
		    $rest = chop($name) . $rest;
		} elsif ($rest eq "") {
		    push(@newargv, $start);
		    last;
		} else {
		    $name = "-" . $rest;
		}
	    } elsif ($flags & $OPT_NO_SPACE) {
		foreach $i (keys(%reg)) {
		    if ($name =~ /$i(.*)/) {
			unshift(argv, $1);
			$name = $i;
		    }
		}
	    } else {
		push(@newargv, $name);
		last;
	    }
	}
    }
    @argv = @newargv;
}

