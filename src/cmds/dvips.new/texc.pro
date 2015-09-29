%!
/VirginMtrx 6 array currentmatrix def /dummystring 20 string def /numberpos 36
def /printnumber{gsave VirginMtrx setmatrix /Helvetica findfont 10 scalefont
setfont 36 numberpos moveto /numberpos numberpos 12 add def dummystring cvs
show grestore}bind def /showVM{vmstatus exch sub exch pop printnumber}def
/eop-aux{showVM}def /TeXDict 200 dict def TeXDict begin /N /def load def /B{
bind def}N /S /exch load def /X{S N}B /TR /translate load N /isls false N
/vsize 10 N /@rigin{isls{[0 1 -1 0 0 0]concat}if 72 Resolution div 72
VResolution div neg scale Resolution VResolution vsize neg mul TR matrix
currentmatrix dup dup 4 get round 4 exch put dup dup 5 get round 5 exch put
setmatrix}B /@letter{/vsize 10 N}B /@landscape{/isls true N /vsize -1 N}B /@a4
{/vsize 10.6929133858 N}B /@a3{/vsize 15.5531 N}B /@ledger{/vsize 16 N}B
/@legal{/vsize 13 N}B /@manualfeed{statusdict /manualfeed true put}B /@copies{
/#copies X}B /FMat[1 0 0 -1 0 0]N /FBB[0 0 0 0]N /df{/sf 1 N /fntrx FMat N
df-tail}B /dfs{div /sf X /fntrx[sf 0 0 sf neg 0 0]N df-tail}B /df-tail{/nn 8
dict N nn begin /FontType 3 N /FontMatrix fntrx N /FontBBox FBB N string /base
X array /BitMaps X /BuildChar{CharBuilder}N /Encoding IE N end dup{/foo
setfont}2 array copy cvx N load 0 nn put /ctr 0 N[}B /E{pop nn dup definefont
setfont}B /ch-image{ch-data dup type /stringtype ne{ctr get /ctr ctr 1 add N}
if}B /ch-width{ch-data dup length 5 sub get}B /ch-height{ch-data dup length 4
sub get}B /ch-xoff{128 ch-data dup length 3 sub get sub}B /ch-yoff{ch-data dup
length 2 sub get 127 sub}B /ch-dx{ch-data dup length 1 sub get}B /ctr 0 N
/CharBuilder{save 3 1 roll S dup /base get 2 index get S /BitMaps get S get
/ch-data X pop /ctr 0 N ch-dx 0 ch-xoff ch-yoff ch-height sub ch-xoff ch-width
add ch-yoff setcachedevice ch-width ch-height true[1 0 0 -1 -.1 ch-xoff sub
ch-yoff .1 add]/id ch-image N /rw ch-width 7 add 8 idiv string N /rc 0 N /gp 0
N /cp 0 N{rc 0 ne{rc 1 sub /rc X rw}{G}ifelse}imagemask restore}B /G{{id gp
get /gp gp 1 add N dup 18 mod S 18 idiv pl S get exec}loop}B /adv{cp add /cp X
}B /chg{rw cp id gp 4 index getinterval putinterval dup gp add /gp X adv}B /nd
{/cp 0 N rw exit}B /lsh{rw cp 2 copy get dup 0 eq{pop 1}{dup 255 eq{pop 254}{
dup dup add 255 and S 1 and or}ifelse}ifelse put 1 adv}B /rsh{rw cp 2 copy get
dup 0 eq{pop 128}{dup 255 eq{pop 127}{dup 2 idiv S 128 and or}ifelse}ifelse
put 1 adv}B /clr{rw cp 2 index string putinterval adv}B /set{rw cp fillstr 0 4
index getinterval putinterval adv}B /fillstr 18 string 0 1 17{2 copy 255 put
pop}for N /pl[{adv 1 chg}bind{adv 1 chg nd}bind{1 add chg}bind{1 add chg nd}
bind{adv lsh}bind{adv lsh nd}bind{adv rsh}bind{adv rsh nd}bind{1 add adv}bind{
/rc X nd}bind{1 add set}bind{1 add clr}bind{adv 2 chg}bind{adv 2 chg nd}bind{
pop nd}bind]N /D{/cc X dup type /stringtype ne{]}if nn /base get cc ctr put nn
/BitMaps get S ctr S sf 1 ne{dup dup length 1 sub dup 2 index S get sf div put
}if put /ctr ctr 1 add N}B /I{cc 1 add D}B /bop{userdict /bop-hook known{
bop-hook}if /SI save N @rigin 0 0 moveto}B /eop{clear SI restore showpage
userdict /eop-hook known{eop-hook}if}B /@start{userdict /start-hook known{
start-hook}if /VResolution X /Resolution X 1000 div /DVImag X /IE 256 array N
0 1 255{IE S 1 string dup 0 3 index put cvn put}for}B /p /show load N /RMat[1
0 0 -1 0 0]N /BDot 260 string N /v{/ruley X /rulex X V}B /V statusdict begin
/product where{pop product 0 7 getinterval(Display)eq}{false}ifelse end{{
gsave TR -.1 -.1 TR 1 1 scale rulex ruley false RMat{BDot}imagemask grestore}}
{{gsave TR -.1 -.1 TR rulex ruley scale 1 1 false RMat{BDot}imagemask grestore
}}ifelse B /a{moveto}B /delta 0 N /tail{dup /delta X 0 rmoveto}B /M{S p delta
add tail}B /b{S p tail}B /c{-4 M}B /d{-3 M}B /e{-2 M}B /f{-1 M}B /g{0 M}B /h{
1 M}B /i{2 M}B /j{3 M}B /k{4 M}B /l{p -4 w}B /m{p -3 w}B /n{p -2 w}B /o{p -1 w
}B /q{p 1 w}B /r{p 2 w}B /s{p 3 w}B /t{p 4 w}B /w{0 rmoveto}B /x{0 S rmoveto}
B /y{3 2 roll p a}B /bos{/SS save N}B /eos{clear SS restore}B end
