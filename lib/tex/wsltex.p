PROGRAM Wsltex;
    {
    **********************************************************************
    Copy an 8-bit file  to a 7-bit file,  turning characters with bit  8
    set into 4-character octal escape sequences \nnn.  This is useful in
    analyzing  WordStar  microcomputer   word  processor  files.    Many
    sequences which  can be  recognized as  doing something  useful  are
    turned into LaTeX command sequences.   Comments  below  describe the
    sequences which are recognized and translated.

    Usage:

    @WSLTEX
    Old 8-bit file: filespec
    New 7-bit file: filespec

    [11-Oct-85]
    **********************************************************************
    }
{ ^L }
CONST
    nul = 0;

    bel = 7;

    ht = 9;
    lf = 10;

    ff = 12;
    cr = 13;

    esc = 27;

    maxring = 256; { limit for character ring }

    maxspecial = 100; { limit for warning when special strings (underline,
		      bold, etc) exceed this length }

    unset = MAXINT;

VAR
    wordstar_file,latex_file : PACKED FILE OF integer;
    ring_index : integer;
    ring : PACKED ARRAY [0 .. maxring] OF integer;
    hyphen_break : boolean;
    eol,is_underline,is_subscript,is_superscript,is_bold,
    is_alternate : boolean;
    bi_eol,bi_underline,bi_subscript,bi_superscript,bi_bold,
    bi_alternate : integer;
    bo_eol,bo_underline,bo_subscript,bo_superscript,bo_bold,
    bo_alternate : integer;
    input_byte_offset,output_byte_offset : integer;
    empty_count,last_c,c : integer;
{ ^L }
PROCEDURE Warning(s : PACKED ARRAY [k1 .. k2 : integer] OF char;
		  b1,b2 : integer { output byte range, or unset,unset }
		  );
    VAR       k : integer;
BEGIN
    Writeln(tty); { !!!! temporary for debugging !!!! }
    Writeln(tty); { !!!! temporary for debugging !!!! }
    Writeln(tty,'----------------------------------------------------------');
    Write(tty,'%');

    FOR k := k1 TO k2 DO
	Write(tty,s[k]);
    Writeln(tty);
    if b1 <> unset then
            Writeln(tty,'	output byte range  = ',b1:0,' .. ',b2:0);
    Writeln(tty,'	input byte offset  = ',input_byte_offset);
    Writeln(tty,'	output byte offset = ',output_byte_offset);
    Writeln(tty,'	current context = [');
    FOR k := ring_index TO maxring DO
	Write(tty,Chr(ring[k]));
    FOR k := 0 TO ((maxring + ring_index - 1) MOD maxring) DO
	Write(tty,Chr(ring[k]));
    Writeln(tty);
    Writeln(tty,'	]');
    Writeln(tty,'----------------------------------------------------------');
    Writeln(tty)
END; { Warning }

{ ^L }
PROCEDURE W_byte(c : integer);
BEGIN
    ring[ring_index] := c;
    ring_index := (ring_index + 1) MOD maxring;
    latex_file^ := c;
    eol := (c = lf);
    Put(latex_file);
    output_byte_offset := output_byte_offset + 1;
    IF eol THEN
    BEGIN
	bi_eol := input_byte_offset;
	bo_eol := output_byte_offset
    END
END; { W_byte }

PROCEDURE W_char(c : char);
BEGIN
    W_byte(Ord(c))
