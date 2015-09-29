Program TEXIndex (Input,Output,InputFile,OutputFile);

{
          XXXXX  XXXXX  X   X  XXX  X   X  XXX    XXXXX  X   X 
            X    X      X   X   X   XX  X  X  X   X      X   X 
            X    X       X X    X   XX  X  X   X  X       X X  
            X    XXXX     X     X   X X X  X   X  XXXX     X   
            X    X       X X    X   X  XX  X   X  X       X X  
            X    X      X   X   X   X  XX  X  X   X      X   X 
            X    XXXXX  X   X  XXX  X   X  XXX    XXXXX  X   X 

**********************************************************************
Creation Date: 2/82

Author  : Skip Montanaro

Address : Lawrence Livermore National Laboratory
          PO 808 / L-226
          Livermore, CA 94550

Revision History:

        First Usable Version :                  2/25/82
        Accepts command line :                  11/24/86

Credits:

The original version of this program was written by Terry Winograd and 
Bill Paxton in INTERLISP. The TEX macro definitions necessary to generate 
TEXIndex's input file and the INTERLISP program can be found in TUGBOAT, 
Volume 1, Number 1, Appendix A.

Running TEXINDEX:

(TEXINDEX is RUN from INDEX.COM. That file just prompts for an input file 
name and ASSIGNs the input and output logical names necessary for the
program.)

Changed to accept file name on the command line.

Input Requirements:

This program expects an input file composed of references as syntactically
described in CSRG:[MONTANRO.TEX]INDEX.SYN. The normal way to create that
file is to let TEX generate them using the macro definitions found in
Appendix A of TUGBOAT, Vol. 1, No. 1. Alternately, the file can be created by
hand, although this is not feasible for long files.

Output:

The output is written to INDEXOUT.TEX and is a file of TEX macros. The
output file is then run through TEX again (with suitable \input files)
to create the final index.

Options:

Two options exist for the user of TEXINDEX. The first concerns the output
format of the final index. The user must \let\indexEntry= one of
\indexLine, \indexPar or \indexComb. This is described completely in
the article in TUGBOAT. The other option is control of span elision.
For instance, the reference span 947-956 is written as 947-56. Page 956
has been elided. To control span elision the user can change the value of
the constant, Elide, at the beginning of the program. It is not anticipated
that this will change very often, so it is not asked for by the program.

**********************************************************************
PROCEDURE and FUNCTION nesting
Procedure GetSymbol
    Function Upshift
    Procedure GetCharacter
Procedure SyntaxError
Procedure Accept
Procedure SemanticError
Procedure Reference
    Procedure ProcessRef
        Procedure DebugPrint
        Function Search
            Function Enter
        Function SearchPageList
            Function EnterPage
            Function HigherPrecedenceOf
Procedure Dump
    Procedure TraversePageList
        Function AlteredEndPage
    Procedure TraverseCrossRefs
}
{}


    Label

        1,9999;

    Const

        { Settable by user to control elision of page numbers in reference
          spans. }
        Elide           = True;

        Space           = ' ';
        Tab             = '     ';

        MaxAlfa = 80;                                   { arbitrarily long }
        NullName =                                      { MaxAlfa spaces }
