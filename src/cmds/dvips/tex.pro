%!
/TeXDict 200 dict def TeXDict begin /bdf{bind def}def /@rigin{72 Resolution
div dup neg scale translate}bdf /@letter{Resolution dup -10 mul @rigin}bdf
/@landscape{[0 1 -1 0 0 0]concat Resolution dup @rigin}bdf /@a4{Resolution dup
-10.6929133858 mul @rigin}bdf /@legal{Resolution dup -13 mul @rigin}bdf
/@manualfeed{statusdict /manualfeed true put}bdf /@copies{/#copies exch def}
bdf /@FontMatrix[1 0 0 -1 0 0]def /@FontBBox[0 0 0 0]def /dmystr(ZZf@@@)def
/newname{dmystr cvn}bdf /df{/fontname exch def dmystr 2 fontname cvx(@@@@)cvs
putinterval newname 7 dict def newname load begin /FontType 3 def /FontMatrix
@FontMatrix def /FontBBox @FontBBox def /BitMaps 256 array def /BuildChar{
CharBuilder}def /Encoding IdentityEncoding def end fontname{/foo setfont}2
array copy cvx def fontname load 0 dmystr 6 string copy cvn cvx put}bdf /dfe{
newname dup load definefont setfont}bdf /ch-image{ch-data 0 get}bdf /ch-width{
ch-data 1 get}bdf /ch-height{ch-data 2 get}bdf /ch-xoff{ch-data 3 get}bdf
/ch-yoff{ch-data 4 get}bdf /ch-dx{ch-data 5 get}bdf /CharBuilder{save 3 1 roll
exch /BitMaps get exch get /ch-data exch def ch-data null ne{ch-dx 0 ch-xoff
ch-yoff neg ch-xoff ch-width add ch-height ch-yoff sub setcachedevice ch-width
ch-height true[1 0 0 -1 -.1 ch-xoff sub ch-height ch-yoff sub .1 add]{
ch-image}imagemask}if restore}bdf /dc{/ch-code exch def /ch-data exch def
newname load /BitMaps get ch-code ch-data put}bdf /bop{gsave /SaveImage save
def 0 0 moveto}bdf /eop{clear SaveImage restore showpage grestore}bdf /@start{
/Resolution exch def /IdentityEncoding 256 array def 0 1 255{IdentityEncoding
exch 1 string dup 0 3 index put cvn put}for}bdf /p{show}bdf /RuleMatrix[1 0 0
-1 -.1 -.1]def /BlackDots 8 string def /v{gsave currentpoint translate false
RuleMatrix{BlackDots}imagemask grestore}bdf /a{moveto}bdf /delta 0 def /tail{
dup /delta exch def 0 rmoveto}bdf /b{exch show tail}bdf /c{show delta 4 sub
tail}bdf /d{show delta 3 sub tail}bdf /e{show delta 2 sub tail}bdf /f{show
delta 1 sub tail}bdf /g{show delta 0 rmoveto}bdf /h{show delta 1 add tail}bdf
/i{show delta 2 add tail}bdf /j{show delta 3 add tail}bdf /k{show delta 4 add
tail}bdf /l{show -4 0 rmoveto}bdf /m{show -3 0 rmoveto}bdf /n{show -2 0
rmoveto}bdf /o{show -1 0 rmoveto}bdf /q{show 1 0 rmoveto}bdf /r{show 2 0
rmoveto}bdf /s{show 3 0 rmoveto}bdf /t{show 4 0 rmoveto}bdf /w{0 rmoveto}bdf
/x{0 exch rmoveto}bdf /y{3 2 roll show moveto}bdf /bos{/section save def}bdf
/eos{clear section restore}bdf end /Resolution 300 def /Inch{Resolution mul}
def /ln03$defs 10 dict def ln03$defs begin /points 256 array def 0 1 255{
points exch[0 0]put}for /linebuf 100 string def /getvarnum{token not{stop}if
exec /varnum exch def}def /get2varnum{{( )anchorsearch{pop pop}{exit}ifelse}
loop( )search{exch pop}{()exch}ifelse(/)search{Pagenum 1 and 0 eq{pop pop}{
exch pop exch pop}ifelse}if cvi /varnum exch def}def /getdimension{exch dup
length 1 sub 0 1 3 -1 roll{pop dup 0 1 getinterval( )eq{dup length 1 sub 1
exch getinterval}{exit}ifelse}for dup length 0 eq{pop}{exch pop dup dup length
2 sub 2 getinterval 1[[1 Inch 72 div(pt)][1 Inch(in)][1 Inch 6 div(pc)][1 Inch
2.54 div(cm)][1 Inch 25.4 div(mm)]]{aload pop 3 index eq{exch pop exit}{pop}
ifelse}forall exch pop exch dup length 2 sub 0 exch getinterval cvr mul}
ifelse}def end /ln03:defpoint{ln03$defs begin{currentfile linebuf readline not
{stop}if getvarnum(\()search not{stop}if pop pop(,)search not{stop}if
currentpoint pop getdimension /x exch def pop(\))search not{stop}if
currentpoint exch pop getdimension[x 3 -1 roll]points varnum 3 -1 roll put pop
pop}stopped{(?Error in \\special ln03:defpoint)print pstack flush stop}if end}
def /ln03:connect{ln03$defs begin{currentfile linebuf readline not{stop}if
get2varnum /firstvarnum varnum def get2varnum 2 getdimension gsave
setlinewidth newpath points firstvarnum get aload pop moveto points varnum get
aload pop lineto stroke grestore}stopped{(?Error in \\special ln03:connect)
print pstack flush stop}if end}def /ln03:resetpoints{ln03$defs begin{
currentfile linebuf readline not{stop}if /firstvarnum 1 def getvarnum dup
token{pop pop /firstvarnum varnum def getvarnum}{pop}ifelse firstvarnum 1
varnum{points exch[0 0]put}for}stopped{(?Error in \\special ln03:resetpoints)
print pstack flush stop}if end}def
