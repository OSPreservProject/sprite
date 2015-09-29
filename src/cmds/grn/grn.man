.TH GRN 1 local
.SH NAME
grn \- ditroff preprocessor for gremlin files
.SH SYNOPSIS
.BR grn
[ options ] ... [
.I file
] ...
.SH DESCRIPTION
.I Grn
is a preprocessor for including
.I gremlin
pictures in
.I ditroff
input.
.I Grn
writes to standard output, processing only input lines between
two that start with `.GS' and `.GE'.  Those lines must contain
.I grn
commands (see below).  These commands request a gremlin file, and the
picture in that file is converted and placed in the ditroff input stream.
The .GS may be followed by a C, L, or R to center, left or right justify
the whole gremlin picture (default justification is center).  If no
.I file
is mentioned, the standard input is read.
At the end of the picture, the position on the page is the bottom
of the gremlin picture.  If the grn entry is ended
with `.GF' instead of `.GE', the position is left at the top
of the picture.  The following command-line options are understood:
.TP
.BI \-P dev
Prepare output for printer
.IR dev .
The default is varian (or versatec since the two are equivalent).
See ditroff(1) for acceptable dev's.
.TP
.BI \-L dir
Set the library to directory
.IR dir .
If a gremlin file cannot be found in the current directory,
.I dir
is prepended to the filename for a second try.  The default
library directory is /usr/local/gremlib.
.SH GRN COMMANDS
Each input line between the `.GS' and `.GE' may have one grn command.
Commands consist of one or two strings separated by white space, the first
string being the command and the second its operand.  Commands may
be upper or lower case and abbreviated down to one character.
.PP
Commands that affect a picture's environment (those listed before
.BR default ,
below) are only in effect for the current picture:
the environment is reinitialized to the defaults at the start of the
next picture.  The commands are as follows:
.TP
.BI 1 " N"
(\fB2\fR,
.BR 3 ,
or
.BR 4 )
\0Set gremlin's text size number 1 (2, 3, or 4) to
.I N
points.  The default is 12 (16, 24, and 36).
.TP
.BI roman " ff"
(\fBitalics\fR,
.BR bold ,
or
.BR special )
\0Set the roman (italics, bold, or special) font to ditroff's font
.I ff
(either a name or number).  The default is R (I, B, and S).
.TP
.BI l " N"
Set the stipple font to ditroff's stipple font
.I ff
(name or number).  The command
.B stipple
can be substituted, but only abbreviated down as far as `st' to avoid
confusion with special.  There is NO default for stipples (unless one is
set by the default command), and it is illegal to include a gremlin
picture with polygons without specifying a stipple font.
.TP
.BI x " N"
Magnify the picture (in addition to any default magnification) by
.IR N ,
a floating point number > 0.  The command
.B scale
may be substituted, abbreviated down to `sc'.
.TP
.BI narrow " N"
(\fBmedium\fR,
or
.BR thick )
\0Set the thickness of gremlin's narrow (medium or thick) lines to
.IR N .
The default is 1 (3 and 5) pixels.
.TP
.BI pointscale " <off/on>"
Scale text to match the picture.  Gremlin text is usually printed in
the point size specified in 1, 2, 3 or 4 regardless
of any scaling factors in the picture.  Setting
.B pointscale
will cause the point sizes to scale with the picture (within
ditroff's limitations, of course).  An operand of anything but
.I off
will turn text scaling on.
.TP
.B default
Reset the picture environment defaults
to the settings in the current picture.  This is meant to be
used as a global parameter setting mechanism at the beginning of
the ditroff input file, but can be used at any time to reset the
default settings.
.TP
.BI width " N"
Forces the picture to be
.I N
inches wide.  This overrides any scaling factors
present in the same picture.
.BI width " 0"
is ignored.
.TP
.BI height " N"
Forces picture to be
.I N
inches high, overriding other scaling factors.
If both ``width'' and ``height'' are specified the tighter constraint
will determine the scale of the picture.
.B Height
and
.B width
commands are not saved with a
.B default
command.  They will, however, affect point size scaling if that
option is set.
.TP
.BI file " name"
Get picture from gremlin file
.I name
from the current directory (or in the library directory see the
.B \-L
option above).  If two
.B file
commands are given,
the second one overrides the first.  If
.I name
doesn't exist, an error message is reported and processing
continues from the .GE line.
.SH NOTES ABOUT DITROFF
Since
.I grn
is a preprocessor, it doesn't know about current indents, point sizes,
margins, number registers, etc.  Consequently, no troff input can be
placed between the ``.GS'' and ``.GE''.  However, gremlin text is now
processed by troff, so anything legal in a single line of troff input
is legal in a line of gremlin text (barring ``.'' directives at
the beginning of a line).
.PP
When using grn along with other preprocessors, it is best
to run tbl before grn, pic, and/or ideal to avoid overworking
tbl.  Eqn should always be run last.
.PP
A picture is considered an entity, but that doesn't stop troff from
trying to break it up if it falls off the end of a page.  Placing
the picture between ``keeps'' in either the \-ms or \-me macros
will ensure proper placement.
.PP
The .GS and .GE lines are sent through to ditroff and used in the \-ms
and \-me macros for adjusting the picture.  There are \-mg macros
available for grn, pic and ideal if you don't use \-ms or \-me.  Grn
uses number registers g1 through g9 and sets register g1 and g2
to the width and height of the gremlin figure (in device units)
before entering the .GS (this is for those who want to rewrite
these macros).
.SH FILES
/usr/lib/font/dev\f2xxx\fP/DESC.out	device \f2xxx\fP description file
.SH SEE ALSO
gremlin(l), ditroff(1), pic(1), ideal(1)
.SH AUTHOR
David Slattengren