'                                                                                ';
        MaxLineLength   = 132;                          { arbitrary }

    Type

        { Symbols }
        SymbolType = (Identifier, Semicolon, RightAngle, LeftAngle,
                      Null);

        ErrorType = (SpanOpen,NoSpanStart,NoSpanClose,UnexpectedSymbol,
                     TwoSpansOnPage);

        AlfaLength = 0..MaxAlfa;
        AlfaRange = 1..MaxAlfa;
        Alfa = Packed Array [AlfaRange] of Char;

        LineRange = 1..MaxLineLength;
        InputLine = Packed Array [LineRange] of Char;

        { Symbol Table Stuff }

        PagePtr = ^PageRef;
        TableEntryPtr = ^TableEntry;

        { Binary tree sorted by value of StartPage }
        PageRef = Record
            LowerPage,
            HigherPage : PagePtr;
            Font,               { font used to print reference }
            RefType : Char;     { type of reference - normal, span, following
                                  folio or note }
            StartPage,
            EndPage : Integer
        End;

        { Binary tree sorted by value of SortedSpelling }
        TableEntry = Record
            Spelling,                   { entry name }
            SortedSpelling : Alfa;      { upshifted - used only for sorting }
            SpellLength : AlfaLength;   { length of name in character array }
            PageList,                   { binary tree of page references for
                                          this particular entry }
            LastEndSpan,                { last span reference closed }
            LastSpan : PagePtr;         { last span reference opened - Nil if
                                          none are currently open }
            NextLevel : TableEntryPtr;  { root of binary tree of subheadings }
            CrossRefList : TableEntryPtr;
                                        { root of binary tree of cross
                                          references to this entry }
            LeftChild,
            RightChild : TableEntryPtr
        End;
{}


    Var

        FormFeed : Char;                { ASCII Form Feed }

        Symbol : SymbolType;            { Last symbol parsed }
        Ch : Char;                      { Last character parsed }
        PageValue : Integer;            { Numeric value of last page number }
        Spelling,                       { Spelling of last identifier
                                          or number }
        SortedSpelling : Alfa;
        SymbolLength : AlfaLength;      { Length of last identifier }
        Line : InputLine;               { Current input line }
        CharCount,                      { Character position in Line }
        LineLength                      { Length of current input line }
            : 0..MaxLineLength;
        LineNumber : 0..Maxint;         { Current Line Number }
        SymbolTable : TableEntryPtr;

        InputFile,                      { User input file }
        OutputFile : Text;              { User output file }

        LastLetter : Char;              { Current index letter being dumped }

        LastRefPrinted : Char;          { Used to see if \pageNumDot must be
                                          printed after the last reference
                                          that is following folio or note. }

Value

        { Compile time initialization }
        LastRefPrinted := Space;
        LastLetter := Space;
        { Ch = Space guarantees GetSymbol calls GetCharacter when first called }
        Ch := Space;
        { CharCount = LineLength guarantees the first line will be read }
        CharCount := 0;
        LineLength := 0;
        { Empty symbol table }
        SymbolTable := Nil;
        { identifiers full of blanks }
        Spelling := NullName;
        SortedSpelling := NullName;
{}


Procedure GetSymbol;
{ Updates: Symbol, Spelling, SortedSpelling, SymbolLength, PageValue }
{ Refers to: Ch, FormFeed }
{ Calls: GetCharacter, UpShift }
{ Picks off the next symbol in the input stream }

Var
  i : integer ;  { locally used indexing variable }



    Function Upshift
    ( { Using } Ch :                    Char)
      { Returning } :                   Char;
    { If Ch is a letter it returns the uppercase of Ch, otherwise it returns Ch }

    Begin { UpShift }

        If Ch in ['a'..'z'] then
            Upshift := Chr (ord('A') + ord(Ch) - ord('a'))
        Else Upshift := Ch

    End; { UpShift }



    Procedure GetCharacter;
    { Updates: Ch, Line, CharCount, LineLength }
    { Refers to: InputFile }
    { Calls: Read, Eof, Eoln }
    { Fetches next character from the input stream }

    Var
      Clear : integer;  { locally used indexing variable
                          used to blank Line }

    Begin { Get Character }

        If CharCount = LineLength then Begin

            { Quit processing input if EOF }
            If Eof (InputFile) then goto 1;

            { Read next input line }
            LineLength := 0;
            CharCount := 0;
            For Clear := 1 to MaxLineLength do
                Line[Clear] := Space;
            LineNumber := LineNumber + 1;
            While not Eoln (InputFile) do Begin
                LineLength := LineLength + 1;
                Read (InputFile, Ch);
                Line[LineLength] := Ch
            End; { While }
            LineLength := LineLength + 1;
            Read (InputFile, Ch); { read past Eoln }
            Line[LineLength] := Ch;

            { Pass all lines that do not begin with '<' directly to the
              output file }
            If Line[1] <> '<' then Begin
                Writeln ( OutputFile, Line:MaxLineLength );
                LineLength := 0;
                GetCharacter
            End;

            { First character on the line }
            CharCount := 1;
            Ch := Line[CharCount]

        End { If CharCount = LineLength }

        Else Begin

            { Advance to next character on line }
            CharCount := CharCount + 1;
            Ch := Line[CharCount]

        End

    End; { Get Character }
{}


