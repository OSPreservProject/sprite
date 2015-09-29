#
# mxedit.command --
#	Definitions for the command entry
#
# Copyright (c) 1992 Xerox Corporation.
# Use and copying of this software and preparation of derivative works based
# upon this software are permitted. Any distribution of this software or
# derivative works must comply with all applicable United States export
# control laws. This software is made available AS IS, and Xerox Corporation
# makes no warranty about the software, its performance or its conformity to
# any specification.

# Imported globals
# mxFeedback - the command entry gets packed before the feedback entry
#	so we import the name of the feedback widget to achieve this

# Exported globals
# focus - ordinarily the focus is shifted back to the main editing
#	window after a command, but this variable lets commands change
#	that default behavior by setting focus to the name of another widget

# File globals
# command - the name of the command entry
# commandState - whether or not the command entry is visible
# commandMenu - the menu that controls the command entry's visibility

# mxCommandEntry --
# An entry widget for entering commands

proc mxCommandEntry { parent { width 20 } { where {bottom fillx expand} } } {
    global command commandState
    if [catch {labeledEntry $parent .command Command: $width $where} msg] {
	mxFeedback "labeledEntry failed $msg"
	return
    } else {
	set self $msg
	mxCommandBindings $self.entry
    }
    # labeldEntry packs us so the command window is visible
    set commandState commandVisible
    set command $self
    return $self
}

# mxCommandFocus
#	Focus on the command window, making it visible if needed

proc mxCommandFocus { } {
    global command commandState
    case $commandState in {
	commandVisible { }
	commandHidden { mxCommandShow }
    }
    focus $command.entry
}

# mxCommandMenuEntry --
#	Set up the menu entry that controls the display of the command window
#	We'll remember the name of the menu so we can adjust the
#	menu entry to reflect the current state

proc mxCommandMenuEntry { menuName } {
    global commandState commandMenu

    set commandMenu ${menuName}

    case $commandState in {
	"commandVisible" {
	    mxMenuAdd $menuName "Hide command window" {mxCommandHide}
	}
	"commandHidden" {
	    mxMenuAdd $menuName "Open command window" {mxCommandShow}
	}
    }
}
# mxCommandHide --
#	Unpack the command window to save screen space

proc mxCommandHide { } {
    global command commandState commandMenu

    set commandState commandHidden
    pack unpack $command
    mxMenuEntryConfigure $commandMenu "Hide command window" \
	    -command { mxCommandShow } -label "Open command window"
}

# mxCommandShow --
#	Pack the command window so it shows up

proc mxCommandShow { } {
    global command commandState commandMenu
    global mxFeedback

    set commandState commandVisible
    pack before $mxFeedback $command {bottom fillx frame s}

    mxMenuEntryConfigure $commandMenu "Open command window" \
	    -command { mxCommandHide } -label "Hide command window"
}

# A flag to control if focus is returned to the editing window after a command
global focus
set focus default

# mxDoCmd --
#	Execute the commands issued to the command entry
#	This is ordinarily bound to hitting <Return> in the command entry

proc mxDoCmd { } {
    global focus
    global command

    set cmd [$command.entry get]
    if {[llength $cmd] != 0} {
	case [lindex $cmd 0] in {
	    {echo feedback mxFeedback} {
		# Echo inputs
		mxFeedback "$cmd"
	    } help {
		# Display help information
		mxopen [info library]/mxedit.tutorial
	    } default {
		# Pass on the command to Tcl
		if [catch {uplevel #0 $cmd} msg] {
		    mxFeedback "Cmd failed: $msg"
		} else {
		    if {[string compare "$msg" ""] != 0} {
			mxFeedback $msg
		    } else {
			mxFeedback "ok"
		    }
		}
	    }
	}
    }
    # Let a command do its own focus if it wants,
    # otherwise return the focus to the editting window
    if {[string compare $focus "default"] == 0} {
	mxeditFocus
    }
    set focus default
}