END; { W_char }
{ ^L }
PROCEDURE W_octal;
BEGIN
    W_byte(Ord('\'));
    W_byte((wordstar_file^ DIV 64) + Ord('0'));
    W_byte(((wordstar_file^ DIV 8) MOD 8) + Ord('0'));
    W_byte((wordstar_file^ MOD 8) + Ord('0'))
END; { W_octal }
{ ^L }
PROCEDURE W_string(s : PACKED ARRAY [k1 .. k2 : integer] OF char);
    VAR k : integer;
BEGIN
    FOR k := k1 TO k2 DO
	W_byte(Ord(s[k]))
END; { W_string }
{ ^L }
PROCEDURE Begin_alternate;
BEGIN
    IF is_alternate THEN
	Warning(
'BEGIN ALTERNATE CHARACTER SET command encounted inside alternate sequence',
		bo_alternate,output_byte_offset);
    bi_alternate := input_byte_offset;
    bo_alternate := output_byte_offset;
    is_alternate := true;
    W_string('{\alt ')
END; { Begin_alternate }
{ ^L }
PROCEDURE Begin_boldface;
BEGIN
    IF is_bold THEN
	Warning('BEGIN BOLDFACE command encountered inside boldface',
		bo_bold,output_byte_offset);
    bi_bold := input_byte_offset;
    bo_bold := output_byte_offset;
    is_bold := true;
    W_string('{\bf ')
END; { Begin_boldface }
{ ^L }
PROCEDURE End_alternate;
BEGIN
    IF is_alternate THEN
	BEGIN
	    IF (bo_alternate + maxspecial) < output_byte_offset THEN
	        Warning('Long ALTERNATE CHARACTER SET sequence',
			bo_alternate,output_byte_offset)
	END
    ELSE
	Warning(
'END ALTERNATE CHARACTER SET command encountered outside alternate text',
		bo_alternate,output_byte_offset);
    is_alternate := false;
    W_byte(Ord('}'))
END; { End_alternate }
{ ^L }
PROCEDURE End_boldface;
BEGIN
    IF is_bold THEN
	BEGIN
	    IF (bo_bold + maxspecial) < output_byte_offset THEN
	        Warning('Long BOLDFACE sequence',bo_bold,output_byte_offset)
	END
    ELSE
	Warning('END BOLDFACE command encountered outside boldface',
		bo_bold,output_byte_offset);
    is_bold := false;
    W_byte(Ord('}'))
END; { End_boldface }
{ ^L }
PROCEDURE Line_feed;
BEGIN { line_feed }
    IF hyphen_break THEN { discard line break }
        hyphen_break := false
    ELSE
	BEGIN
	    IF bo_eol < output_byte_offset THEN { text on line }
 		W_byte(lf)
	    ELSE { empty line }
		BEGIN
		    IF last_c <> (128 + cr) THEN
			W_byte(lf)
		END
       END
END; { Line_feed }
{ ^L }
PROCEDURE Superscript;
BEGIN
    IF is_superscript THEN { ending old superscript }
	BEGIN
	     IF (bo_superscript + maxspecial) < output_byte_offset THEN
		 Warning('Long SUPERSCRIPT sequence',
			 bo_superscript,output_byte_offset);
	     W_byte(Ord('}'))
	END
    ELSE { beginning new superscript }
	BEGIN
	    bi_superscript := input_byte_offset;
	    bo_superscript := output_byte_offset;
	    W_string('^{')
	END;
    is_superscript := NOT is_superscript
END; { Superscipt }
{ ^L }
PROCEDURE Subscript;
BEGIN
    IF is_subscript THEN { ending old superscript }
	BEGIN
	     IF (bo_subscript + maxspecial) < output_byte_offset THEN
		 Warning('Long SUBSCRIPT sequence',
		    bo_subscript,output_byte_offset);
	     W_byte(Ord('}'))
	     END
    ELSE { beginning new superscript  }
	BEGIN
	    bi_subscript := input_byte_offset;
	    bo_subscript := output_byte_offset;
	    W_string('_{');
	END;
    is_subscript := NOT is_subscript
END; { Subscript }
{ ^L }
PROCEDURE Underline;
BEGIN
    IF is_underline THEN { ending old underline }
	BEGIN
	    IF (bo_underline + maxspecial) < output_byte_offset THEN
		Warning('Long UNDERLINE sequence',
			bo_underline,output_byte_offset);
	    W_byte(Ord('}'))
	END
    ELSE { beginning new underline }
	BEGIN
	    bi_underline := input_byte_offset;
	    bo_underline := output_byte_offset;
	    W_string('{\em ')
	END;
    { underlined text is \emphasized in LaTeX }
    is_underline := NOT is_underline
END; { Underline }
{ ^L }
BEGIN { Main -- WSLTEX }

Rewrite(output,'tty:');
Write('Input WordStar 8-bit file: ');
Reset(wordstar_file,'':@,'/e/b:8');
Write('Output LaTeX 7-bit file:   ');
Rewrite(latex_file,'':@,'/e/b:7');

{ Initializations -- in alphabetical order }
bi_alternate := -1;
bi_bold := -1;
bi_subscript := -1;
bi_superscript := -1;
bi_underline := -1;
bo_alternate := -1;
bo_bold := -1;
bo_subscript := -1;
bo_superscript := -1;
bo_underline := -1;
eol := true;
hyphen_break := false;
input_byte_offset := -1;
is_bold := false;
is_subscript := false;
is_superscript := false;
is_underline := false;
last_c := -1;
output_byte_offset := -1;

for ring_index := 0 to maxring do
    ring[ring_index] := ord(' ');
ring_index := 0;
{
========================================================================
WordStar makes heavy use of the 8-th (high-order) bit of 8-bit ASCII
characters.  In general, it turns on that bit for
-- any character (0..127) at end-of-word
-- any character (0..127) at end-of-line
-- "hard" line break on CTL-M
-- "hard" page break on CTL-L

A discretionary hyphen (which  can be removed on  joining the lines)  at
end-of-line is encoded as "-\215" (i.e. "-<\200|CTL-M>").

A required hyphen falling at end-of-line is encoded as "-\040\215" (i.e.
followed by a space).

Comment lines and WordStar command lines begin with a dot.

"Hard" line breaks are encoded as \215\015 (i.e. "<\200|CTL-M><CTL-J>");
these occur between double-space lines.

"Soft" line breaks are encoded as <CTL-M><CTL-J>; these occur at
paragraph ends.

Centered text is stored centered without any special markings.

Underlined text is bracketed by CTL-S.

Boldface text is bracketed by CTL-E ..text.. CTL-R.

Superscripts are bracketed by CTL-T.

Subscripts are bracketed by CTL-V.

Alternate typewheel strings appear as CTL-Q ..text.. CTL-W.  Some of
these at least are mnemonic (e = epsilon, m = mu, . = centered dot).

CTL-R also appears after all sub/superscript sequences, apparently
functioning as a "return to normal" state.
========================================================================
}

c := wordstar_file^;
WHILE NOT Eof(wordstar_file) DO
BEGIN
    IF c < 32 THEN { ordinary control character }
    BEGIN
	CASE c OF
	    0,1,2,3,4 :      { CTL-@ .. CTL-D }
		W_octal;

	    5 :      { CTL-E }
		Begin_boldface;

	    6,7,8 : { CTL-F .. CTL-H }
		W_octal;

	    9 :     { CTL-I }
		W_byte(ht);

	    10 :    { CTL-J }
		Line_feed;

	    11 :    { CTL-K }
		W_octal;

	    12 :    { CTL-L }
		W_byte(ff);

	    13 :    { CTL-M }
	        IF last_c = Ord('-') THEN
		    hyphen_break := true
	        ELSE IF NOT hyphen_break THEN
	            W_byte(cr);

	    14,15,16 :       { CTL-N .. CTL-P }
		W_octal;

	    17 :     { CTL-Q }
		Begin_alternate;

	    18 :     { CTL-R }
	        IF is_bold THEN
		    End_boldface;
	        { ELSE discard character }

	    19 :     { CTL-S }
		Underline;

	    20 :     { CTL-T }
		Superscript;

	    21 :     { CTL-U }
		W_octal;

	    22 :     { CTL-V }
		Subscript;

	    23 :     { CTL-W }
	        End_alternate;

	    24,25,26,27,28,29,30,31 :       { CTL-X .. CTL-_ }
		W_octal
	    END { case }
    END
    ELSE IF chr(c) IN ['#','$','%','&','~','_','^','\','{','}']
	THEN { LaTeX special characters must be quoted by backslash }
	BEGIN
	    W_char('\');
	    W_byte(c)
	END
    ELSE IF c = Ord('.') THEN { check for WordStar comment }
    BEGIN
	IF eol THEN
	    W_string('%-WS-% ');
	W_byte(c)
    END
    ELSE IF c < 128 THEN { c in 32 .. 127 }
	W_byte(c)
    ELSE { c > 127 }
    BEGIN
	c := c - 128; { lop off 8-th bit }
	CASE c OF
	    0 : { CTL-@ }
		W_octal;

	    1 : { CTL-A }
		W_octal;

	    2 : { CTL-B }
		W_octal;

	    3 : { CTL-C }
		W_octal;

	    4 : { CTL-D }
		W_octal;

	    5 : { CTL-E }
		Begin_boldface;

	    6 : { CTL-F }
		W_octal;

	    7 : { CTL-G }
		W_octal;

	    8 : { CTL-H }
		W_octal;

	    9 : { CTL-I }
		W_byte(ht);


	    10 :        { CTL-J }
		Line_feed;

	    11 :        { CTL-K }
		W_octal;

	    12 :        { CTL-L }
		W_byte(ff);


	    13 :        { CTL-M }
	        IF last_c = Ord('-') THEN
		    hyphen_break := true
	        ELSE IF NOT hyphen_break THEN
		    BEGIN
		        IF bo_eol < output_byte_offset THEN { not double space}
			    W_byte(cr);
		    END;

	    14 :        { CTL-N }
		W_octal;

	    15 :        { CTL-O }
		W_octal;

	    16 :        { CTL-P }
		W_octal;

	    17 :        { CTL-Q }
		Begin_alternate;

	    18 :        { CTL-R }
	        IF is_bold THEN
		    End_boldface;
		{ ELSE discard character }

	    19 :        { CTL-S }
		Underline;

	    20 :        { CTL-T }
		Superscript;

	    21 :        { CTL-U }
		W_octal;

	    22 :        { CTL-V }
		Subscript;

	    23 :        { CTL-W }
	        End_alternate;

	    24 :        { CTL-X }
		W_octal;

	    25 :        { CTL-Y }
		W_octal;

	    26 :        { CTL-Z }
		W_octal;

	    27 :        { CTL-[ }
		W_octal;

	    28 :        { CTL-\ }
		W_octal;

	    29 :        { CTL-] }
		W_octal;

	    30 :        { CTL-^ }
		W_octal;

	    31 :        { CTL-_ }
		W_octal;

	    32,33,34,35,36,37,38,39,40,41,42,43,44,45 :
		W_byte(c);

	    46 : { period }
	    BEGIN
		IF eol THEN
		    W_string('%-WS-% ');
		W_byte(c)
	    END;

	    47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63 :
		W_byte(c);

	    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
	    81,82,83,84,85,86,87,88,89,90,91,92,93,94,95 :
		W_byte(c);

	    96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,
	    111,112,113,114,115,116,117,118,119,120,121,122,123,124,
	    125,126 :
		W_byte(c);

	    127 :
		W_octal;

	    OTHERS :
		W_octal;
	    END { CASE }
    END;
    last_c := wordstar_file^;
    input_byte_offset := input_byte_offset + 1;
    Get(wordstar_file);
    c := wordstar_file^
END;
END. { Main -- WSLTEX }
