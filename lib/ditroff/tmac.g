.de IS  \"	ideal picture
.nr g7 \\n(.u
.ls 1
..
.de IE
.if \\n(g7 .fi
.ls
..
.de IF
.if \\n(g7 .fi
.ls
..
.de PS	\" 	start picture (bwk) $1 is height, $2 is width in units
.if t .sp .3
.in (\\n(.lu-\\$2u)/2u
.ne \\$1u
.nr g7 \\n(.u
.ls 1
..
.de PE
.in
.if \\n(g7 .fi
.if t .sp .6
.ls
..
.de GS	\"	GRN called with C (default), L or R  (g1=width, g2=height)
.nr g7 (\\n(.lu-\\n(g1u)/2u
.if "\\$1"L" .nr g7 \\n(.iu
.if "\\$1"R" .nr g7 \\n(.lu-\\n(g1u
.in \\n(g7u
.nr g7 \\n(.u
.ls 1
.nf
.ne \\n(g2u
..
.de GE
.ls
.in
.if \\n(g7 .fi
.if t .sp .6
..
.de GF
.ls
.in
.if \\n(g7 .fi
..
