PROGRAM Ws87;
    {
    **********************************************************************
    Copy an 8-bit file  to a 7-bit file,  turning characters with bit  8
    set into 4-character octal escape sequences \nnn.  This is useful in
    analyzing WordStar microcomputer word processor files.

    Usage:

    @WS87
    Old 8-bit file: filespec
    New 7-bit file: filespec

    [10-Oct-85]
    **********************************************************************
    }
VAR
    fp8,fp7 : PACKED FILE OF integer;
BEGIN
Rewrite(output,'tty:');
Write('Old 8-bit file: ');
Reset(fp8,'':@,'/e/b:8');
Write('New 7-bit file: ');
Rewrite(fp7,'':@,'/e/b:7');
WHILE NOT Eof(fp8) DO
    BEGIN
    IF fp8^ > 127 THEN
    BEGIN
        fp7^ := ord('\');
        Put(fp7);
        fp7^ := (fp8^ DIV 64) + ord('0');
        Put(fp7);
        fp7^ := ((fp8^ DIV 8) MOD 8) + ord('0');
        Put(fp7);        
        fp7^ := (fp8^ MOD 8) + ord('0');
        Put(fp7)
    END
    ELSE
    BEGIN
	fp7^ := fp8^;
        Put(fp7)
    END;
    Get(fp8)
    END;
END.