Begin { Get Symbol }

    While Ch in [Space, Tab, FormFeed] do 
        GetCharacter;

    Case Ch of

    ';' : Begin
        Symbol := Semicolon;
        GetCharacter
    End; { ';' }

    '<' : Begin
        Symbol := LeftAngle;
        GetCharacter
    End; { '<' }

    '>' : Begin
        Symbol := RightAngle;
        GetCharacter
    End; { '>' }

    '0','1','2','3','4','5','6','7','8','9' : Begin
        Symbol := Identifier;
        PageValue := ord (Ch) - ord ('0');
        GetCharacter;
        While Ch in ['0'..'9'] do Begin
            PageValue := PageValue * 10 + ord (Ch) - ord('0');
            GetCharacter
        End; { While }
        Spelling := NullName;
        SortedSpelling := NullName
    End; { Ch in ['0'..'9'] }

    Otherwise Begin 
    { everything else - usually alphabetic - but anything except '<', '>'
      and ';' is allowed }
        Symbol := Identifier;
        { blank now so we don't have to blank pad after parsing identifier }
        Spelling := NullName;
        SortedSpelling := NullName;

        { first character }
        Spelling[1] := Ch;
        SortedSpelling[1] := UpShift (Ch);
        SymbolLength := 1;
        GetCharacter;

        { fetch the rest }
        While not (Ch in ['<','>',';']) do Begin

            { test prevents overflowing character array ==> truncate long
              identifiers }
            If SymbolLength <= MaxAlfa then Begin
                SymbolLength := SymbolLength + 1;
                Spelling[SymbolLength] := Ch;
                SortedSpelling[SymbolLength] := UpShift (Ch)
            End;
            GetCharacter
        End ; { While }

        { Symbols that start with a single '\' represent special characters.
          As such, they should appear together.  Symbols that start with
          '\\' represent index entries that should papear as '\entry' in the
          index.  In this case, the leading '\' should be ignored in the
          alphabetic sort }

        { Look for \\entry }
        If SymbolLength >= 2 then
            If ( SortedSpelling[ 1 ] = '\' ) and
               ( SortedSpelling[ 2 ] = '\' ) then
                If SymbolLength = 2 then Begin  { was exactly \\ }
                    SortedSpelling[ 1 ] := Space ;
                    SortedSpelling[ 2 ] := Space
                End { then }
                Else Begin  { was \\entry - make just entry }
                    For i := 3 to SymbolLength Do
                        SortedSpelling[ i-2 ] := SortedSpelling[ i ] ;
                    SortedSpelling[ SymbolLength - 1 ] := Space ;
                    SortedSpelling[ SymbolLength     ] := Space
                End { else }

    End { Otherwise }

    End { Case }

End; { Get Symbol }
{}




Procedure SyntaxError
( { Using }  ErrorSymbol :                      ErrorType;
             ExpectedSymbol :                   SymbolType);
{ Displays appropriate error message on terminal }

Begin { Syntax Error }

    Write ('Syntax Error --> ');
    Case ErrorSymbol of
        UnexpectedSymbol : Writeln ('Unexpected ',Symbol:10,
                         ' found in input stream. ',ExpectedSymbol:10,
                         ' was expected.')
    End { Case ErrorSymbol }

End; { Syntax Error }


Procedure Accept
( { Using } ExpectedSymbol :                Symboltype);
{ Refers to: Symbol }
{ Calls: GetSymbol }
{ Checks to make sure the proper symbol is present in the input text and 
  advances the parser to the next symbol. If an unexpected symbol is found, 
  an error message is generated and the old symbol is retained }

Begin { Accept }

    if Symbol = ExpectedSymbol then
        GetSymbol
    Else
        SyntaxError (UnexpectedSymbol,ExpectedSymbol)

End; { Accept }



Procedure SemanticError
( { Using }  ErrorSymbol :                      ErrorType;
             PageValue :                        Integer);
{ Displays appropriate error message on terminal }

Begin { Semantic Error }

    Write ('Semantic Error --> ');
    Case ErrorSymbol of

        NoSpanStart : Writeln ('No reference span open on page ',
                         PageValue:1,' during attempt to close span.');

        SpanOpen : Writeln ('Reference span already open during attempt ',
                         'to open span on page ',PageValue:1,'.');

        NoSpanClose : Writeln ('Reference span that started on page ',
                         PageValue:1,' was never closed.');

        TwoSpansOnPage : Writeln ('Two span references present on ',
                         PageValue:1,'.')

    End { Case ErrorSymbol }

End; { Semantic Error }



{}



Procedure Reference;
{ Refers to: Symbol, Spelling, SortedSpelling, SymbolLength }
{ Calls: Accept, ProcessRef }
{ Parses a single reference }

Var

    Field1,
    SortedField1 : Alfa;
    Field1Length : AlfaLength;
    FontType,
    RefType : Char;



    Procedure ProcessRef
    ( { Using }  Target,
                 SortedTarget :                 Alfa;
                 TargetLength :                 AlfaLength;
                 FontType,
                 RefType :                      Char);
    { Refers to : PageValue }
    { Calls: DebugPrint (diagnostic only), Search, Accept, SearchPageList,
      SemanticError
    { Enters References in symbol table }

    Var

        Entry : TableEntryPtr ;
        PageEntry : PagePtr;

        Procedure DebugPrint
        ( { Using } Root :                              TableEntryPtr;
                    Indent :                            Integer);
        { Dumps Symbol Table }

        Begin { Debug Print }

            With Root^ do

                If Root <> Nil then Begin
                    Writeln (' ':Indent,Spelling:SpellLength);
                    DebugPrint (LeftChild,Indent+2);
                    DebugPrint (RightChild,Indent+2)
                End { If }

        End; { Debug Print }
{}



        Function Search
        ( { Using }  Name,
                     SortedName :                       Alfa;
                     NameLength :                       AlfaLength;
                     FontType,
                     RefType :                          Char;
          { Alters } Var Root :                         TableEntryPtr)
          { Returns } :                                 TableEntryPtr;
        { Searches tree headed by Root for Name, creating entry if not found.
          returns a pointer to the found/created entry. }

        Var

            Last,
            Temp : TableEntryPtr;
            Finished,
            LeftTaken : Boolean;

            Function Enter
            { Returns } :                               TableEntryPtr;
            { Refers to: Name, SortedName, NameLength }
            { Creates an entry in the symbol table }

            Var

                Entry : TableEntryPtr;

            Begin { Enter }

                New (Entry);
                With Entry^ do Begin
                    Spelling := Name;
                    SortedSpelling := SortedName;
                    SpellLength := NameLength;
                    PageList := Nil;
                    LastSpan := Nil;
                    NextLevel := Nil;
                    CrossRefList := Nil;
                    LeftChild := Nil;
                    RightChild := Nil
                End; { With Entry^ }
                Enter := Entry

            End; { Enter }
{}



        Begin { Search }
            If Root = Nil then Begin { Empty Tree - create new entry }
                Root := Enter;
                Search := Root

            End
            Else Begin
                Temp := Root;

                { Binary Search of Tree headed by Temp }
                Finished := Temp^.SortedSpelling = SortedName;

                While Not Finished do Begin

                    { If another loop is necessary, which branch will we take? }
                    LeftTaken := Temp^.SortedSpelling > SortedName;

                    { Save where we came from }
                    Last := Temp;

                    { Take appropriate branch }
                    If LeftTaken then
                        Temp := Last^.LeftChild
                    Else
                        Temp := Last^.RightChild;

                    { If we run out of entries then back up one and create an entry }
                    If Temp = Nil then Begin

                        Temp := Enter;
                        If LeftTaken then
                            Last^.LeftChild := Temp
                        Else
                            Last^.RightChild := Temp

                    End; { Else }

                    Finished := Temp^.{Sorted}Spelling = {Sorted}Name

                End; { While Not Finished }
                Search := Temp

            End { Else Begin }

        End; { Search }
{}

        Function SearchPageList
        ( { Using } PageValue :                         Integer;
                    FType, { Font Type }
                    RType : { Ref Type }                Char;
          { Alters }Var Root :                          PagePtr)
          { Returns } :                                 PagePtr;
        { Similar to Search, but looks through page lists. }

        Var

            Temp,
            Last : PagePtr;
            Finished,
            LeftTaken : Boolean;


            Function EnterPage
            { Returns } :                               PagePtr;
            { Creates an entry in the page list symbol table }

            Var

                Entry : PagePtr;

            Begin { Enter Page }

                New (Entry);
                With Entry^ do Begin
                    StartPage := PageValue;
                    EndPage := -1;
                    RefType := RType;
                    Font := FType;
                    LowerPage := Nil;
                    HigherPage := Nil
                End; { With Entry^ }
                EnterPage := Entry

            End; { Enter Page }
{}


            Function HigherPrecedenceOf
            ( { Using } RType1,
                        RType2 :                        Char)
              { Returns } :                             Char;
            { Picks higher precedence reference type }

            Var

                RefSet : Set of Char;

            Begin { Higher Precedence Of }

                RefSet := [RType1,RType2];

                If 'S' in RefSet then
                    HigherPrecedenceOf := 'S'
                Else If 'F' in RefSet then
                    HigherPrecedenceOf := 'F'
                Else if 'N' in RefSet then
                    HigherPrecedenceOf := 'N'
                Else
                    HigherPrecedenceOf := 'P'

            End; { Higher Precedence Of }
{}


        Begin { Search Page List }

            If Root = Nil then Begin { Empty Tree - create new entry }
                Root := EnterPage;
                SearchPageList := Root

            End
            Else Begin
                Temp := Root;

                { Binary Search of Tree headed by Temp }
                Finished := Temp^.StartPage = PageValue;

                While Not Finished do Begin

                    { If another loop is necessary, which branch will we take? }
                    LeftTaken := Temp^.StartPage > PageValue;

                    { Save where we came from }
                    Last := Temp;

                    { Take appropriate branch }
                    If LeftTaken then
                        Temp := Last^.LowerPage
                    Else
                        Temp := Last^.HigherPage;

                    { If we run out of entries then back up one and create an entry }
                    If Temp = Nil then Begin

                        Temp := EnterPage;
                        If LeftTaken then
                            Last^.LowerPage := Temp
                        Else
                            Last^.HigherPage := Temp

                    End; { Else }

                    Finished := Temp^.StartPage = PageValue

                End; { While Not Finished }

                { Error if last span ends on same page the current one
                  begins on, if a span is even active }
                With Entry^ do
                    If LastSpan <> Nil then
                        If LastEndSpan^.EndPage = Temp^.StartPage then
                            SemanticError (TwoSpansOnPage,Temp^.StartPage);

                Temp^.RefType := HigherPrecedenceOf (Temp^.RefType,RType);

                SearchPageList := Temp

            End { Else Begin }

        End; { Search Page List }
{}



    Begin { Process Ref }

        Entry := 
            Search (Spelling,SortedSpelling,SymbolLength,FontType,
                RefType,SymbolTable);
        Accept (Identifier);
        While Symbol = Semicolon do Begin
           Accept (Semicolon);
           Entry :=
               Search (Spelling,SortedSpelling,SymbolLength,FontType,
                   RefType,Entry^.NextLevel);
           Accept (Identifier)
        End; { While }

        Case RefType of 

            'C' : Entry :=
                Search (Target,SortedTarget,TargetLength,
                    FontType,RefType,Entry^.CrossRefList);

            'P' : PageEntry := 
                SearchPageList (PageValue,FontType,RefType,Entry^.PageList);

            'F' : PageEntry := 
                SearchPageList (PageValue,FontType,RefType,Entry^.PageList);

            'S' : With Entry^ do 
                If LastSpan <> Nil then
                    SemanticError (SpanOpen,PageValue)
                Else
                    LastSpan := SearchPageList (PageValue,FontType,RefType,
                        Entry^.PageList);

            'E' : With Entry^ do Begin
                If LastSpan = Nil then
                    SemanticError (NoSpanStart,PageValue)
                Else if LastSpan^.StartPage = PageValue then
                    LastSpan^.RefType := 'P'
                Else Begin
                    LastSpan^.EndPage := PageValue;
                    LastEndSpan := LastSpan
                End;
                LastSpan := Nil
            End; { Case 'E' }

            'N' : PageEntry := 
                SearchPageList (PageValue,FontType,RefType,Entry^.PageList)

        End { Case RefType }

    End; { Process Ref }
{}




Begin { Reference }

    Accept (LeftAngle);

    { If the identifier present is a page number its value is stored in the
      global variable, PageValue; Spelling and SortedSpelling are both blank. }
    Field1 := Spelling;
    SortedField1 := SortedSpelling;
    Field1Length := SymbolLength;
    Accept (Identifier);
    Accept (Semicolon);

    FontType := SortedSpelling[1];
    Accept (Identifier);
    Accept (Semicolon);

    RefType := SortedSpelling[1];
    Accept (Identifier);
    Accept (Semicolon);

    ProcessRef (Field1,SortedField1,Field1Length,FontType,RefType);

    Accept (RightAngle)

End; { Reference }
{}


Procedure Dump
( { Using } Root :                      TableEntryPtr;
            Level :                     Integer);
{ Traverses and prints the symbol table in order }



    Procedure TraversePageList
    ( { Using } PageRoot :                      PagePtr;
                ElideRefs :                     Boolean);
    { Traverses and prints the Page List Tree, with optional Eliding }

    Const

        Comma = ', ';

    Var

        I : Integer;

        Function AlteredEndPage
        ( { Using } StartP,
                    EndP :                      Integer)
          { Returns } : Integer;
        { Returns that part of EndP to be printed, taking elision into account. }

        Begin { Altered End Page }

            If (StartP mod 100 = 0) or not ElideRefs then
                AlteredEndPage := EndP
            Else
                AlteredEndPage := EndP mod 100

        End; { Altered End Page }
{}


    Begin { Traverse Page List }

        With PageRoot^ do Begin

            If LowerPage <> Nil then
                TraversePageList (LowerPage,ElideRefs);

            Write (OutputFile,Comma);

            If Font = 'B' then
                Write (OutputFile,'\mainEntry{');

            Case RefType of

                'S' : Begin

                    If EndPage = -1 then
                        SemanticError (NoSpanClose,StartPage);
                    Write (OutputFile,'\indexSpan ',StartPage:1,'-');
                    EndPage := AlteredEndPage (StartPage,EndPage);
                    Write (OutputFile,EndPage:1,'.')

                End; { RefType = 'S' }

                'F' : Write (OutputFile,'\indexFF ',StartPage:1,'.');

                'N' : Write (OutputFile,'\indexN ',StartPage:1,'.');

                'P' : Write (OutputFile,StartPage:1)

            End; { Case RefType of }

            LastRefPrinted := RefType;

            If Font = 'B' then
                Write (OutputFile,'}');

            If HigherPage <> Nil then
                TraversePageList (HigherPage,ElideRefs)

        End { With }

    End; { Traverse Page List }
{}


    Procedure TraverseCrossRefs
    ( { Using } CrossRoot :                     TableEntryPtr);
    { Traverses and prints the Cross Reference list }

    Const

        Comma = ', ';

    Begin { Traverse Cross Refs }

        With CrossRoot^ do Begin
            { Traverse left subtree }
            If LeftChild <> Nil then Begin
                TraverseCrossRefs (LeftChild);
                Write (OutputFile,Comma)
            End;

            { Print this node }
            Write (OutputFile,Spelling:SpellLength);

            { Traverse Right subtree }
            If RightChild <> Nil then Begin
                Write (OutputFile,Comma);
                TraverseCrossRefs (RightChild)
            End
        End { With CrossRoot^ }

    End; { Traverse Cross Refs }
{}



Begin { Dump }

    With Root^ do Begin

        { Traverse Left Sub-tree }
        If LeftChild <> Nil then
            Dump (LeftChild,Level);

        { Dump this node }

        If (LastLetter <> SortedSpelling[1]) and (Level = 0) then Begin
            Writeln (OutputFile,'\indexChar{',SortedSpelling[1],'}');
            LastLetter := SortedSpelling[1]
        End;

        Write (OutputFile,'\indexEntry',Level:1,'{',Spelling:SpellLength);
        If PageList <> Nil then Begin
            Write (OutputFile, '\') ;
            TraversePageList (PageList,Elide);
            If LastRefPrinted in ['F','N'] then
                Write (OutputFile,'\pageNumDot')
        End;
        Write (OutputFile,'}{{');

        If CrossRefList <> Nil then Begin
            If PageList <> Nil then
                Write (OutputFile,'\indexAlso{')
            Else Write (OutputFile,'\indexSee{');
            TraverseCrossRefs (CrossRefList);
            Write (OutputFile,'}')
        End;

        Write (OutputFile,'}');

        If NextLevel <> Nil then Begin
            Writeln (OutputFile,'+{');
            Dump (NextLevel,Level+1);
            Writeln (OutputFile,'}}')
        End
        Else Writeln (OutputFile,'-{}}');

        { Traverse Right Sub-tree }
        If RightChild <> Nil then
            Dump (RightChild,Level)

    End { With Root^ }

End; { Dump }
{}

procedure OpenFiles;

type
   word= 0..65535;
var
  command_line:packed array[1..300] of char;
  cmd_len:word;
  cmd_i:integer;
  file_name,def_file_name:varying [300] of char;
  ask,got_file_name: boolean;

[external] function lib$get_foreign(
  %stdescr cmdlin:[volatile] packed array [$l1..$u1:integer] of char
        := %immed 0;
  %stdescr prompt:[volatile] packed array [$l2..$u2:integer] of char
        := %immed 0;
  var len : [volatile] word := %immed 0;
  var flag : [volatile] integer := %immed 0)
    :integer; extern;

begin { OpenFiles }
  cmd_i:=0;
  lib$get_foreign(command_line,,cmd_len,cmd_i);
  cmd_i:=1;
  while (cmd_i<=cmd_len) and (command_line[cmd_i]=' ') do cmd_i:=cmd_i+1;
  got_file_name:=cmd_i<=cmd_len;
  if got_file_name then
        def_file_name:=substr(command_line,cmd_i,cmd_len-cmd_i+1);

  if got_file_name then begin
        file_name:=def_file_name+'.IDX';
        open(InputFile,file_name,readonly, error:=continue);
        ask:=status(InputFile)<>0;
        if ask then writeln('Couldn''t open ',file_name);
        end
else ask:=true;
while ask do begin
        got_file_name:=false;
        write('Index input file: ');
        if eof then goto 9999;
        readln(file_name);
        open(InputFile,file_name,readonly, error:=continue);
        ask:=status(InputFile)<>0;
        if ask then writeln('Couldn''t open ',file_name);
        end;
reset(InputFile);

if got_file_name then begin
        cmd_i:=1;
        for cmd_len:=1 to def_file_name.length do
                if (def_file_name[cmd_len]=']')
                or (def_file_name[cmd_len]=':')
                then cmd_i:=cmd_len+1;
        if cmd_i<=def_file_name.length then
                def_file_name:=substr(def_file_name,cmd_i,
                        def_file_name.length-cmd_i+1);
        file_name:=def_file_name+'.IND';
        open(OutputFile,file_name,new,32767,disposition:=delete,
                error:=continue);
        ask:=status(OutputFile)>0;
        if ask then writeln('Couldn''t open ',file_name);
        end
else ask:=true;
while ask do begin
        write('Index output file: ');
        if eof then goto 9999;
        readln(file_name);
        open(OutputFile,file_name,new,32767,disposition:=delete,
                error:=continue);
        ask:=status(OutputFile)>0;
        if ask then writeln('Couldn''t open ',file_name);
        end;
rewrite(OutputFile);
end;  { OpenFiles }

{}

Begin { Main Program }

    OpenFiles;

    GetSymbol;

    While Symbol = LeftAngle do 
        Reference;

1:  Close (InputFile);

    if SymbolTable<>Nil then
      begin
        Writeln (OutputFile,'\indexStart');
        Dump (SymbolTable,0);
        Writeln (OutputFile,'\indexEnd');
      end
    else
        Writeln (OutputFile,'\relax');
    Close( OutputFile, disposition:=save );
9999:
End. { Main Program }
