#@package: experimental_debugger trace_step bp bc tp
#
# ndebug.tcl
#

# trace_step is the Tcl procedure called when tracing is done with
# "traceproc"
#
# This is one possible implementation.  We can "step in" and "continue"
# by playing games with the depth, as demonstrated here.
#
# it could do much more, like get and eval input lines, poke into
# variables, and so forth.
#
#
# future
#
# g, print globals
# l, print local vars
#
# have a way to show the argv
# have a way to toggle between command and argv
# 

proc trace_step {depth command argv} {

    # fix up depth (if condition is true, we were stepping into a procedure
    # and we now need to decrease the depth so we won't step into the
    # next procedure unless commanded to do so
    if {$depth < [tracecon depth]} {
	tracecon depth $depth
    }

    echo [replicate "  " [expr {$depth - [tracecon depthfloor]}]]$command

    while {1} {
        puts stdout "nsca!? " nonewline
    
        set line [gets stdin]

        set command [string index $line 0]

        if {$command == "" || $command == "n"} {
	    return
        }

        if {$command == "s"} {
	    tracecon depth [expr {[tracecon depth] + 1}]
	    return
        }

        if {$command == "c"} {
	    tracecon depth [expr {[tracecon depth] - 1}]
	    return
        }

        if {$command == "!"} {
	    if {[string length $line] <= 1} {
	        echo "Now in command loop at same level as proc being debugged."
	        echo "Enter Control-D to return to the debugger."
	        uplevel 1 {commandloop {return "debug> "} {return "debug=> "}}
	    } else {
		uplevel 1 [string range $line 1 end]
	    }
	    continue
        }

        if {$command == "a"} {
	    echo $argv
	    continue
        }


	if {$command == "?"} {
	    echo "a    show the command as it will execute (subordinate expressions evaluated)"
	    echo "c    continuous, execute remainder of proc continuously"
	    echo "n    next, execute next statement at this depth"
	    echo "s    step in, step into next procedure"
	    echo "!    push to an interactive command loop"
	    echo "!command   execute tcl command"
	    continue
	}
	echo "unrecognized command"
    }
}

#
# exec_breakpoint
#
# support routine for breakpoints.  We do it by renaming the procedure
# being breakpointed, then create a procedure that calls exec_breakpoint,
# which calls traceproc on the breakpointed procedure.  uplevel magic
# is used to get the variable context from the correct level.
#
proc exec_breakpoint {procedure argv} {
    echo "breakpoint in $procedure"
    uplevel 2 "traceproc ${procedure}_bp $argv"
}

#
# bp - breakpoint, turn on breakpoints for one or more named
# procedures, or list procedures with breakpoints defined
# if no procnames are specified
#
proc bp {args} {

    foreach procedure $args {
        if {[info procs $procedure] == ""} {
	    error "$procedure: no such procedure"
        }
        rename $procedure ${procedure}_bp

        proc $procedure {args} "exec_breakpoint $procedure \$args"
    }

    if {$args == ""} {
	echo [info procs "*_bp"]
    }
}

#
# bc - breakpoint clear, turn off breakpoints for one or more named
# procedures, or all if none are specified
#
#
proc bc {args} {
    if {$args == ""} {
	foreach procedure [info procs "*_bp"] {
	    set oldName [string range $procedure 0 [expr {[string length $procedure] - 4}]]
	    rename $oldName ""
	    rename $procedure $oldName
	}
	echo "all breakpoints cleared"
	return
    }

    foreach procedure $args {
        if {[info procs ${procedure}_bp] == ""} continue
        rename $procedure ""
        rename ${procedure}_bp $procedure
    }
}

#
# a convenient shorthand for traceproc
#
proc tp {procName} {uplevel "traceproc $procName"}
