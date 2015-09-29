%!PS-Adobe-1.0
%%Creator: sniff:adam (Adam de Boor,Ext. 238,,5492264)
%%Title: stdin (ditroff)
%%CreationDate: Sun Jan  8 20:04:05 1989
%%EndComments
%      @(#)psdit.pro   1.3 4/15/88
% lib/psdit.pro -- prolog for psdit (ditroff) files
% Copyright (c) 1984, 1985 Adobe Systems Incorporated. All Rights Reserved.
% last edit: shore Sat Nov 23 20:28:03 1985
% RCSID: $Header: psdit.pro,v 2.1 85/11/24 12:19:43 shore Rel $

% Changed by Edward Wang (edward@ucbarpa.berkeley.edu) to handle graphics,
% 17 Feb, 87.

/$DITroff 140 dict def $DITroff begin
/fontnum 1 def /fontsize 10 def /fontheight 10 def /fontslant 0 def
/xi{0 72 11 mul translate 72 resolution div dup neg scale 0 0 moveto
 /fontnum 1 def /fontsize 10 def /fontheight 10 def /fontslant 0 def F
 /pagesave save def}def
/PB{save /psv exch def currentpoint translate 
 resolution 72 div dup neg scale 0 0 moveto}def
/PE{psv restore}def
/arctoobig 90 def /arctoosmall .05 def
/m1 matrix def /m2 matrix def /m3 matrix def /oldmat matrix def
/tan{dup sin exch cos div}def
/point{resolution 72 div mul}def
/dround        {transform round exch round exch itransform}def
/xT{/devname exch def}def
/xr{/mh exch def /my exch def /resolution exch def}def
/xp{}def
/xs{docsave restore end}def
/xt{}def
/xf{/fontname exch def /slotno exch def fontnames slotno get fontname eq not
 {fonts slotno fontname findfont put fontnames slotno fontname put}if}def
/xH{/fontheight exch def F}def
/xS{/fontslant exch def F}def
/s{/fontsize exch def /fontheight fontsize def F}def
/f{/fontnum exch def F}def
/F{fontheight 0 le{/fontheight fontsize def}if
 fonts fontnum get fontsize point 0 0 fontheight point neg 0 0 m1 astore
 fontslant 0 ne{1 0 fontslant tan 1 0 0 m2 astore m3 concatmatrix}if
 makefont setfont .04 fontsize point mul 0 dround pop setlinewidth}def
/X{exch currentpoint exch pop moveto show}def
/N{3 1 roll moveto show}def
/Y{exch currentpoint pop exch moveto show}def
/S{show}def
/ditpush{}def/ditpop{}def
/AX{3 -1 roll currentpoint exch pop moveto 0 exch ashow}def
/AN{4 2 roll moveto 0 exch ashow}def
/AY{3 -1 roll currentpoint pop exch moveto 0 exch ashow}def
/AS{0 exch ashow}def
/MX{currentpoint exch pop moveto}def
/MY{currentpoint pop exch moveto}def
/MXY{moveto}def
/cb{pop}def    % action on unknown char -- nothing for now
/n{}def/w{}def
/p{pop showpage pagesave restore /pagesave save def}def
/Dt{/Dlinewidth exch def}def 1 Dt
/Ds{/Ddash exch def}def -1 Ds
/Di{/Dstipple exch def}def 1 Di
/Dsetlinewidth{2 Dlinewidth mul setlinewidth}def
/Dsetdash{Ddash 4 eq{[8 12]}{Ddash 16 eq{[32 36]}
 {Ddash 20 eq{[32 12 8 12]}{[]}ifelse}ifelse}ifelse 0 setdash}def
/Dstroke{gsave Dsetlinewidth Dsetdash 1 setlinecap stroke grestore
 currentpoint newpath moveto}def
/Dl{rlineto Dstroke}def
/arcellipse{/diamv exch def /diamh exch def oldmat currentmatrix pop
 currentpoint translate 1 diamv diamh div scale /rad diamh 2 div def
 currentpoint exch rad add exch rad -180 180 arc oldmat setmatrix}def
/Dc{dup arcellipse Dstroke}def
/De{arcellipse Dstroke}def
/Da{/endv exch def /endh exch def /centerv exch def /centerh exch def
 /cradius centerv centerv mul centerh centerh mul add sqrt def
 /eradius endv endv mul endh endh mul add sqrt def
 /endang endv endh atan def
 /startang centerv neg centerh neg atan def
 /sweep startang endang sub dup 0 lt{360 add}if def
 sweep arctoobig gt
 {/midang startang sweep 2 div sub def /midrad cradius eradius add 2 div def
  /midh midang cos midrad mul def /midv midang sin midrad mul def
  midh neg midv neg endh endv centerh centerv midh midv Da
  Da}
 {sweep arctoosmall ge
  {/controldelt 1 sweep 2 div cos sub 3 sweep 2 div sin mul div 4 mul def
   centerv neg controldelt mul centerh controldelt mul
   endv neg controldelt mul centerh add endh add
   endh controldelt mul centerv add endv add
   centerh endh add centerv endv add rcurveto Dstroke}
  {centerh endh add centerv endv add rlineto Dstroke}
  ifelse}
 ifelse}def
/Dpatterns[
[%cf[widthbits]
[8<0000000000000010>]
[8<0411040040114000>]
[8<0204081020408001>]
[8<0000103810000000>]
[8<6699996666999966>]
[8<0000800100001008>]
[8<81c36666c3810000>]
[8<0f0e0c0800000000>]
[8<0000000000000010>]
[8<0411040040114000>]
[8<0204081020408001>]
[8<0000001038100000>]
[8<6699996666999966>]
[8<0000800100001008>]
[8<81c36666c3810000>]
[8<0f0e0c0800000000>]
[8<0042660000246600>]
[8<0000990000990000>]
[8<0804020180402010>]
[8<2418814242811824>]
[8<6699996666999966>]
[8<8000000008000000>]
[8<00001c3e363e1c00>]
[8<0000000000000000>]
[32<00000040000000c00000004000000040000000e0000000000000000000000000>]
[32<00000000000060000000900000002000000040000000f0000000000000000000>]
[32<000000000000000000e0000000100000006000000010000000e0000000000000>]
[32<00000000000000002000000060000000a0000000f00000002000000000000000>]
[32<0000000e0000000000000000000000000000000f000000080000000e00000001>]
[32<0000090000000600000000000000000000000000000007000000080000000e00>]
[32<00010000000200000004000000040000000000000000000000000000000f0000>]
[32<0900000006000000090000000600000000000000000000000000000006000000>]]
[%ug
[8<0000020000000000>]
[8<0000020000002000>]
[8<0004020000002000>]
[8<0004020000402000>]
[8<0004060000402000>]
[8<0004060000406000>]
[8<0006060000406000>]
[8<0006060000606000>]
[8<00060e0000606000>]
[8<00060e000060e000>]
[8<00070e000060e000>]
[8<00070e000070e000>]
[8<00070e020070e000>]
[8<00070e020070e020>]
[8<04070e020070e020>]
[8<04070e024070e020>]
[8<04070e064070e020>]
[8<04070e064070e060>]
[8<06070e064070e060>]
[8<06070e066070e060>]
[8<06070f066070e060>]
[8<06070f066070f060>]
[8<060f0f066070f060>]
[8<060f0f0660f0f060>]
[8<060f0f0760f0f060>]
[8<060f0f0760f0f070>]
[8<0e0f0f0760f0f070>]
[8<0e0f0f07e0f0f070>]
[8<0e0f0f0fe0f0f070>]
[8<0e0f0f0fe0f0f0f0>]
[8<0f0f0f0fe0f0f0f0>]
[8<0f0f0f0ff0f0f0f0>]
[8<1f0f0f0ff0f0f0f0>]
[8<1f0f0f0ff1f0f0f0>]
[8<1f0f0f8ff1f0f0f0>]
[8<1f0f0f8ff1f0f0f8>]
[8<9f0f0f8ff1f0f0f8>]
[8<9f0f0f8ff9f0f0f8>]
[8<9f0f0f9ff9f0f0f8>]
[8<9f0f0f9ff9f0f0f9>]
[8<9f8f0f9ff9f0f0f9>]
[8<9f8f0f9ff9f8f0f9>]
[8<9f8f1f9ff9f8f0f9>]
[8<9f8f1f9ff9f8f1f9>]
[8<bf8f1f9ff9f8f1f9>]
[8<bf8f1f9ffbf8f1f9>]
[8<bf8f1fdffbf8f1f9>]
[8<bf8f1fdffbf8f1fd>]
[8<ff8f1fdffbf8f1fd>]
[8<ff8f1fdffff8f1fd>]
[8<ff8f1ffffff8f1fd>]
[8<ff8f1ffffff8f1ff>]
[8<ff9f1ffffff8f1ff>]
[8<ff9f1ffffff9f1ff>]
[8<ff9f9ffffff9f1ff>]
[8<ff9f9ffffff9f9ff>]
[8<ffbf9ffffff9f9ff>]
[8<ffbf9ffffffbf9ff>]
[8<ffbfdffffffbf9ff>]
[8<ffbfdffffffbfdff>]
[8<ffffdffffffbfdff>]
[8<ffffdffffffffdff>]
[8<fffffffffffffdff>]
[8<ffffffffffffffff>]]
[%mg
[8<8000000000000000>]
[8<0822080080228000>]
[8<0204081020408001>]
[8<40e0400000000000>]
[8<66999966>]
[8<8001000010080000>]
[8<81c36666c3810000>]
[8<f0e0c08000000000>]
[16<07c00f801f003e007c00f800f001e003c007800f001f003e007c00f801f003e0>]
[16<1f000f8007c003e001f000f8007c003e001f800fc007e003f001f8007c003e00>]
[8<c3c300000000c3c3>]
[16<0040008001000200040008001000200040008000000100020004000800100020>]
[16<0040002000100008000400020001800040002000100008000400020001000080>]
[16<1fc03fe07df0f8f8f07de03fc01f800fc01fe03ff07df8f87df03fe01fc00f80>]
[8<80>]
[8<8040201000000000>]
[8<84cc000048cc0000>]
[8<9900009900000000>]
[8<08040201804020100800020180002010>]
[8<2418814242811824>]
[8<66999966>]
[8<8000000008000000>]
[8<70f8d8f870000000>]
[8<0814224180402010>]
[8<aa00440a11a04400>]
[8<018245aa45820100>]
[8<221c224180808041>]
[8<88000000>]
[8<0855800080550800>]
[8<2844004482440044>]
[8<0810204080412214>]
[8<00>]]]def
/Dfill{
 transform /maxy exch def /maxx exch def
 transform /miny exch def /minx exch def
 minx maxx gt{/minx maxx /maxx minx def def}if
 miny maxy gt{/miny maxy /maxy miny def def}if
 Dpatterns Dstipple 1 sub get exch 1 sub get
 aload pop /stip exch def /stipw exch def /stiph 128 def
 /imatrix[stipw 0 0 stiph 0 0]def
 /tmatrix[stipw 0 0 stiph 0 0]def
 /minx minx cvi stiph idiv stiph mul def
 /miny miny cvi stipw idiv stipw mul def
 gsave eoclip 0 setgray
 miny stiph maxy{
  tmatrix exch 5 exch put
  minx stipw maxx{
   tmatrix exch 4 exch put tmatrix setmatrix
   stipw stiph true imatrix {stip} imagemask
  }for
 }for
 grestore
}def
/Dp{Dfill Dstroke}def
/DP{Dfill currentpoint newpath moveto}def
end

/ditstart{$DITroff begin
 /nfonts 60 def                        % NFONTS makedev/ditroff dependent!
 /fonts[nfonts{0}repeat]def
 /fontnames[nfonts{()}repeat]def
/docsave save def
}def

% character outcalls
/oc{
 /pswid exch def /cc exch def /name exch def
 /ditwid pswid fontsize mul resolution mul 72000 div def
 /ditsiz fontsize resolution mul 72 div def
 ocprocs name known{ocprocs name get exec}{name cb}ifelse
}def
/fractm [.65 0 0 .6 0 0] def
/fraction{
 /fden exch def /fnum exch def gsave /cf currentfont def
 cf fractm makefont setfont 0 .3 dm 2 copy neg rmoveto
 fnum show rmoveto currentfont cf setfont(\244)show setfont fden show 
 grestore ditwid 0 rmoveto
}def
/oce{grestore ditwid 0 rmoveto}def
/dm{ditsiz mul}def
/ocprocs 50 dict def ocprocs begin
(14){(1)(4)fraction}def
(12){(1)(2)fraction}def
(34){(3)(4)fraction}def
(13){(1)(3)fraction}def
(23){(2)(3)fraction}def
(18){(1)(8)fraction}def
(38){(3)(8)fraction}def
(58){(5)(8)fraction}def
(78){(7)(8)fraction}def
(sr){gsave 0 .06 dm rmoveto(\326)show oce}def
(is){gsave 0 .15 dm rmoveto(\362)show oce}def
(->){gsave 0 .02 dm rmoveto(\256)show oce}def
(<-){gsave 0 .02 dm rmoveto(\254)show oce}def
(==){gsave 0 .05 dm rmoveto(\272)show oce}def
(uc){gsave currentpoint 400 .009 dm mul add translate
     8 -8 scale ucseal oce}def
end

% an attempt at a PostScript FONT to implement ditroff special chars
% this will enable us to 
%      cache the little buggers
%      generate faster, more compact PS out of psdit
%      confuse everyone (including myself)!
50 dict dup begin
/FontType 3 def
/FontName /DIThacks def
/FontMatrix [.001 0 0 .001 0 0] def
/FontBBox [-260 -260 900 900] def% a lie but ...
/Encoding 256 array def
0 1 255{Encoding exch /.notdef put}for
Encoding
 dup 8#040/space put %space
 dup 8#110/rc put %right ceil
 dup 8#111/lt put %left  top curl
 dup 8#112/bv put %bold vert
 dup 8#113/lk put %left  mid curl
 dup 8#114/lb put %left  bot curl
 dup 8#115/rt put %right top curl
 dup 8#116/rk put %right mid curl
 dup 8#117/rb put %right bot curl
 dup 8#120/rf put %right floor
 dup 8#121/lf put %left  floor
 dup 8#122/lc put %left  ceil
 dup 8#140/sq put %square
 dup 8#141/bx put %box
 dup 8#142/ci put %circle
 dup 8#143/br put %box rule
 dup 8#144/rn put %root extender
 dup 8#145/vr put %vertical rule
 dup 8#146/ob put %outline bullet
 dup 8#147/bu put %bullet
 dup 8#150/ru put %rule
 dup 8#151/ul put %underline
 pop
/DITfd 100 dict def
/BuildChar{0 begin
 /cc exch def /fd exch def
 /charname fd /Encoding get cc get def
 /charwid fd /Metrics get charname get def
 /charproc fd /CharProcs get charname get def
 charwid 0 fd /FontBBox get aload pop setcachedevice
 2 setlinejoin 40 setlinewidth
 newpath 0 0 moveto gsave charproc grestore
 end}def
/BuildChar load 0 DITfd put
/CharProcs 50 dict def
CharProcs begin
/space{}def
/.notdef{}def
/ru{500 0 rls}def
/rn{0 840 moveto 500 0 rls}def
/vr{0 800 moveto 0 -770 rls}def
/bv{0 800 moveto 0 -1000 rls}def
/br{0 840 moveto 0 -1000 rls}def
/ul{0 -140 moveto 500 0 rls}def
/ob{200 250 rmoveto currentpoint newpath 200 0 360 arc closepath stroke}def
/bu{200 250 rmoveto currentpoint newpath 200 0 360 arc closepath fill}def
/sq{80 0 rmoveto currentpoint dround newpath moveto
    640 0 rlineto 0 640 rlineto -640 0 rlineto closepath stroke}def
/bx{80 0 rmoveto currentpoint dround newpath moveto
    640 0 rlineto 0 640 rlineto -640 0 rlineto closepath fill}def
/ci{500 360 rmoveto currentpoint newpath 333 0 360 arc
    50 setlinewidth stroke}def

/lt{0 -200 moveto 0 550 rlineto currx 800 2cx s4 add exch s4 a4p stroke}def
/lb{0 800 moveto 0 -550 rlineto currx -200 2cx s4 add exch s4 a4p stroke}def
/rt{0 -200 moveto 0 550 rlineto currx 800 2cx s4 sub exch s4 a4p stroke}def
/rb{0 800 moveto 0 -500 rlineto currx -200 2cx s4 sub exch s4 a4p stroke}def
/lk{0 800 moveto 0 300 -300 300 s4 arcto pop pop 1000 sub
    0 300 4 2 roll s4 a4p 0 -200 lineto stroke}def
/rk{0 800 moveto 0 300 s2 300 s4 arcto pop pop 1000 sub
    0 300 4 2 roll s4 a4p 0 -200 lineto stroke}def
/lf{0 800 moveto 0 -1000 rlineto s4 0 rls}def
/rf{0 800 moveto 0 -1000 rlineto s4 neg 0 rls}def
/lc{0 -200 moveto 0 1000 rlineto s4 0 rls}def
/rc{0 -200 moveto 0 1000 rlineto s4 neg 0 rls}def
end

/Metrics 50 dict def Metrics begin
/.notdef 0 def
/space 500 def
/ru 500 def
/br 0 def
/lt 416 def
/lb 416 def
/rt 416 def
/rb 416 def
/lk 416 def
/rk 416 def
/rc 416 def
/lc 416 def
/rf 416 def
/lf 416 def
/bv 416 def
/ob 350 def
/bu 350 def
/ci 750 def
/bx 750 def
/sq 750 def
/rn 500 def
/ul 500 def
/vr 0 def
end

DITfd begin
/s2 500 def /s4 250 def /s3 333 def
/a4p{arcto pop pop pop pop}def
/2cx{2 copy exch}def
/rls{rlineto stroke}def
/currx{currentpoint pop}def
/dround{transform round exch round exch itransform} def
end
end
/DIThacks exch definefont pop
ditstart
(psc)xT
576 1 1 xr
1(Times-Roman)xf 1 f
2(Times-Italic)xf 2 f
3(Times-Bold)xf 3 f
4(Times-BoldItalic)xf 4 f
5(Helvetica)xf 5 f
6(Helvetica-Bold)xf 6 f
7(Courier)xf 7 f
8(Courier-Bold)xf 8 f
9(Symbol)xf 9 f
10(DIThacks)xf 10 f
10 s
1 f
xi
%%EndProlog

%%Page: 0 1
10 s 10 xH 0 xS 1 f
3 f
12 s
1846 960(PMake)N
2164(\320)X
2284(A)X
2377(Tutorial)X
2 f
10 s
2051 1152(Adam)N
2258(de)X
2354(Boor)X
1 f
1963 1296(Berkeley)N
2273(Softworks)X
1791 1392(2150)N
1971(Shattuck)X
2271(Ave,)X
2445(Penthouse)X
1952 1488(Berkeley,)N
2282(CA)X
2413(94704)X
1985 1584(adam@bsw.uu.net)N
1972 1680(...!uunet!bsw!adam)N
555 2352(January)N
825(8,)X
905(1989)X
10 f
555 5280(h)N
571(hhhhhhhhhhhhhh)X
8 s
1 f
667 5374(Permission)N
970(to)X
1038(use,)X
1157(copy,)X
1315(modify,)X
1534(and)X
1644(distribute)X
1904(this)X
2015(software)X
2252(and)X
2362(its)X
2441(documentation)X
2839(for)X
2931(any)X
3041(purpose)X
3261(and)X
3371(without)X
3585(fee)X
3680(is)X
555 5454(hereby)N
745(granted,)X
969(provided)X
1213(that)X
1326(the)X
1420(above)X
1588(copyright)X
1849(notice)X
2021(appears)X
2231(in)X
2297(all)X
2377(copies.)X
2588(The)X
2703(University)X
2989(of)X
3058(California,)X
3349(Berkeley)X
3595(Soft-)X
555 5534(works,)N
745(and)X
855(Adam)X
1028(de)X
1105(Boor)X
1250(make)X
1405(no)X
1486(representations)X
1889(about)X
2048(the)X
2143(suitability)X
2417(of)X
2487(this)X
2597(software)X
2833(for)X
2924(any)X
3033(purpose.)X
3284(It)X
3340(is)X
3400(provided)X
3644("as)X
555 5614(is")N
640(without)X
852(express)X
1059(or)X
1128(implied)X
1340(warranty.)X

1 p
%%Page: 1 2
8 s 8 xH 0 xS 1 f
10 s
3 f
12 s
1846 984(PMake)N
2164(\320)X
2284(A)X
2377(Tutorial)X
2 f
10 s
2051 1176(Adam)N
2258(de)X
2354(Boor)X
1 f
1963 1320(Berkeley)N
2273(Softworks)X
1791 1416(2150)N
1971(Shattuck)X
2271(Ave,)X
2445(Penthouse)X
1952 1512(Berkeley,)N
2282(CA)X
2413(94704)X
1985 1608(adam@bsw.uu.net)N
1972 1704(...!uunet!bsw!adam)N
3 f
555 2020(1.)N
655(Introduction)X
1 f
755 2144(PMake)N
1006(is)X
1083(a)X
1143(program)X
1439(for)X
1557(creating)X
1840(other)X
2029(programs,)X
2376(or)X
2467(anything)X
2771(else)X
2919(you)X
3062(can)X
3197(think)X
3384(of)X
3474(for)X
3591(it)X
3658(to)X
3743(do.)X
3886(The)X
555 2240(basic)N
747(idea)X
908(behind)X
1153(PMake)X
1407(is)X
1487(that,)X
1653(for)X
1773(any)X
1915(given)X
2119(system,)X
2387(be)X
2489(it)X
2559(a)X
2621(program)X
2919(or)X
3012(a)X
3074(document)X
3416(or)X
3509(whatever,)X
3850(there)X
555 2336(will)N
706(be)X
809(some)X
1005(\256les)X
1165(that)X
1312(depend)X
1571(on)X
1678(the)X
1803(state)X
1976(of)X
2069(other)X
2260(\256les)X
2419(\(on)X
2552(when)X
2752(they)X
2916(were)X
3099(last)X
3236(modi\256ed\).)X
3593(PMake)X
3846(takes)X
555 2432(these)N
740(dependencies,)X
1213(which)X
1429(you)X
1569(must)X
1744(specify,)X
2016(and)X
2152(uses)X
2310(them)X
2490(to)X
2572(build)X
2756(whatever)X
3071(it)X
3135(is)X
3208(you)X
3348(want)X
3524(it)X
3588(to)X
3670(build.)X
755 2556(PMake)N
1014(is)X
1099(almost)X
1344(fully-compatible)X
1910(with)X
2084(Make,)X
2319(with)X
2493(which)X
2721(you)X
2873(may)X
3043(already)X
3312(be)X
3420(familiar.)X
3726(PMake's)X
555 2652(most)N
739(important)X
1079(feature)X
1332(is)X
1414(its)X
1517(ability)X
1749(to)X
1839(run)X
1974(several)X
2230(different)X
2535(jobs)X
2696(at)X
2782(once,)X
2982(making)X
3250(the)X
3376(creation)X
3663(of)X
3758(systems)X
555 2748(considerably)N
991(faster.)X
1216(It)X
1291(also)X
1446(has)X
1579(a)X
1641(great)X
1828(deal)X
1988(more)X
2179(functionality)X
2614(than)X
2778(Make.)X
3007(Throughout)X
3410(the)X
3533(text,)X
3698(whenever)X
555 2844(something)N
910(is)X
985(mentioned)X
1345(that)X
1487(is)X
1562(an)X
1660(important)X
1993(difference)X
2342(between)X
2632(PMake)X
2881(and)X
3019(Make)X
3224(\(i.e.)X
3391(something)X
3746(that)X
3887(will)X
555 2940(cause)N
756(a)X
814(make\256le)X
1112(to)X
1196(fail)X
1325(if)X
1396(you)X
1538(don't)X
1729(do)X
1831(something)X
2185(about)X
2384(it\),)X
2496(or)X
2584(is)X
2658(simply)X
2896(important,)X
3248(it)X
3313(will)X
3458(be)X
3555(\257agged)X
3812(with)X
3975(a)X
555 3036(little)N
721(sign)X
874(in)X
956(the)X
1074(left)X
1201(margin,)X
1468(like)X
1608(this:)X
-1 Ds
5 Dt
317 MX
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
1 Di
320 3043 MXY
 320 3043 lineto
 388 3043 lineto
 434 3089 lineto
 434 3157 lineto
 388 3203 lineto
 320 3203 lineto
 273 3157 lineto
 273 3089 lineto
 320 3043 lineto
closepath 14 273 3043 434 3203 Dp
6 s
288 3137(NOTE)N
3 Dt
-1 Ds
10 s
755 3160(This)N
925(tutorial)X
1184(is)X
1265(divided)X
1533(into)X
1685(three)X
1874(main)X
2062(sections)X
2348(corresponding)X
2835(to)X
2924(basic,)X
3136(intermediate)X
3564(and)X
3707(advanced)X
555 3256(PMake)N
816(usage.)X
1053(If)X
1141(you)X
1295(already)X
1566(know)X
1778(Make)X
1995(well,)X
2187(you)X
2341(will)X
2499(only)X
2675(need)X
2861(to)X
2957(skim)X
3146(chapter)X
3416(2)X
3489(\(there)X
3710(are)X
3842(some)X
555 3352(aspects)N
812(of)X
904(PMake)X
1156(that)X
1301(I)X
1353(consider)X
1650(basic)X
1839(to)X
1925(its)X
2024(use)X
2155(that)X
2299(didn't)X
2514(exist)X
2689(in)X
2775(Make\).)X
3049(Things)X
3295(in)X
3381(chapter)X
3642(3)X
3706(make)X
3904(life)X
555 3448(much)N
759(easier,)X
993(while)X
1197(those)X
1392(in)X
1480(chapter)X
1743(4)X
1809(are)X
1934(strictly)X
2182(for)X
2302(those)X
2497(who)X
2661(know)X
2865(what)X
3046(they)X
3209(are)X
3333(doing.)X
3560(Chapter)X
3839(5)X
3904(has)X
555 3544(de\256nitions)N
933(for)X
1068(the)X
1207(jargon)X
1453(I)X
1521(use)X
1669(and)X
1826(chapter)X
2104(6)X
2185(contains)X
2493(possible)X
2796(solutions)X
3125(to)X
3227(the)X
3365(problems)X
3703(presented)X
555 3640(throughout)N
926(the)X
1044(tutorial.)X
3 f
555 3832(2.)N
655(The)X
808(Basics)X
1041(of)X
1128(PMake)X
1 f
755 3956(PMake)N
1004(takes)X
1191(as)X
1280(input)X
1466(a)X
1524(\256le)X
1648(that)X
1790(tells)X
1945(a\))X
2030(which)X
2247(\256les)X
2401(depend)X
2654(on)X
2755(which)X
2972(other)X
3158(\256les)X
3312(to)X
3395(be)X
3492(complete)X
3807(and)X
3944(b\))X
555 4052(what)N
734(to)X
819(do)X
922(about)X
1122(\256les)X
1277(that)X
1419(are)X
1540 0.2083(``out-of-date.'')AX
2047(This)X
2211(\256le)X
2335(is)X
2410(known)X
2650(as)X
2739(a)X
2797(``make\256le'')X
3203(and)X
3341(is)X
3416(usually)X
3669(kept)X
3829(in)X
3913(the)X
555 4148(top-most)N
867(directory)X
1185(of)X
1280(the)X
1406(system)X
1656(to)X
1746(be)X
1850(built.)X
2044(While)X
2268(you)X
2416(can)X
2556(call)X
2699(the)X
2824(make\256le)X
3127(anything)X
3434(you)X
3581(want,)X
3784(PMake)X
555 4244(will)N
705(look)X
873(for)X
7 f
992(Makefile)X
1 f
1401(and)X
7 f
1542(makefile)X
1 f
1951(\(in)X
2065(that)X
2210(order\))X
2432(in)X
2519(the)X
2642(current)X
2895(directory)X
3210(if)X
3284(you)X
3429(don't)X
3623(tell)X
3750(it)X
3819(other-)X
555 4340(wise.)N
762(To)X
871(specify)X
1123(a)X
1179(different)X
1476(make\256le,)X
1792(use)X
1919(the)X
3 f
9 f
2037(-)X
3 f
2081(f)X
1 f
2128(\257ag)X
2268(\(e.g.)X
2451(``)X
7 f
2505(pmake)X
2793(-f)X
2937(program.mk)X
1 f
(''\).)S
755 4464(A)N
833(make\256le)X
1129(has)X
1256(four)X
1410(different)X
1707(types)X
1896(of)X
1983(lines)X
2154(in)X
2236(it:)X
10 f
755 4588(g)N
1 f
835(File)X
979(dependency)X
1383(speci\256cations)X
10 f
755 4712(g)N
1 f
835(Creation)X
1131(commands)X
10 f
755 4836(g)N
1 f
835(Variable)X
1132(assignments)X
10 f
755 4960(g)N
1 f
835(Comments,)X
1221(include)X
1477(statements)X
1835(and)X
1971(conditional)X
2351(directives)X
555 5084(Any)N
723(line)X
873(may)X
1041(be)X
1147(continued)X
1492(over)X
1664(multiple)X
1959(lines)X
2139(by)X
2248(ending)X
2495(it)X
2568(with)X
2739(a)X
2804(backslash.)X
3185(The)X
3339(backslash,)X
3700(following)X
555 5180(newline)N
830(and)X
967(any)X
1103(initial)X
1309(whitespace)X
1686(on)X
1786(the)X
1904(following)X
2235(line)X
2375(are)X
2494(compressed)X
2893(into)X
3037(a)X
3093(single)X
3304(space)X
3503(before)X
3729(the)X
3847(input)X
555 5276(line)N
695(is)X
768(examined)X
1100(by)X
1200(PMake.)X
8 s
10 f
555 5356(hhhhhhhhhhhhhhhhhh)N
1 f
667 5516(Permission)N
970(to)X
1038(use,)X
1157(copy,)X
1315(modify,)X
1534(and)X
1644(distribute)X
1904(this)X
2015(software)X
2252(and)X
2362(its)X
2441(documentation)X
2839(for)X
2931(any)X
3041(purpose)X
3261(and)X
3371(without)X
3585(fee)X
3680(is)X
555 5596(hereby)N
745(granted,)X
969(provided)X
1213(that)X
1326(the)X
1420(above)X
1588(copyright)X
1849(notice)X
2021(appears)X
2231(in)X
2297(all)X
2377(copies.)X
2588(The)X
2703(University)X
2989(of)X
3058(California,)X
3349(Berkeley)X
3595(Soft-)X
555 5676(works,)N
745(and)X
855(Adam)X
1028(de)X
1105(Boor)X
1250(make)X
1405(no)X
1486(representations)X
1889(about)X
2048(the)X
2143(suitability)X
2417(of)X
2487(this)X
2597(software)X
2833(for)X
2924(any)X
3033(purpose.)X
3284(It)X
3340(is)X
3400(provided)X
3644("as)X
555 5756(is")N
640(without)X
852(express)X
1059(or)X
1128(implied)X
1340(warranty.)X

2 p
%%Page: 2 3
8 s 8 xH 0 xS 1 f
10 s
2216 384(-)N
2263(2)X
2323(-)X
3 f
555 672(2.1.)N
715(Dependency)X
1153(Lines)X
1 f
755 796(As)N
868(mentioned)X
1230(in)X
1316(the)X
1438(introduction,)X
1873(in)X
1959(any)X
2099(system,)X
2364(there)X
2548(are)X
2670(dependencies)X
3126(between)X
3417(the)X
3538(\256les)X
3694(that)X
3837(make)X
555 892(up)N
657(the)X
777(system.)X
1061(For)X
1194(instance,)X
1499(in)X
1583(a)X
1641(program)X
1935(made)X
2131(up)X
2233(of)X
2322(several)X
2572(C)X
2647(source)X
2878(\256les)X
3032(and)X
3169(one)X
3306(header)X
3542(\256le,)X
3685(the)X
3804(C)X
3878(\256les)X
555 988(will)N
702(need)X
877(to)X
962(be)X
1061(re-compiled)X
1472(should)X
1708(the)X
1829(header)X
2067(\256le)X
2191(be)X
2289(changed.)X
2599(For)X
2732(a)X
2790(document)X
3128(of)X
3217(several)X
3467(chapters)X
3757(and)X
3895(one)X
555 1084(macro)N
779(\256le,)X
924(the)X
1045(chapters)X
1336(will)X
1483(need)X
1658(to)X
1743(be)X
1842(reprocessed)X
2245(if)X
2317(any)X
2456(of)X
2546(the)X
2667(macros)X
2922(changes.)X
3243(These)X
3457(are)X
3578(dependencies)X
555 1180(and)N
691(are)X
810(speci\256ed)X
1115(by)X
1215(means)X
1440(of)X
1527(dependency)X
1931(lines)X
2102(in)X
2184(the)X
2302(make\256le.)X
755 1304(On)N
879(a)X
941(dependency)X
1351(line,)X
1517(there)X
1704(are)X
1829(targets)X
2069(and)X
2211(sources,)X
2498(separated)X
2828(by)X
2934(a)X
2995(one-)X
3163(or)X
3255 0.2813(two-character)AX
3723(operator.)X
555 1400(The)N
714(targets)X
962(``depend'')X
1336(on)X
1450(the)X
1582(sources)X
1857(and)X
2007(are)X
2140(usually)X
2405(created)X
2672(from)X
2862(them.)X
3096(Any)X
3268(number)X
3547(of)X
3648(targets)X
3895(and)X
555 1496(sources)N
823(may)X
988(be)X
1091(speci\256ed)X
1403(on)X
1510(a)X
1573(dependency)X
1984(line.)X
2171(All)X
2300(the)X
2425(targets)X
2666(in)X
2755(the)X
2880(line)X
3027(are)X
3153(made)X
3354(to)X
3443(depend)X
3701(on)X
3807(all)X
3913(the)X
555 1592(sources.)N
868(Targets)X
1141(and)X
1288(sources)X
1560(need)X
1743(not)X
1876(be)X
1983(actual)X
2206(\256les,)X
2390(but)X
2523(every)X
2733(source)X
2974(must)X
3160(be)X
3267(either)X
3481(an)X
3588(actual)X
3811(\256le)X
3944(or)X
555 1688(another)N
823(target)X
1033(in)X
1122(the)X
1247(make\256le.)X
1590(If)X
1671(you)X
1818(run)X
1952(out)X
2081(of)X
2175(room,)X
2391(use)X
2525(a)X
2588(backslash)X
2927(at)X
3012(the)X
3137(end)X
3280(of)X
3374(the)X
3499(line)X
3646(to)X
3735(continue)X
555 1784(onto)N
717(the)X
835(next)X
993(one.)X
755 1908(Any)N
916(\256le)X
1041(may)X
1202(be)X
1301(a)X
1360(target)X
1566(and)X
1704(any)X
1842(\256le)X
1966(may)X
2126(be)X
2224(a)X
2282(source,)X
2534(but)X
2658(the)X
2778(relationship)X
3178(between)X
3468(the)X
3588(two)X
3730(\(or)X
3846(how-)X
555 2004(ever)N
729(many\))X
969(is)X
1057(determined)X
1453(by)X
1568(the)X
1701(``operator'')X
2112(that)X
2266(separates)X
2595(them.)X
2829(Three)X
3051(types)X
3254(of)X
3355(operators)X
3688(exist:)X
3895(one)X
555 2100(speci\256es)N
852(that)X
993(the)X
1111(datedness)X
1443(of)X
1530(a)X
1586(target)X
1789(is)X
1862(determined)X
2243(by)X
2343(the)X
2461(state)X
2628(of)X
2715(its)X
2810(sources,)X
3091(while)X
3289(another)X
3550(speci\256es)X
3846(other)X
555 2196(\256les)N
713(\(the)X
863(sources\))X
1156(that)X
1301(need)X
1478(to)X
1565(be)X
1666(dealt)X
1847(with)X
2013(before)X
2243(the)X
2365(target)X
2572(can)X
2708(be)X
2808 0.3375(re-created.)AX
3175(The)X
3324(third)X
3499(operator)X
3791(is)X
3868(very)X
555 2292(similar)N
807(to)X
899(the)X
1026(\256rst,)X
1199(with)X
1370(the)X
1497(additional)X
1846(condition)X
2177(that)X
2326(the)X
2453(target)X
2665(is)X
2747(out-of-date)X
3133(if)X
3211(it)X
3284(has)X
3420(no)X
3529(sources.)X
3819(These)X
555 2388(operations)N
914(are)X
1038(represented)X
1434(by)X
1539(the)X
1662(colon,)X
1885(the)X
2007(exclamation)X
2423(point)X
2611(and)X
2751(the)X
2873(double-colon,)X
3340(respectively,)X
3772(and)X
3912(are)X
555 2484(mutually)N
859(exclusive.)X
1202(Their)X
1396(exact)X
1586(semantics)X
1922(are)X
2041(as)X
2128(follows:)X
555 2608(:)N
755(If)X
829(a)X
885(colon)X
1083(is)X
1156(used,)X
1343(a)X
1399(target)X
1602(on)X
1702(the)X
1820(line)X
1960(is)X
2033(considered)X
2401(to)X
2483(be)X
2579 0.2232(``out-of-date'')AX
3064(\(and)X
3227(in)X
3309(need)X
3481(of)X
3568(creation\))X
3874(if)X
10 f
755 2732(g)N
1 f
835(any)X
971(of)X
1058(the)X
1176(sources)X
1437(has)X
1564(been)X
1736(modi\256ed)X
2040(more)X
2225(recently)X
2504(than)X
2662(the)X
2780(target,)X
3003(or)X
10 f
755 2856(g)N
1 f
835(the)X
953(target)X
1156(doesn't)X
1412(exist.)X
755 2980(Under)N
978(this)X
1115(operation,)X
1460(steps)X
1642(will)X
1788(be)X
1886(taken)X
2082(to)X
2166 0.4219(re-create)AX
2470(the)X
2589(target)X
2793(only)X
2956(if)X
3026(it)X
3091(is)X
3165(found)X
3373(to)X
3456(be)X
3553(out-of-date)X
3931(by)X
755 3076(using)N
948(these)X
1133(two)X
1273(rules.)X
555 3200(!)N
755(If)X
832(an)X
931(exclamation)X
1346(point)X
1533(is)X
1609(used,)X
1799(the)X
1919(target)X
2124(will)X
2270(always)X
2515(be)X
2613 0.3375(re-created,)AX
2978(but)X
3102(this)X
3239(will)X
3385(not)X
3509(happen)X
3763(until)X
3931(all)X
755 3296(of)N
842(its)X
937(sources)X
1198(have)X
1370(been)X
1542(examined)X
1874(and)X
2010 0.3375(re-created,)AX
2373(if)X
2442(necessary.)X
555 3420(::)N
755(If)X
829(a)X
885(double-colon)X
1328(is)X
1401(used,)X
1588(a)X
1644(target)X
1847(is)X
1920(out-of-date)X
2297(if:)X
10 f
755 3544(g)N
1 f
835(any)X
971(of)X
1058(the)X
1176(sources)X
1437(has)X
1564(been)X
1736(modi\256ed)X
2040(more)X
2225(recently)X
2504(than)X
2662(the)X
2780(target,)X
3003(or)X
10 f
755 3668(g)N
1 f
835(the)X
953(target)X
1156(doesn't)X
1412(exist,)X
1603(or)X
10 f
755 3792(g)N
1 f
835(the)X
953(target)X
1156(has)X
1283(no)X
1383(sources.)X
755 3916(If)N
841(the)X
971(target)X
1185(is)X
1269(out-of-date)X
1657(according)X
2005(to)X
2098(these)X
2294(rules,)X
2501(it)X
2576(will)X
2731(be)X
2838 0.3375(re-created.)AX
3232(This)X
3405(operator)X
3704(also)X
3864(does)X
755 4012(something)N
1108(else)X
1253(to)X
1335(the)X
1453(targets,)X
1707(but)X
1829(I'll)X
1947(go)X
2047(into)X
2191(that)X
2331(in)X
2413(the)X
2531(next)X
2689(section)X
2936(\(``Shell)X
3201(Commands''\).)X
755 4136(Enough)N
1028(words,)X
1268(now)X
1430(for)X
1548(an)X
1648(example.)X
1964(Take)X
2149(that)X
2293(C)X
2370(program)X
2665(I)X
2715(mentioned)X
3076(earlier.)X
3325(Say)X
3468(there)X
3652(are)X
3774(three)X
3958(C)X
555 4232(\256les)N
716(\()X
7 f
743(a.c)X
1 f
(,)S
7 f
935(b.c)X
1 f
1107(and)X
7 f
1251(c.c)X
1 f
(\))S
1450(each)X
1626(of)X
1720(which)X
1943(includes)X
2237(the)X
2362(\256le)X
7 f
2491(defs.h)X
1 f
(.)S
2846(The)X
2998(dependencies)X
3458(between)X
3753(the)X
3878(\256les)X
555 4328(could)N
753(then)X
911(be)X
1007(expressed)X
1344(as)X
1431(follows:)X
7 f
843 4472(program)N
1611(:)X
1707(a.o)X
1899(b.o)X
2091(c.o)X
843 4568(a.o)N
1035(b.o)X
1227(c.o)X
1611(:)X
1707(defs.h)X
843 4664(a.o)N
1611(:)X
1707(a.c)X
843 4760(b.o)N
1611(:)X
1707(b.c)X
843 4856(c.o)N
1611(:)X
1707(c.c)X
1 f
555 5028(You)N
715(may)X
875(be)X
973(wondering)X
1338(at)X
1418(this)X
1555(point,)X
1761(where)X
7 f
1979(a.o)X
1 f
(,)S
7 f
2164(b.o)X
1 f
2329(and)X
7 f
2466(c.o)X
1 f
2631(came)X
2822(in)X
2905(and)X
3042(why)X
2 f
3201(they)X
1 f
3369(depend)X
3622(on)X
7 f
3723(defs.h)X
1 f
555 5124(and)N
692(the)X
811(C)X
884(\256les)X
1037(don't.)X
1246(The)X
1391(reason)X
1621(is)X
1694(quite)X
1874(simple:)X
7 f
2129(program)X
1 f
2485(cannot)X
2719(be)X
2815(made)X
3009(by)X
3109(linking)X
3355(together)X
3638(.c)X
3714(\256les)X
3867(\320)X
3967(it)X
555 5220(must)N
732(be)X
830(made)X
1026(from)X
1204(.o)X
1286(\256les.)X
1461(Likewise,)X
1797(if)X
1868(you)X
2010(change)X
7 f
2260(defs.h)X
1 f
(,)S
2590(it)X
2656(isn't)X
2820(the)X
2940(.c)X
3018(\256les)X
3173(that)X
3315(need)X
3488(to)X
3571(be)X
3668 0.3375(re-created,)AX
555 5316(it's)N
679(the)X
799(.o)X
881(\256les.)X
1076(If)X
1151(you)X
1292(think)X
1477(of)X
1565(dependencies)X
2019(in)X
2102(these)X
2288(terms)X
2487(\320)X
2588(which)X
2805(\256les)X
2959(\(targets\))X
3248(need)X
3421(to)X
3504(be)X
3601(created)X
3855(from)X
555 5412(which)N
771(\256les)X
924(\(sources\))X
1239(\320)X
1339(you)X
1479(should)X
1712(have)X
1884(no)X
1984(problems.)X
755 5536(An)N
873(important)X
1204(thing)X
1388(to)X
1470(notice)X
1686(about)X
1884(the)X
2002(above)X
2214(example,)X
2526(is)X
2599(that)X
2739(all)X
2839(the)X
2957(.o)X
3037(\256les)X
3190(appear)X
3425(as)X
3512(targets)X
3746(on)X
3846(more)X
555 5632(than)N
714(one)X
851(line.)X
1012(This)X
1175(is)X
1249(perfectly)X
1556(all)X
1657(right:)X
1851(the)X
1970(target)X
2174(is)X
2248(made)X
2442(to)X
2524(depend)X
2776(on)X
2876(all)X
2976(the)X
3094(sources)X
3355(mentioned)X
3713(on)X
3813(all)X
3913(the)X
555 5728(dependency)N
959(lines.)X
1150(E.g.)X
7 f
1319(a.o)X
1 f
1483(depends)X
1766(on)X
1866(both)X
7 f
2028(defs.h)X
1 f
2336(and)X
7 f
2472(a.c)X
1 f
(.)S

3 p
%%Page: 3 4
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(3)X
2323(-)X
-1 Ds
5 Dt
317 672 MXY
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
320 679 MXY
 320 679 lineto
 388 679 lineto
 434 725 lineto
 434 793 lineto
 388 839 lineto
 320 839 lineto
 273 793 lineto
 273 725 lineto
 320 679 lineto
closepath 14 273 679 434 839 Dp
6 s
288 773(NOTE)N
3 Dt
-1 Ds
10 s
755 700(The)N
901(order)X
1092(of)X
1180(the)X
1299(dependency)X
1704(lines)X
1876(in)X
1959(the)X
2078(make\256le)X
2374(is)X
2447(important:)X
2800(the)X
2918(\256rst)X
3062(target)X
3265(on)X
3365(the)X
3483(\256rst)X
3627(dependency)X
555 796(line)N
696(in)X
779(the)X
897(make\256le)X
1193(will)X
1337(be)X
1433(the)X
1551(one)X
1687(that)X
1827(gets)X
1976(made)X
2170(if)X
2239(you)X
2379(don't)X
2568(say)X
2695(otherwise.)X
3067(That's)X
3292(why)X
7 f
3450(program)X
1 f
3806(comes)X
555 892(\256rst)N
699(in)X
781(the)X
899(example)X
1191(make\256le,)X
1507(above.)X
755 1016(Both)N
931(targets)X
1166(and)X
1303(sources)X
1565(may)X
1724(contain)X
1981(the)X
2100(standard)X
2393(C-Shell)X
2658(wildcard)X
2960(characters)X
3308(\()X
7 f
3335({)X
1 f
(,)S
7 f
3424(})X
1 f
(,)S
7 f
3513(*)X
1 f
(,)S
7 f
3602(?)X
1 f
(,)S
7 f
3691([)X
1 f
(,)S
3780(and)X
7 f
3916(])X
1 f
(\),)S
555 1112(but)N
689(the)X
819 0.2321(non-curly-brace)AX
1365(ones)X
1544(may)X
1714(only)X
1888(appear)X
2135(in)X
2229(the)X
2359(\256nal)X
2533(component)X
2921(\(the)X
3078(\256le)X
3212(portion\))X
3502(of)X
3601(the)X
3730(target)X
3944(or)X
555 1208(source.)N
805(The)X
950(characters)X
1297(mean)X
1491(the)X
1609(following)X
1940(things:)X
3 f
555 1332({})N
1 f
755(These)X
970(enclose)X
1234(a)X
1293(comma-separated)X
1883(list)X
2003(of)X
2093(options)X
2351(and)X
2490(cause)X
2692(the)X
2813(pattern)X
3059(to)X
3144(be)X
3243(expanded)X
3573(once)X
3747(for)X
3863(each)X
755 1428(element)N
1098(of)X
1254(the)X
1441(list.)X
1647(Each)X
1897(expansion)X
2311(contains)X
2667(a)X
2792(different)X
3158(element.)X
3520(For)X
3719(example,)X
7 f
755 1524(src/{whiffle,beep,fish}.c)N
1 f
2061(expands)X
2430(to)X
2597(the)X
2800(three)X
3066(words)X
7 f
3367(src/whiffle.c)X
1 f
(,)S
7 f
755 1620(src/beep.c)N
1 f
(,)S
1291(and)X
7 f
1443(src/fish.c)X
1 f
(.)S
1999(These)X
2226(braces)X
2467(may)X
2640(be)X
2751(nested)X
2991(and,)X
3162(unlike)X
3397(the)X
3530(other)X
3730(wildcard)X
755 1716(characters,)N
1129(the)X
1254(resulting)X
1561(words)X
1784(need)X
1963(not)X
2092(be)X
2195(actual)X
2414(\256les.)X
2594(All)X
2723(other)X
2915(wildcard)X
3223(characters)X
3577(are)X
3703(expanded)X
755 1812(using)N
948(the)X
1066(\256les)X
1219(that)X
1359(exist)X
1530(when)X
1724(PMake)X
1971(is)X
2044(started.)X
3 f
555 1936(*)N
1 f
755(This)X
920(matches)X
1206(zero)X
1368(or)X
1458(more)X
1646(characters)X
1996(of)X
2086(any)X
2225(sort.)X
7 f
2388(src/*.c)X
1 f
2747(will)X
2894(expand)X
3149(to)X
3234(the)X
3355(same)X
3543(three)X
3726(words)X
3944(as)X
755 2032(above)N
967(as)X
1054(long)X
1216(as)X
7 f
1303(src)X
1 f
1467(contains)X
1754(those)X
1943(three)X
2124(\256les)X
2277(\(and)X
2440(no)X
2540(other)X
2725(\256les)X
2878(that)X
3018(end)X
3154(in)X
7 f
3236(.c)X
1 f
(\).)S
3 f
555 2156(?)N
1 f
755(Matches)X
1047(any)X
1183(single)X
1394(character.)X
3 f
555 2280([])N
1 f
755(This)X
918(is)X
992(known)X
1231(as)X
1319(a)X
1376(character)X
1693(class)X
1870(and)X
2007(contains)X
2295(either)X
2499(a)X
2556(list)X
2674(of)X
2762(single)X
2973(characters,)X
3340(or)X
3427(a)X
3483(series)X
3686(of)X
3773(charac-)X
755 2376(ter)N
866(ranges)X
1102(\()X
7 f
1129(a-z)X
1 f
(,)S
1319(for)X
1439(example)X
1737(means)X
1968(all)X
2074(characters)X
2427(between)X
2721(a)X
2783(and)X
2925(z\),)X
3034(or)X
3127(both.)X
3315(It)X
3390(matches)X
3679(any)X
3820(single)X
755 2472(character)N
1078(contained)X
1417(in)X
1506(the)X
1631(list.)X
1775(E.g.)X
7 f
1951([A-Za-z])X
1 f
2361(will)X
2511(match)X
2733(all)X
2839(letters,)X
3081(while)X
7 f
3285([0123456789])X
1 f
3887(will)X
755 2568(match)N
971(all)X
1071(numbers.)X
3 f
555 2760(2.2.)N
715(Shell)X
903(Commands)X
1 f
755 2884(``Isn't)N
984(that)X
1132(nice,'')X
1368(you)X
1516(say)X
1651(to)X
1741(yourself,)X
2052(``but)X
2236(how)X
2402(are)X
2529(\256les)X
2690(actually)X
2972 0.3438(`re-created,')AX
3396(as)X
3490(he)X
3593(likes)X
3771(to)X
3860(spell)X
555 2980(it?'')N
710(The)X
856(re-creation)X
1226(is)X
1300(accomplished)X
1762(by)X
1863(commands)X
2231(you)X
2372(place)X
2563(in)X
2645(the)X
2763(make\256le.)X
3099(These)X
3311(commands)X
3678(are)X
3797(passed)X
555 3076(to)N
640(the)X
761(Bourne)X
1020(shell)X
1194(\(better)X
1427(known)X
1668(as)X
1758(``/bin/sh''\))X
2133(to)X
2218(be)X
2317(executed)X
2625(and)X
2763(are)X
2884(expected)X
3192(to)X
3276(do)X
3378(what's)X
3614(necessary)X
3949(to)X
555 3172(update)N
798(the)X
925(target)X
1137(\256le)X
1268(\(PMake)X
1551(doesn't)X
1816(actually)X
2099(check)X
2316(to)X
2407(see)X
2539(if)X
2617(the)X
2744(target)X
2956(was)X
3110(created.)X
3392(It)X
3470(just)X
3614(assumes)X
3909(it's)X
555 3268(there\).)N
755 3392(Shell)N
941(commands)X
1310(in)X
1394(a)X
1452(make\256le)X
1750(look)X
1914(a)X
1972(lot)X
2078(like)X
2220(shell)X
2393(commands)X
2762(you)X
2904(would)X
3126(type)X
3286(at)X
3366(a)X
3424(terminal,)X
3732(with)X
3895(one)X
555 3488(important)N
886(exception:)X
1240(each)X
1408(command)X
1744(in)X
1826(a)X
1882(make\256le)X
2 f
2178(must)X
1 f
2362(be)X
2458(preceded)X
2769(by)X
2869(at)X
2947(least)X
3114(one)X
3250(tab.)X
755 3612(Each)N
936(target)X
1139(has)X
1266(associated)X
1616(with)X
1778(it)X
1842(a)X
1898(shell)X
2069(script)X
2267(made)X
2461(up)X
2561(of)X
2648(one)X
2784(or)X
2871(more)X
3056(of)X
3143(these)X
3328(shell)X
3499(commands.)X
3886(The)X
555 3708(creation)N
838(script)X
1040(for)X
1158(a)X
1218(target)X
1425(should)X
1662(immediately)X
2086(follow)X
2318(the)X
2439(dependency)X
2846(line)X
2989(for)X
3106(that)X
3249(target.)X
3475(While)X
3694(any)X
3833(given)X
555 3804(target)N
760(may)X
920(appear)X
1157(on)X
1259(more)X
1446(than)X
1606(one)X
1744(dependency)X
2150(line,)X
2312(only)X
2476(one)X
2614(of)X
2703(these)X
2890(dependency)X
3296(lines)X
3469(may)X
3629(be)X
3726(followed)X
555 3900(by)N
655(a)X
711(creation)X
990(script,)X
1208(unless)X
1428(the)X
1546(`::')X
1664(operator)X
1952(was)X
2097(used)X
2264(on)X
2364(the)X
2482(dependency)X
2886(line.)X
-1 Ds
5 Dt
317 MX
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
320 3907 MXY
 320 3907 lineto
 388 3907 lineto
 434 3953 lineto
 434 4021 lineto
 388 4067 lineto
 320 4067 lineto
 273 4021 lineto
 273 3953 lineto
 320 3907 lineto
closepath 14 273 3907 434 4067 Dp
6 s
288 4001(NOTE)N
3 Dt
-1 Ds
10 s
755 4024(If)N
833(the)X
955(double-colon)X
1402(was)X
1551(used,)X
1742(each)X
1914(dependency)X
2322(line)X
2465(for)X
2582(the)X
2703(target)X
2909(may)X
3070(be)X
3169(followed)X
3477(by)X
3580(a)X
3639(shell)X
3813(script.)X
555 4120(That)N
725(script)X
926(will)X
1073(only)X
1238(be)X
1337(executed)X
1646(if)X
1718(the)X
1838(target)X
2043(on)X
2145(the)X
2265(associated)X
2617(dependency)X
3023(line)X
3165(is)X
3240(out-of-date)X
3619(with)X
3783(respect)X
555 4216(to)N
642(the)X
765(sources)X
1031(on)X
1136(that)X
1280(line,)X
1444(according)X
1785(to)X
1871(the)X
1993(rules)X
2173(I)X
2224(gave)X
2400(earlier.)X
2670(I'll)X
2792(give)X
2954(you)X
3098(a)X
3158(good)X
3342(example)X
3638(of)X
3729(this)X
3868(later)X
555 4312(on.)N
755 4436(To)N
864(expand)X
1116(on)X
1216(the)X
1334(earlier)X
1560(make\256le,)X
1876(you)X
2016(might)X
2222(add)X
2358(commands)X
2725(as)X
2812(follows:)X
7 f
843 4580(program)N
1611(:)X
1707(a.o)X
1899(b.o)X
2091(c.o)X
1227 4676(cc)N
1371(a.o)X
1563(b.o)X
1755(c.o)X
9 f
1947(-)X
7 f
1991(o)X
2087(program)X
843 4772(a.o)N
1035(b.o)X
1227(c.o)X
1611(:)X
1707(defs.h)X
843 4868(a.o)N
1611(:)X
1707(a.c)X
1227 4964(cc)N
9 f
1371(-)X
7 f
1415(c)X
1511(a.c)X
843 5060(b.o)N
1611(:)X
1707(b.c)X
1227 5156(cc)N
9 f
1371(-)X
7 f
1415(c)X
1511(b.c)X
843 5252(c.o)N
1611(:)X
1707(c.c)X
1227 5348(cc)N
9 f
1371(-)X
7 f
1415(c)X
1511(c.c)X
1 f
555 5520(Something)N
925(you)X
1069(should)X
1306(remember)X
1656(when)X
1854(writing)X
2109(a)X
2169(make\256le)X
2469(is,)X
2566(the)X
2688(commands)X
3059(will)X
3206(be)X
3305(executed)X
3614(if)X
3686(the)X
2 f
3807(target)X
1 f
555 5616(on)N
656(the)X
775(dependency)X
1180(line)X
1321(is)X
1395(out-of-date,)X
1793(not)X
1916(the)X
2035(sources.)X
2337(In)X
2425(this)X
2561(example,)X
2874(the)X
2993(command)X
3330(``)X
7 f
3384(cc)X
9 f
3529(-)X
7 f
3573(c)X
3669(a.c)X
1 f
('')S
3887(will)X
555 5712(be)N
661(executed)X
977(if)X
7 f
1056(a.o)X
1 f
1230(is)X
1313(out-of-date.)X
1720(Because)X
2018(of)X
2115(the)X
2243(`:')X
2349(operator,)X
2667(this)X
2812(means)X
3046(that)X
3195(should)X
7 f
3437(a.c)X
2 f
3610(or)X
7 f
3723(defs.h)X
1 f
555 5808(have)N
732(been)X
909(modi\256ed)X
1218(more)X
1408(recently)X
1692(than)X
7 f
1855(a.o)X
1 f
(,)S
2044(the)X
2167(command)X
2508(will)X
2657(be)X
2757(executed)X
3067(\()X
7 f
3094(a.o)X
1 f
3262(will)X
3410(be)X
3510(considered)X
3882(out-)X

4 p
%%Page: 4 5
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(4)X
2323(-)X
555 672(of-date\).)N
755 796(Remember)N
1138(how)X
1307(I)X
1365(said)X
1525(the)X
1654(only)X
1827(difference)X
2185(between)X
2484(a)X
2551(make\256le)X
2858(shell)X
3040(command)X
3387(and)X
3534(a)X
3601(regular)X
3860(shell)X
555 892(command)N
894(was)X
1042(the)X
1163(leading)X
1422(tab?)X
1578(I)X
1627(lied.)X
1789(There)X
1999(is)X
2074(another)X
2337(way)X
2493(in)X
2577(which)X
2795(make\256le)X
3093(commands)X
3462(differ)X
3663(from)X
3841(regu-)X
555 988(lar)N
661(ones.)X
869(The)X
1015(\256rst)X
1160(two)X
1301(characters)X
1649(after)X
1817(the)X
1935(initial)X
2141(whitespace)X
2518(are)X
2637(treated)X
2876(specially.)X
3221(If)X
3295(they)X
3453(are)X
3572(any)X
3708(combina-)X
555 1084(tion)N
699(of)X
786(`@')X
934(and)X
1070(`)X
9 f
1097(-)X
1 f
1141(',)X
1208(they)X
1366(cause)X
1565(PMake)X
1812(to)X
1894(do)X
1994(different)X
2291(things.)X
755 1208(In)N
856(most)X
1045(cases,)X
1268(shell)X
1452(commands)X
1832(are)X
1964(printed)X
2224(before)X
2463(they're)X
2724(actually)X
3011(executed.)X
3350(This)X
3525(is)X
3611(to)X
3706(keep)X
3891(you)X
555 1304(informed)N
879(of)X
976(what's)X
1220(going)X
1432(on.)X
1562(If)X
1646(an)X
1752(`@')X
1910(appears,)X
2206(however,)X
2533(this)X
2678(echoing)X
2962(is)X
3045(suppressed.)X
3447(In)X
3544(the)X
3671(case)X
3839(of)X
3935(an)X
7 f
555 1400(echo)N
1 f
767(command,)X
1123(say)X
1250(``)X
7 f
1304(echo)X
1544(Linking)X
1928(index)X
1 f
(,'')S
2262(it)X
2326(would)X
2546(be)X
2642(rather)X
2850(silly)X
3007(to)X
3089(see)X
7 f
843 1544(echo)N
1083(Linking)X
1467(index)X
843 1640(Linking)N
1227(index)X
1 f
555 1812(so)N
648(PMake)X
897(allows)X
1127(you)X
1268(to)X
1351(place)X
1542(an)X
1639(`@')X
1788(before)X
2015(the)X
2134(command)X
2471(\(``)X
7 f
2552(@echo)X
2841(Linking)X
3226(index)X
1 f
(''\))S
3568(to)X
3651(prevent)X
3913(the)X
555 1908(command)N
891(from)X
1067(being)X
1265(printed.)X
755 2032(The)N
903(other)X
1091(special)X
1337(character)X
1656(is)X
1732(the)X
1853(`)X
9 f
1880(-)X
1 f
1924('.)X
1994(In)X
2084(case)X
2246(you)X
2389(didn't)X
2603(know,)X
2824(shell)X
2998(commands)X
3368(\256nish)X
3568(with)X
3733(a)X
3792(certain)X
555 2128(``exit)N
750(status.'')X
1027(This)X
1190(status)X
1393(is)X
1467(made)X
1662(available)X
1973(by)X
2074(the)X
2193(operating)X
2517(system)X
2760(to)X
2843(whatever)X
3158(program)X
3450(invoked)X
3728(the)X
3846(com-)X
555 2224(mand.)N
775(Normally)X
1104(this)X
1241(status)X
1445(will)X
1591(be)X
1689(0)X
1751(if)X
1822(everything)X
2187(went)X
2365(ok)X
2467(and)X
2605(non-zero)X
2913(if)X
2984(something)X
3339(went)X
3517(wrong.)X
3764(For)X
3896(this)X
555 2320(reason,)N
815(PMake)X
1072(will)X
1225(consider)X
1526(an)X
1631(error)X
1817(to)X
1908(have)X
2089(occurred)X
2400(if)X
2478(one)X
2623(of)X
2719(the)X
2846(shells)X
3057(it)X
3130(invokes)X
3408(returns)X
3660(a)X
3725(non-zero)X
555 2416(status.)N
782(When)X
999(it)X
1068(detects)X
1316(an)X
1417(error,)X
1619(PMake's)X
1929(usual)X
2123(action)X
2344(is)X
2422(to)X
2509(abort)X
2699(whatever)X
3019(it's)X
3146(doing)X
3353(and)X
3494(exit)X
3638(with)X
3804(a)X
3864(non-)X
555 2512(zero)N
717(status)X
922(itself)X
1105(\(any)X
1271(other)X
1459(targets)X
1696(that)X
1839(were)X
2019(being)X
2220(created)X
2476(will)X
2623(continue)X
2922(being)X
3123(made,)X
3340(but)X
3465(nothing)X
3731(new)X
3887(will)X
555 2608(be)N
654(started.)X
911(PMake)X
1161(will)X
1308(exit)X
1451(after)X
1622(the)X
1743(last)X
1877(job)X
2002(\256nishes\).)X
2336(This)X
2500(behavior)X
2803(can)X
2937(be)X
3035(altered,)X
3296(however,)X
3615(by)X
3717(placing)X
3975(a)X
555 2704(`)N
9 f
582(-)X
1 f
626(')X
674(at)X
753(the)X
872(front)X
1049(of)X
1137(a)X
1194(command)X
1531(\(``)X
7 f
9 f
1612(-)X
7 f
1656(mv)X
1801(index)X
2090(index.old)X
1 f
(''\),)S
2644(certain)X
2884(command-line)X
3368(arguments,)X
3742(or)X
3829(doing)X
555 2800(other)N
749(things,)X
992(to)X
1082(be)X
1186(detailed)X
1468(later.)X
1659(In)X
1754(such)X
1929(a)X
1993(case,)X
2180(the)X
2306(non-zero)X
2620(status)X
2830(is)X
2911(simply)X
3156(ignored)X
3429(and)X
3573(PMake)X
3828(keeps)X
555 2896(chugging)N
873(along.)X
-1 Ds
5 Dt
317 MX
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
320 2903 MXY
 320 2903 lineto
 388 2903 lineto
 434 2949 lineto
 434 3017 lineto
 388 3063 lineto
 320 3063 lineto
 273 3017 lineto
 273 2949 lineto
 320 2903 lineto
closepath 14 273 2903 434 3063 Dp
6 s
288 2997(NOTE)N
3 Dt
-1 Ds
10 s
755 3020(Because)N
1052(all)X
1161(the)X
1288(commands)X
1664(are)X
1792(given)X
1999(to)X
2090(a)X
2155(single)X
2375(shell)X
2555(to)X
2646(execute,)X
2941(such)X
3117(things)X
3341(as)X
3437(setting)X
3679(shell)X
3859(vari-)X
555 3116(ables,)N
765(changing)X
1084(directories,)X
1468(etc.,)X
1627(last)X
1763(beyond)X
2024(the)X
2146(command)X
2486(in)X
2572(which)X
2792(they)X
2954(are)X
3077(found.)X
3308(This)X
3474(also)X
3627(allows)X
3860(shell)X
555 3212(compound)N
916(commands)X
1286(\(like)X
7 f
1456(for)X
1 f
1622(loops\))X
1844(to)X
1928(be)X
2026(entered)X
2285(in)X
2369(a)X
2427(natural)X
2672(manner.)X
2975(Since)X
3175(this)X
3312(could)X
3512(cause)X
3713(problems)X
555 3308(for)N
670(some)X
860(make\256les)X
1188(that)X
1329(depend)X
1582(on)X
1683(each)X
1852(command)X
2189(being)X
2388(executed)X
2695(by)X
2795(a)X
2851(single)X
3062(shell,)X
3253(PMake)X
3500(has)X
3627(a)X
3 f
9 f
3683(-)X
3 f
3727(B)X
1 f
3800(\257ag)X
3940(\(it)X
555 3404(stands)N
784(for)X
907 0.1625(backwards-compatible\))AX
1690(that)X
1839(forces)X
2065(each)X
2241(command)X
2585(to)X
2675(be)X
2779(given)X
2985(to)X
3075(a)X
3139(separate)X
3431(shell.)X
3630(It)X
3707(also)X
3864(does)X
555 3500(several)N
803(other)X
988(things,)X
1223(all)X
1323(of)X
1410(which)X
1626(I)X
1673(discourage)X
2041(since)X
2226(they)X
2384(are)X
2503(now)X
2661(old-fashioned.)X
3135(.)X
3168(.)X
3201(.)X
-1 Ds
5 Dt
317 MX
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
320 3507 MXY
 320 3507 lineto
 388 3507 lineto
 434 3553 lineto
 434 3621 lineto
 388 3667 lineto
 320 3667 lineto
 273 3621 lineto
 273 3553 lineto
 320 3507 lineto
closepath 14 273 3507 434 3667 Dp
6 s
288 3601(NOTE)N
3 Dt
-1 Ds
10 s
755 3624(A)N
839(target's)X
1106(shell)X
1283(script)X
1487(is)X
1565(fed)X
1693(to)X
1780(the)X
1903(shell)X
2079(on)X
2184(its)X
2284(\(the)X
2434(shell's\))X
2695(input)X
2884(stream.)X
3163(This)X
3330(means)X
3560(that)X
3705(any)X
3846(com-)X
555 3720(mands,)N
805(such)X
973(as)X
7 f
1061(ci)X
1 f
1178(that)X
1319(need)X
1492(to)X
1575(get)X
1694(input)X
1879(from)X
2056(the)X
2175(terminal)X
2463(won't)X
2671(work)X
2857(right)X
3029(\320)X
3130(they'll)X
3360(get)X
3479(the)X
3598(shell's)X
3827(input,)X
555 3816(something)N
917(they)X
1084(probably)X
1397(won't)X
1612(\256nd)X
1764(to)X
1854(their)X
2029(liking.)X
2263(A)X
2349(simple)X
2590(way)X
2752(around)X
3003(this)X
3146(is)X
3227(to)X
3317(give)X
3483(a)X
3547(command)X
3891(like)X
555 3912(this:)N
7 f
843 4056(ci)N
987($\(SRCS\))X
1371(<)X
1467(/dev/tty)X
1 f
555 4200(This)N
719(would)X
941(force)X
1129(the)X
1249(program's)X
1601(input)X
1787(to)X
1871(come)X
2067(from)X
2245(the)X
2365(terminal.)X
2673(If)X
2748(you)X
2889(can't)X
3071(do)X
3172(this)X
3308(for)X
3423(some)X
3613(reason,)X
3864(your)X
555 4296(only)N
717(other)X
902(alternative)X
1261(is)X
1334(to)X
1416(use)X
1543(PMake)X
1790(in)X
1872(its)X
1967(fullest)X
2187(compatibility)X
2633(mode.)X
2851(See)X
3 f
2987(Compatibility)X
1 f
3482(in)X
3564(chapter)X
3821(4.)X
3 f
555 4516(2.3.)N
715(Variables)X
1 f
755 4640(PMake,)N
1027(like)X
1172(Make)X
1380(before)X
1611(it,)X
1700(has)X
1832(the)X
1955(ability)X
2184(to)X
2271(save)X
2439(text)X
2583(in)X
2669(variables)X
2983(to)X
3069(be)X
3169(recalled)X
3448(later)X
3615(at)X
3697(your)X
3868(con-)X
555 4736(venience.)N
893(Variables)X
1233(in)X
1327(PMake)X
1586(are)X
1717(used)X
1895(much)X
2104(like)X
2255(variables)X
2576(in)X
2669(the)X
2798(shell)X
2980(and,)X
3147(by)X
3258(tradition,)X
3580(consist)X
3833(of)X
3931(all)X
555 4832(upper-case)N
932(letters)X
1156(\(you)X
1331(don't)X
2 f
1528(have)X
1 f
1721(to)X
1811(use)X
1946(all)X
2054(upper-case)X
2431(letters.)X
2695(In)X
2790(fact)X
2939(there's)X
3186(nothing)X
3458(to)X
3548(stop)X
3708(you)X
3855(from)X
555 4928(calling)N
793(a)X
849(variable)X
7 f
1128(@\303&$%$)X
1 f
(.)S
1476(Just)X
1620(tradition\).)X
1958(Variables)X
2286(are)X
2405(assigned-to)X
2790(using)X
2983(lines)X
3154(of)X
3241(the)X
3359(form)X
7 f
843 5072(VARIABLE)N
1275(=)X
1371(value)X
1 f
555 5216(appended-to)N
972(by)X
7 f
843 5360(VARIABLE)N
1275(+=)X
1419(value)X
1 f
555 5504(conditionally)N
997(assigned-to)X
1382(\(if)X
1478(the)X
1596(variable)X
1875(isn't)X
2037(already)X
2294(de\256ned\))X
2577(by)X
7 f
843 5648(VARIABLE)N
1275(?=)X
1419(value)X
1 f
555 5792(and)N
710(assigned-to)X
1114(with)X
1295(expansion)X
1659(\(i.e.)X
1823(the)X
1960(value)X
2173(is)X
2264(expanded)X
2610(\(see)X
2778(below\))X
3039(before)X
3283(being)X
3499(assigned)X
3813(to)X
3913(the)X

5 p
%%Page: 5 6
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(5)X
2323(-)X
555 672(variable\320useful)N
1110(for)X
1224(placing)X
1480(a)X
1536(value)X
1730(at)X
1808(the)X
1926(beginning)X
2266(of)X
2353(a)X
2409(variable,)X
2708(or)X
2795(other)X
2980(things\))X
3222(by)X
7 f
843 816(VARIABLE)N
1275(:=)X
1419(value)X
1 f
555 988(Any)N
715(whitespace)X
1094(before)X
2 f
1322(value)X
1 f
1531(is)X
1605(stripped)X
1884(off.)X
2019(When)X
2232(appending,)X
2607(a)X
2664(space)X
2864(is)X
2938(placed)X
3169(between)X
3458(the)X
3577(old)X
3700(value)X
3895(and)X
555 1084(the)N
673(stuff)X
840(being)X
1038(appended.)X
755 1208(The)N
900(\256nal)X
1062(way)X
1216(a)X
1272(variable)X
1551(may)X
1709(be)X
1805(assigned)X
2101(to)X
2183(is)X
2256(using)X
7 f
843 1352(VARIABLE)N
1275(!=)X
1419(shell-command)X
1 f
555 1496(In)N
647(this)X
787(case,)X
2 f
971(shell-command)X
1 f
1499(has)X
1631(all)X
1736(its)X
1836(variables)X
2151(expanded)X
2484(\(see)X
2639(below\))X
2887(and)X
3028(is)X
3106(passed)X
3345(off)X
3464(to)X
3551(a)X
3611(shell)X
3786(to)X
3872(exe-)X
555 1592(cute.)N
740(The)X
896(output)X
1131(of)X
1229(the)X
1358(shell)X
1540(is)X
1624(then)X
1793(placed)X
2034(in)X
2127(the)X
2256(variable.)X
2566(Any)X
2734(newlines)X
3049(\(other)X
3271(than)X
3439(the)X
3567(\256nal)X
3739(one\))X
3912(are)X
555 1688(replaced)N
852(by)X
956(spaces)X
1190(before)X
1420(the)X
1542(assignment)X
1926(is)X
2003(made.)X
2221(This)X
2387(is)X
2464(typically)X
2767(used)X
2937(to)X
3022(\256nd)X
3169(the)X
3290(current)X
3541(directory)X
3854(via)X
3975(a)X
555 1784(line)N
695(like:)X
7 f
843 1928(CWD)N
1611(!=)X
1755(pwd)X
3 f
555 2100(Note:)N
1 f
764(this)X
900(is)X
974(intended)X
1271(to)X
1354(be)X
1451(used)X
1619(to)X
1702(execute)X
1969(commands)X
2337(that)X
2478(produce)X
2758(small)X
2952(amounts)X
3244(of)X
3331(output)X
3555(\(e.g.)X
3718(``pwd''\).)X
555 2196(The)N
708(implementation)X
1238(is)X
1319(less)X
1467(than)X
1633(intelligent)X
1985(and)X
2129(will)X
2281(likely)X
2491(freeze)X
2717(if)X
2794(you)X
2941(execute)X
3214(something)X
3574(that)X
3721(produces)X
555 2292(thousands)N
895(of)X
982(bytes)X
1171(of)X
1258(output)X
1482(\(8)X
1569(Kb)X
1687(is)X
1760(the)X
1878(limit)X
2048(on)X
2148(many)X
2346(UNIX)X
2567(systems\).)X
755 2416(The)N
912(value)X
1118(of)X
1217(a)X
1285(variable)X
1576(may)X
1746(be)X
1854(retrieved)X
2172(by)X
2284(enclosing)X
2623(the)X
2753(variable)X
3044(name)X
3249(in)X
3342(parentheses)X
3748(or)X
3846(curly)X
555 2512(braces)N
781(and)X
917(preceeding)X
1290(the)X
1408(whole)X
1624(thing)X
1808(with)X
1970(a)X
2026(dollar)X
2233(sign.)X
755 2636(For)N
890(example,)X
1205(to)X
1290(set)X
1402(the)X
1523(variable)X
1805(CFLAGS)X
2134(to)X
2219(the)X
2340(string)X
2545(``)X
7 f
9 f
2599(-)X
7 f
2643(I/sprite/src/lib/libc)X
9 f
3702(-)X
7 f
3746(O)X
1 f
(,'')S
3891(you)X
555 2732(would)N
775(place)X
965(a)X
1021(line)X
7 f
843 2876(CFLAGS)N
1179(=)X
9 f
1275(-)X
7 f
1319(I/sprite/src/lib/libc)X
9 f
2375(-)X
7 f
2419(O)X
1 f
555 3020(in)N
701(the)X
883(make\256le)X
1243(and)X
1442(use)X
1632(the)X
1813(word)X
7 f
2061($\(CFLAGS\))X
1 f
2576(wherever)X
2959(you)X
3162(would)X
3445(like)X
3648(the)X
3829(string)X
7 f
9 f
555 3116(-)N
7 f
599(I/sprite/src/lib/libc)X
9 f
1655(-)X
7 f
1699(O)X
1 f
1767(to)X
1849(appear.)X
2104(This)X
2266(is)X
2339(called)X
2551(variable)X
2830(expansion.)X
-1 Ds
5 Dt
317 MX
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
320 3123 MXY
 320 3123 lineto
 388 3123 lineto
 434 3169 lineto
 434 3237 lineto
 388 3283 lineto
 320 3283 lineto
 273 3237 lineto
 273 3169 lineto
 320 3123 lineto
closepath 14 273 3123 434 3283 Dp
6 s
288 3217(NOTE)N
3 Dt
-1 Ds
10 s
755 3240(Unlike)N
997(Make,)X
1224(PMake)X
1474(will)X
1621(not)X
1746(expand)X
2001(a)X
2060(variable)X
2342(unless)X
2565(it)X
2632(knows)X
2864(the)X
2985(variable)X
3267(exists.)X
3492(E.g.)X
3644(if)X
3716(you)X
3859(have)X
555 3336(a)N
7 f
616(${i})X
1 f
833(in)X
920(a)X
981(shell)X
1157(command)X
1498(and)X
1639(you)X
1784(have)X
1961(not)X
2088(assigned)X
2389(a)X
2450(value)X
2649(to)X
2735(the)X
2857(variable)X
7 f
3140(i)X
1 f
3212(\(the)X
3361(empty)X
3585(string)X
3791(is)X
3868(con-)X
555 3432(sidered)N
813(a)X
875(value,)X
1095(by)X
1201(the)X
1325(way\),)X
1532(where)X
1755(Make)X
1964(would)X
2190(have)X
2368(substituted)X
2740(the)X
2864(empty)X
3090(string,)X
3317(PMake)X
3569(will)X
3718(leave)X
3913(the)X
7 f
555 3528(${i})N
1 f
767(alone.)X
1001(To)X
1110(keep)X
1282(PMake)X
1529(from)X
1705(substituting)X
2097(for)X
2211(a)X
2267(variable)X
2546(it)X
2610(knows,)X
2859(precede)X
3130(the)X
3248(dollar)X
3455(sign)X
3608(with)X
3770(another)X
555 3624(dollar)N
764(sign.)X
959(\(e.g.)X
1124(to)X
1208(pass)X
7 f
1368(${HOME})X
1 f
1726(to)X
1810(the)X
1930(shell,)X
2123(use)X
7 f
2252($${HOME})X
1 f
(\).)S
2725(This)X
2889(causes)X
3120(PMake,)X
3388(in)X
3471(effect,)X
3696(to)X
3779(expand)X
555 3720(the)N
7 f
680($)X
1 f
755(macro,)X
1003(which)X
1226(expands)X
1516(to)X
1605(a)X
1668(single)X
7 f
1886($)X
1 f
(.)S
2001(For)X
2139(compatibility,)X
2612(Make's)X
2879(style)X
3056(of)X
3149(variable)X
3434(expansion)X
3785(will)X
3935(be)X
555 3816(used)N
727(if)X
801(you)X
946(invoke)X
1189(PMake)X
1441(with)X
1608(any)X
1749(of)X
1841(the)X
1963(compatibility)X
2413(\257ags)X
2588(\()X
3 f
9 f
2615(-)X
3 f
2659(V)X
1 f
2717(,)X
3 f
9 f
2761(-)X
3 f
2805(B)X
1 f
2882(or)X
3 f
9 f
2973(-)X
3 f
3017(M)X
1 f
3093(.)X
3157(The)X
3 f
9 f
3306(-)X
3 f
3350(V)X
1 f
3432(\257ag)X
3576(alters)X
3774(just)X
3913(the)X
555 3912(variable)N
834(expansion\).)X
755 4036(There)N
968(are)X
1092(two)X
1237(different)X
1539(times)X
1737(at)X
1819(which)X
2039(variable)X
2322(expansion)X
2671(occurs:)X
2927(When)X
3143(parsing)X
3403(a)X
3463(dependency)X
3871(line,)X
555 4132(the)N
687(expansion)X
1046(occurs)X
1290(immediately)X
1724(upon)X
1918(reading)X
2193(the)X
2325(line.)X
2498(If)X
2585(any)X
2734(variable)X
3026(used)X
3206(on)X
3319(a)X
3388(dependency)X
3805(line)X
3958(is)X
555 4228(unde\256ned,)N
912(PMake)X
1160(will)X
1304(print)X
1475(a)X
1531(message)X
1823(and)X
1959(exit.)X
2139(Variables)X
2467(in)X
2549(shell)X
2720(commands)X
3087(are)X
3206(expanded)X
3534(when)X
3728(the)X
3846(com-)X
555 4324(mand)N
766(is)X
852(executed.)X
1211(Variables)X
1552(used)X
1732(inside)X
1956(another)X
2230(variable)X
2522(are)X
2654(expanded)X
2995(whenever)X
3340(the)X
3470(outer)X
3667(variable)X
3958(is)X
555 4420(expanded)N
885(\(the)X
1032(expansion)X
1379(of)X
1468(an)X
1566(inner)X
1753(variable)X
2034(has)X
2163(no)X
2265(effect)X
2471(on)X
2573(the)X
2693(outer)X
2879(variable.)X
3179(I.e.)X
3303(if)X
3373(the)X
3492(outer)X
3678(variable)X
3958(is)X
555 4516(used)N
723(on)X
824(a)X
881(dependency)X
1286(line)X
1427(and)X
1564(in)X
1647(a)X
1704(shell)X
1876(command,)X
2233(and)X
2370(the)X
2489(inner)X
2675(variable)X
2955(changes)X
3235(value)X
3430(between)X
3719(when)X
3913(the)X
555 4612(dependency)N
962(line)X
1105(is)X
1181(read)X
1343(and)X
1482(the)X
1603(shell)X
1776(command)X
2114(is)X
2189(executed,)X
2517(two)X
2659(different)X
2958(values)X
3185(will)X
3331(be)X
3429(substituted)X
3797(for)X
3913(the)X
555 4708(outer)N
740(variable\).)X
755 4832(Variables)N
1089(come)X
1289(in)X
1377(four)X
1537(\257avors,)X
1801(though)X
2049(they)X
2212(are)X
2336(all)X
2441(expanded)X
2774(the)X
2897(same)X
3087(and)X
3228(all)X
3333(look)X
3500(about)X
3703(the)X
3826(same.)X
555 4928(They)N
740(are)X
859(\(in)X
968(order)X
1158(of)X
1245(expanding)X
1599(scope\):)X
10 f
755 5052(g)N
1 f
835(Local)X
1038(variables.)X
10 f
755 5176(g)N
1 f
835(Command-line)X
1335(variables.)X
10 f
755 5300(g)N
1 f
835(Global)X
1073(variables.)X
10 f
755 5424(g)N
1 f
835(Environment)X
1273(variables.)X
555 5548(The)N
716(classi\256cation)X
1170(of)X
1273(variables)X
1599(doesn't)X
1870(matter)X
2110(much,)X
2343(except)X
2588(that)X
2743(the)X
2876(classes)X
3134(are)X
3268(searched)X
3585(from)X
3776(the)X
3909(top)X
555 5644(\(local\))N
785(to)X
867(the)X
985(bottom)X
1231(\(environment\))X
1710(when)X
1904(looking)X
2168(up)X
2268(a)X
2324(variable.)X
2623(The)X
2768(\256rst)X
2912(one)X
3048(found)X
3255(wins.)X

6 p
%%Page: 6 7
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(6)X
2323(-)X
3 f
555 672(2.3.1.)N
775(Local)X
986(Variables)X
1 f
755 796(Each)N
943(target)X
1153(can)X
1292(have)X
1471(as)X
1565(many)X
1770(as)X
1864(seven)X
2074(local)X
2256(variables.)X
2592(These)X
2810(are)X
2935(variables)X
3251(that)X
3397(are)X
3522(only)X
3690(``visible'')X
555 892(within)N
784(that)X
929(target's)X
1195(shell)X
1371(script)X
1574(and)X
1715(contain)X
1976(such)X
2148(things)X
2368(as)X
2460(the)X
2583(target's)X
2848(name,)X
3066(all)X
3170(of)X
3261(its)X
3360(sources)X
3625(\(from)X
3832(all)X
3936(its)X
555 988(dependency)N
962(lines\),)X
1183(those)X
1375(sources)X
1639(that)X
1782(were)X
1962(out-of-date,)X
2361(etc.)X
2517(Four)X
2690(local)X
2868(variables)X
3180(are)X
3301(de\256ned)X
3559(for)X
3675(all)X
3777(targets.)X
555 1084(They)N
740(are:)X
755 1208(.TARGET)N
955 1304(The)N
1100(name)X
1294(of)X
1381(the)X
1499(target.)X
755 1428(.OODATE)N
955 1524(The)N
1106(list)X
1228(of)X
1320(the)X
1443(sources)X
1709(for)X
1828(the)X
1951(target)X
2159(that)X
2304(were)X
2486(considered)X
2859(out-of-date.)X
3281(The)X
3431(order)X
3626(in)X
3713(the)X
3836(list)X
3958(is)X
955 1620(not)N
1077(guaranteed)X
1450(to)X
1532(be)X
1628(the)X
1746(same)X
1931(as)X
2018(the)X
2136(order)X
2326(in)X
2408(which)X
2624(the)X
2742(dependencies)X
3195(were)X
3372(given.)X
755 1744(.ALLSRC)N
955 1840(The)N
1100(list)X
1217(of)X
1304(all)X
1404(sources)X
1665(for)X
1779(this)X
1914(target)X
2117(in)X
2199(the)X
2317(order)X
2507(in)X
2589(which)X
2805(they)X
2963(were)X
3140(given.)X
755 1964(.PREFIX)N
955 2060(The)N
1148(target)X
1399(without)X
1710(its)X
1852(suf\256x)X
2101(and)X
2284(without)X
2595(any)X
2778(leading)X
3081(path.)X
3306(E.g.)X
3502(for)X
3663(the)X
3828(target)X
7 f
955 2156(../../lib/compat/fsRead.c)N
1 f
(,)S
2195(this)X
2330(variable)X
2609(would)X
2829(contain)X
7 f
3085(fsRead)X
1 f
(.)S
555 2280(Three)N
779(other)X
980(local)X
1172(variables)X
1498(are)X
1633(set)X
1757(only)X
1934(for)X
2063(certain)X
2317(targets)X
2566(under)X
2784(special)X
3042(circumstances.)X
3552(These)X
3779(are)X
3913(the)X
555 2376(``.IMPSRC,'')N
1024(``.ARCHIVE,'')X
1557(and)X
1702(``.MEMBER'')X
2205(variables.)X
2544(When)X
2765(they)X
2932(are)X
3060(set)X
3178(and)X
3323(how)X
3490(they)X
3656(are)X
3783(used)X
3958(is)X
555 2472(described)N
883(later.)X
755 2596(Four)N
932(of)X
1025(these)X
1216(variables)X
1532(may)X
1696(be)X
1797(used)X
1969(in)X
2056(sources)X
2322(as)X
2414(well)X
2577(as)X
2669(in)X
2756(shell)X
2932(scripts.)X
3206(These)X
3423(are)X
3547(``.TARGET'',)X
555 2692(``.PREFIX'',)N
1003(``.ARCHIVE'')X
1512(and)X
1653(``.MEMBER''.)X
2172(The)X
2322(variables)X
2637(in)X
2724(the)X
2847(sources)X
3113(are)X
3237(expanded)X
3569(once)X
3745(for)X
3863(each)X
555 2788(target)N
765(on)X
872(the)X
997(dependency)X
1408(line,)X
1575(providing)X
1913(what)X
2095(is)X
2174(known)X
2418(as)X
2511(a)X
2573(``dynamic)X
2929(source,'')X
3239(allowing)X
3545(you)X
3691(to)X
3779(specify)X
555 2884(several)N
803(dependency)X
1207(lines)X
1378(at)X
1456(once.)X
1648(For)X
1779(example,)X
7 f
843 3028($\(OBJS\))N
1611(:)X
1707($\(.PREFIX\).c)X
1 f
555 3172(will)N
699(create)X
912(a)X
968(dependency)X
1372(between)X
1660(each)X
1828(object)X
2044(\256le)X
2166(and)X
2302(its)X
2397(corresponding)X
2876(C)X
2949(source)X
3179(\256le.)X
3 f
555 3364(2.3.2.)N
775(Command-line)X
1306(Variables)X
1 f
755 3488(Command-line)N
1258(variables)X
1571(are)X
1693(set)X
1805(when)X
2002(PMake)X
2252(is)X
2328(\256rst)X
2475(invoked)X
2756(by)X
2859(giving)X
3085(a)X
3143(variable)X
3424(assignment)X
3806(as)X
3895(one)X
555 3584(of)N
642(the)X
760(arguments.)X
1134(For)X
1265(example,)X
7 f
843 3728(pmake)N
1131("CFLAGS)X
1515(=)X
1611(-I/sprite/src/lib/libc)X
2715(-O")X
1 f
555 3872(would)N
783(make)X
7 f
985(CFLAGS)X
1 f
1300(be)X
1403(a)X
1466(command-line)X
1956(variable)X
2242(with)X
2411(the)X
2536(given)X
2741(value.)X
2962(Any)X
3127(assignments)X
3545(to)X
7 f
3634(CFLAGS)X
1 f
3949(in)X
555 3968(the)N
681(make\256le)X
985(will)X
1137(have)X
1317(no)X
1425(effect,)X
1657(because)X
1939(once)X
2118(it)X
2189(is)X
2269(set,)X
2405(there)X
2593(is)X
2673(\(almost\))X
2967(nothing)X
3238(you)X
3385(can)X
3524(do)X
3631(to)X
3720(change)X
3975(a)X
555 4064(command-line)N
1046(variable)X
1333(\(the)X
1486(search)X
1720(order,)X
1938(you)X
2086(see\).)X
2264(Command-line)X
2772(variables)X
3090(may)X
3256(be)X
3360(set)X
3476(using)X
3676(any)X
3819(of)X
3913(the)X
555 4160(four)N
719(assignment)X
1109(operators,)X
1458(though)X
1710(only)X
7 f
1882(=)X
1 f
1960(and)X
7 f
2106(?=)X
1 f
2232(behave)X
2490(as)X
2587(you)X
2737(would)X
2967(expect)X
3207(them)X
3397(to,)X
3509(mostly)X
3756(because)X
555 4256(assignments)N
968(to)X
1052(command-line)X
1537(variables)X
1848(are)X
1968(performed)X
2324(before)X
2551(the)X
2670(make\256le)X
2967(is)X
3041(read,)X
3221(thus)X
3375(the)X
3494(values)X
3720(set)X
3830(in)X
3913(the)X
555 4352(make\256le)N
857(are)X
982(unavailable)X
1378(at)X
1462(the)X
1586(time.)X
7 f
1794(+=)X
1 f
1916(is)X
1994(the)X
2117(same)X
2307(as)X
7 f
2399(=)X
1 f
(,)S
2492(because)X
2772(the)X
2895(old)X
3022(value)X
3221(of)X
3313(the)X
3436(variable)X
3720(is)X
3798(sought)X
555 4448(only)N
725(in)X
815(the)X
941(scope)X
1152(in)X
1242(which)X
1466(the)X
1592(assignment)X
1980(is)X
2061(taking)X
2289(place)X
2487(\(for)X
2636(reasons)X
2905(of)X
3000(ef\256ciency)X
3345(that)X
3493(I)X
3548(won't)X
3762(get)X
3887(into)X
555 4544(here\).)N
7 f
782(:=)X
1 f
899(and)X
7 f
1036(?=)X
1 f
1153(will)X
1298(work)X
1484(if)X
1554(the)X
1673(only)X
1836(variables)X
2147(used)X
2315(are)X
2435(in)X
2518(the)X
2637(environment.)X
7 f
3102(!=)X
1 f
3218(is)X
3291(sort)X
3431(of)X
3518(pointless)X
3822(to)X
3904(use)X
555 4640(from)N
739(the)X
864(command)X
1207(line,)X
1374(since)X
1566(the)X
1691(same)X
1883(effect)X
2094(can)X
2233(no)X
2340(doubt)X
2549(be)X
2652(accomplished)X
3120(using)X
3320(the)X
3445(shell's)X
3681(own)X
3846(com-)X
555 4736(mand)N
753(substitution)X
1145(mechanisms)X
1561(\(backquotes)X
1969(and)X
2105(all)X
2205(that\).)X
3 f
555 4928(2.3.3.)N
775(Global)X
1025(Variables)X
1 f
755 5052(Global)N
999(variables)X
1315(are)X
1439(those)X
1633(set)X
1747(or)X
1839(appended-to)X
2261(in)X
2348(the)X
2471(make\256le.)X
2812(There)X
3025(are)X
3149(two)X
3294(classes)X
3542(of)X
3634(global)X
3859(vari-)X
555 5148(ables:)N
765(those)X
957(you)X
1099(set)X
1210(and)X
1348(those)X
1539(PMake)X
1788(sets.)X
1970(As)X
2081(I)X
2130(said)X
2281(before,)X
2529(the)X
2649(ones)X
2818(you)X
2960(set)X
3071(can)X
3205(have)X
3379(any)X
3517(name)X
3713(you)X
3855(want)X
555 5244(them)N
746(to)X
839(have,)X
1042(except)X
1283(they)X
1452(may)X
1621(not)X
1754(contain)X
2021(a)X
2088(colon)X
2297(or)X
2395(an)X
2502(exclamation)X
2925(point.)X
3159(The)X
3314(variables)X
3634(PMake)X
3891(sets)X
555 5340(\(almost\))N
848(always)X
1097(begin)X
1301(with)X
1469(a)X
1531(period)X
1762(and)X
1903(always)X
2151(contain)X
2412(upper-case)X
2786(letters,)X
3027(only.)X
3214(The)X
3364(variables)X
3679(are)X
3803(as)X
3895(fol-)X
555 5436(lows:)N
755 5560(.PMAKE)N
955 5656(The)N
1103(name)X
1300(by)X
1403(which)X
1622(PMake)X
1872(was)X
2020(invoked)X
2301(is)X
2376(stored)X
2594(in)X
2678(this)X
2815(variable.)X
3116(For)X
3249(compatibility,)X
3717(the)X
3837(name)X
955 5752(is)N
1028(also)X
1177(stored)X
1393(in)X
1475(the)X
1593(MAKE)X
1849(variable.)X

7 p
%%Page: 7 8
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(7)X
2323(-)X
755 672(.MAKEFLAGS)N
955 768(All)N
1081(the)X
1203(relevant)X
1486(\257ags)X
1660(with)X
1825(which)X
2044(PMake)X
2294(was)X
2442(invoked.)X
2743(This)X
2908(does)X
3078(not)X
3203(include)X
3462(such)X
3632(things)X
3850(as)X
3 f
9 f
3940(-)X
3 f
3984(f)X
1 f
955 864(or)N
1046(variable)X
1329(assignments.)X
1764(Again)X
1983(for)X
2100(compatibility,)X
2569(this)X
2707(value)X
2904(is)X
2980(stored)X
3199(in)X
3284(the)X
3405(MFLAGS)X
3752(variable)X
955 960(as)N
1042(well.)X
555 1084(Two)N
724(other)X
910(variables,)X
1241(``.INCLUDES'')X
1786(and)X
1923(``.LIBS,'')X
2265(are)X
2385(covered)X
2661(in)X
2744(the)X
2863(section)X
3111(on)X
3212(special)X
3456(targets)X
3691(in)X
3774(chapter)X
555 1180(3.)N
755 1304(Global)N
993(variables)X
1303(may)X
1461(be)X
1557(deleted)X
1809(using)X
2002(lines)X
2173(of)X
2260(the)X
2378(form:)X
7 f
843 1448(#undef)N
2 f
1179(variable)X
1 f
555 1592(The)N
700(`)X
7 f
727(#)X
1 f
(')S
822(must)X
997(be)X
1093(the)X
1211(\256rst)X
1355(character)X
1671(on)X
1771(the)X
1889(line.)X
2049(Note)X
2225(that)X
2365(this)X
2500(may)X
2658(only)X
2820(be)X
2916(done)X
3092(on)X
3192(global)X
3412(variables.)X
3 f
555 1784(2.3.4.)N
775(Environment)X
1248(Variables)X
1 f
755 1908(Environment)N
1198(variables)X
1513(are)X
1637(passed)X
1876(by)X
1981(the)X
2104(shell)X
2280(that)X
2424(invoked)X
2706(PMake)X
2957(and)X
3097(are)X
3220(given)X
3422(by)X
3526(PMake)X
3777(to)X
3863(each)X
555 2004(shell)N
726(it)X
790(invokes.)X
1079(They)X
1264(are)X
1383(expanded)X
1711(like)X
1851(any)X
1987(other)X
2172(variable,)X
2471(but)X
2593(they)X
2751(cannot)X
2985(be)X
3081(altered)X
3320(in)X
3402(any)X
3538(way.)X
755 2128(One)N
914(special)X
1162(environment)X
1592(variable,)X
7 f
1896(PMAKE)X
1 f
(,)S
2181(is)X
2259(examined)X
2596(by)X
2701(PMake)X
2952(for)X
3070(command-line)X
3557(\257ags,)X
3752(variable)X
555 2224(assignments,)N
994(etc.,)X
1156(it)X
1228(should)X
1469(always)X
1720(use.)X
1875(This)X
2045(variable)X
2332(is)X
2413(examined)X
2753(before)X
2987(the)X
3113(actual)X
3333(arguments)X
3695(to)X
3784(PMake)X
555 2320(are.)N
699(In)X
791(addition,)X
1098(all)X
1203(\257ags)X
1379(given)X
1582(to)X
1669(PMake,)X
1941(either)X
2149(through)X
2422(the)X
7 f
2544(PMAKE)X
1 f
2808(variable)X
3091(or)X
3182(on)X
3286(the)X
3408(command)X
3748(line,)X
3912(are)X
555 2416(placed)N
787(in)X
871(this)X
1008(environment)X
1435(variable)X
1716(and)X
1853(exported)X
2155(to)X
2238(each)X
2407(shell)X
2579(PMake)X
2827(executes.)X
3145(Thus)X
3326(recursive)X
3642(invocations)X
555 2512(of)N
642(PMake)X
889(automatically)X
1345(receive)X
1598(the)X
1716(same)X
1901(\257ags)X
2072(as)X
2159(the)X
2277(top-most)X
2581(one.)X
755 2636(Using)N
966(all)X
1066(these)X
1251(variables,)X
1581(you)X
1721(can)X
1853(compress)X
2176(the)X
2294(sample)X
2541(make\256le)X
2837(even)X
3009(more:)X
7 f
843 2780(OBJS)N
1611(=)X
1707(a.o)X
1899(b.o)X
2091(c.o)X
843 2876(program)N
1611(:)X
1707($\(OBJS\))X
1227 2972(cc)N
1371($\(.ALLSRC\))X
9 f
1899(-)X
7 f
1943(o)X
2039($\(.TARGET\))X
843 3068($\(OBJS\))N
1611(:)X
1707(defs.h)X
843 3164(a.o)N
1611(:)X
1707(a.c)X
1227 3260(cc)N
9 f
1371(-)X
7 f
1415(c)X
1511(a.c)X
843 3356(b.o)N
1611(:)X
1707(b.c)X
1227 3452(cc)N
9 f
1371(-)X
7 f
1415(c)X
1511(b.c)X
843 3548(c.o)N
1611(:)X
1707(c.c)X
1227 3644(cc)N
9 f
1371(-)X
7 f
1415(c)X
1511(c.c)X
3 f
555 3884(2.4.)N
715(Comments)X
1 f
755 4008(Comments)N
1123(in)X
1207(a)X
1264(make\256le)X
1561(start)X
1720(with)X
1883(a)X
1940(`#')X
2055(character)X
2372(and)X
2509(extend)X
2744(to)X
2827(the)X
2946(end)X
3083(of)X
3171(the)X
3290(line.)X
3451(They)X
3637(may)X
3796(appear)X
555 4104(anywhere)N
893(you)X
1038(want)X
1219(them,)X
1423(except)X
1657(in)X
1743(a)X
1803(shell)X
1978(command)X
2318(\(though)X
2591(the)X
2713(shell)X
2888(will)X
3036(treat)X
3203(it)X
3271(as)X
3362(a)X
3422(comment,)X
3764(too\).)X
3937(If,)X
555 4200(for)N
670(some)X
860(reason,)X
1111(you)X
1252(need)X
1425(to)X
1508(use)X
1636(the)X
1754(`#')X
1868(in)X
1950(a)X
2006(variable)X
2285(or)X
2372(on)X
2472(a)X
2528(dependency)X
2932(line,)X
3092(put)X
3214(a)X
3270(backslash)X
3602(in)X
3684(front)X
3860(of)X
3947(it.)X
555 4296(PMake)N
822(will)X
986(compress)X
1329(the)X
1467(two)X
1627(into)X
1791(a)X
1867(single)X
2098(`#')X
2232(\(Note:)X
2477(this)X
2632(isn't)X
2814(true)X
2979(if)X
3068(PMake)X
3335(is)X
3428(operating)X
3771(in)X
3873(full-)X
555 4392(compatibility)N
1001(mode\).)X
3 f
555 4584(2.5.)N
715(Parallelism)X
-1 Ds
5 Dt
317 MX
71 0 Dl
50 50 Dl
0 71 Dl
-50 50 Dl
-71 0 Dl
-50 -50 Dl
0 -71 Dl
50 -50 Dl
3 Dt
320 4591 MXY
 320 4591 lineto
 388 4591 lineto
 434 4637 lineto
 434 4705 lineto
 388 4751 lineto
 320 4751 lineto
 273 4705 lineto
 273 4637 lineto
 320 4591 lineto
closepath 14 273 4591 434 4751 Dp
1 f
6 s
288 4685(NOTE)N
3 Dt
-1 Ds
10 s
755 4708(PMake)N
1003(was)X
1149(speci\256cally)X
1535(designed)X
1841(to)X
1924 0.4219(re-create)AX
2228(several)X
2477(targets)X
2712(at)X
2791(once,)X
2983(when)X
3177(possible.)X
3479(You)X
3637(do)X
3737(not)X
3859(have)X
555 4804(to)N
639(do)X
740(anything)X
1041(special)X
1285(to)X
1368(cause)X
1568(this)X
1704(to)X
1787(happen)X
2040(\(unless)X
2288(PMake)X
2536(was)X
2682(con\256gured)X
3046(to)X
3129(not)X
3252(act)X
3367(in)X
3450(parallel,)X
3732(in)X
3815(which)X
555 4900(case)N
714(you)X
854(will)X
998(have)X
1170(to)X
1252(make)X
1446(use)X
1573(of)X
1660(the)X
3 f
9 f
1778(-)X
3 f
1822(L)X
1 f
1895(and)X
3 f
9 f
2031(-)X
3 f
2075(J)X
1 f
2135(\257ags)X
2306(\(see)X
2456(below\)\),)X
2746(but)X
2868(you)X
3008(do)X
3108(have)X
3280(to)X
3362(be)X
3458(careful)X
3702(at)X
3780(times.)X
755 5024(There)N
969(are)X
1094(several)X
1348(problems)X
1672(you)X
1818(are)X
1943(likely)X
2151(to)X
2239(encounter.)X
2602(One)X
2762(is)X
2841(that)X
2987(some)X
3181(make\256les)X
3513(\(and)X
3681(programs\))X
555 5120(are)N
679(written)X
931(in)X
1018(such)X
1190(a)X
1251(way)X
1410(that)X
1555(it)X
1624(is)X
1702(impossible)X
2073(for)X
2192(two)X
2336(targets)X
2574(to)X
2660(be)X
2760(made)X
2958(at)X
3040(once.)X
3236(The)X
3385(program)X
7 f
3681(xstr)X
1 f
(,)S
3917(for)X
555 5216(example,)N
869(always)X
1114(modi\256es)X
1411(the)X
1531(\256les)X
7 f
1686(strings)X
1 f
2044(and)X
7 f
2182(x.c)X
1 f
(.)S
2388(There)X
2598(is)X
2673(no)X
2775(way)X
2930(to)X
3013(change)X
3262(it.)X
3347(Thus)X
3528(you)X
3669(cannot)X
3904(run)X
555 5312(two)N
696(of)X
784(them)X
965(at)X
1044(once)X
1217(without)X
1482(something)X
1836(being)X
2035(trashed.)X
2308(Similarly,)X
2646(if)X
2716(you)X
2856(have)X
3028(commands)X
3395(in)X
3477(the)X
3595(make\256le)X
3891(that)X
555 5408(always)N
803(send)X
975(output)X
1204(to)X
1291(the)X
1414(same)X
1604(\256le,)X
1751(you)X
1895(will)X
2043(not)X
2169(be)X
2269(able)X
2427(to)X
2513(make)X
2711(more)X
2900(than)X
3062(one)X
3202(target)X
3409(at)X
3491(once)X
3667(unless)X
3891(you)X
555 5504(change)N
808(the)X
931(\256le)X
1058(you)X
1203(use.)X
1355(You)X
1518(can,)X
1675(for)X
1794(instance,)X
2102(add)X
2243(a)X
7 f
2304($$$$)X
1 f
2521(to)X
2608(the)X
2730(end)X
2870(of)X
2961(the)X
3083(\256le)X
3209(name)X
3407(to)X
3493(tack)X
3651(on)X
3755(the)X
3877(pro-)X
555 5600(cess)N
712(ID)X
820(of)X
910(the)X
1031(shell)X
1205(executing)X
1540(the)X
1660(command)X
1998(\(each)X
7 f
2195($$)X
1 f
2313(expands)X
2598(to)X
2682(a)X
2740(single)X
7 f
2953($)X
1 f
(,)S
3043(thus)X
3198(giving)X
3424(you)X
3566(the)X
3686(shell)X
3859(vari-)X
555 5696(able)N
7 f
714($$)X
1 f
(\).)S
902(Since)X
1105(only)X
1272(one)X
1413(shell)X
1589(is)X
1667(used)X
1839(for)X
1958(all)X
2063(the)X
2186(commands,)X
2578(you'll)X
2794(get)X
2917(the)X
3040(same)X
3230(\256le)X
3357(name)X
3556(for)X
3674(each)X
3846(com-)X
555 5792(mand)N
753(in)X
835(the)X
953(script.)X

8 p
%%Page: 8 9
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(8)X
2323(-)X
755 672(The)N
906(other)X
1097(problem)X
1390(comes)X
1621(from)X
1803(improperly-speci\256ed)X
2497(dependencies)X
2956(that)X
3102(worked)X
3369(in)X
3456(Make)X
3664(because)X
3944(of)X
555 768(its)N
663(sequential,)X
1041(depth-\256rst)X
1403(way)X
1570(of)X
1670(examining)X
2041(them.)X
2254(While)X
2482(I)X
2541(don't)X
2742(want)X
2930(to)X
3024(go)X
3136(into)X
3292(depth)X
3502(on)X
3614(how)X
3784(PMake)X
555 864(works)N
779(\(look)X
976(in)X
1066(chapter)X
1331(4)X
1399(if)X
1476(you're)X
1714(interested\),)X
2101(I)X
2156(will)X
2308(warn)X
2497(you)X
2645(that)X
2793(\256les)X
2954(in)X
3044(two)X
3192(different)X
3497(``levels'')X
3819(of)X
3913(the)X
555 960(dependency)N
968(tree)X
1118(may)X
1285(be)X
1390(examined)X
1731(in)X
1822(a)X
1887(different)X
2193(order)X
2392(in)X
2483(PMake)X
2739(than)X
2906(they)X
3073(were)X
3259(in)X
3349(Make.)X
3580(For)X
3719(example,)X
555 1056(given)N
753(the)X
871(make\256le)X
7 f
843 1200(a)N
1611(:)X
1707(b)X
1803(c)X
843 1296(b)N
1611(:)X
1707(d)X
1 f
555 1440(PMake)N
807(will)X
956(examine)X
1253(the)X
1376(targets)X
1615(in)X
1702(the)X
1825(order)X
7 f
2020(c)X
1 f
(,)S
7 f
2113(d)X
1 f
(,)S
7 f
2206(b)X
1 f
(,)S
7 f
2299(a)X
1 f
(.)S
2412(If)X
2490(the)X
2612(make\256le's)X
2970(author)X
3199(expected)X
3509(PMake)X
3760(to)X
3846(abort)X
555 1536(before)N
784(making)X
7 f
1047(c)X
1 f
1118(if)X
1190(an)X
1289(error)X
1469(occurred)X
1774(while)X
1975(making)X
7 f
2238(b)X
1 f
(,)S
2329(or)X
2419(if)X
7 f
2491(b)X
1 f
2562(needed)X
2813(to)X
2898(exist)X
3072(before)X
7 f
3301(c)X
1 f
3372(was)X
3520(made,)X
3736(s/he)X
3887(will)X
555 1632(be)N
655(sorely)X
875(disappointed.)X
1327(The)X
1475(dependencies)X
1931(are)X
2053(incomplete,)X
2452(since)X
2640(in)X
2725(both)X
2890(these)X
3078(cases,)X
7 f
3291(c)X
1 f
3362(would)X
3585(depend)X
3840(on)X
7 f
3943(b)X
1 f
(.)S
555 1728(So)N
659(watch)X
871(out.)X
755 1852(Another)N
1040(problem)X
1329(you)X
1470(may)X
1629(face)X
1785(is)X
1859(that,)X
2020(while)X
2219(PMake)X
2467(is)X
2541(set)X
2651(up)X
2752(to)X
2835(handle)X
3070(the)X
3189(output)X
3414(from)X
3591(multiple)X
3878(jobs)X
555 1948(in)N
638(a)X
695(graceful)X
980(fashion,)X
1257(the)X
1375(same)X
1560(is)X
1633(not)X
1755(so)X
1846(for)X
1960(input.)X
2184(It)X
2253(has)X
2380(no)X
2480(way)X
2634(to)X
2716(regulate)X
2995(input)X
3179(to)X
3261(different)X
3558(jobs,)X
3731(so)X
3822(if)X
3891(you)X
555 2044(use)N
686(the)X
808(redirection)X
1180(from)X
7 f
1360(/dev/tty)X
1 f
1768(I)X
1819(mentioned)X
2181(earlier,)X
2431(you)X
2575(must)X
2754(be)X
2854(careful)X
3102(not)X
3228(to)X
3313(run)X
3443(two)X
3586(of)X
3676(the)X
3797(jobs)X
3953(at)X
555 2140(once.)N
3 f
555 2332(2.6.)N
715(Writing)X
1006(and)X
1154(Debugging)X
1542(a)X
1602(Make\256le)X
1 f
755 2456(Now)N
937(you)X
1083(know)X
1287(most)X
1468(of)X
1561(what's)X
1801(in)X
1889(a)X
1951(make\256le,)X
2273(what)X
2455(do)X
2561(you)X
2707(do)X
2813(next?)X
3013(There)X
3227(are)X
3352(two)X
3497(choices:)X
3785(\(1\))X
3904(use)X
555 2552(one)N
694(of)X
784(the)X
905(uncommonly-available)X
1667(make\256le)X
1966(generators)X
2324(or)X
2414(\(2\))X
2531(write)X
2719(your)X
2889(own)X
3050(make\256le)X
3348(\(I)X
3424(leave)X
3616(out)X
3740(the)X
3860(third)X
555 2648(choice)N
785(of)X
872(ignoring)X
1163(PMake)X
1410(and)X
1546(doing)X
1748(everything)X
2111(by)X
2211(hand)X
2387(as)X
2474(being)X
2672(beyond)X
2928(the)X
3046(bounds)X
3297(of)X
3384(common)X
3684(sense\).)X
755 2772(When)N
974(faced)X
1176(with)X
1345(the)X
1470(writing)X
1728(of)X
1821(a)X
1883(make\256le,)X
2205(it)X
2275(is)X
2354(usually)X
2611(best)X
2766(to)X
2854(start)X
3018(from)X
3200(\256rst)X
3350(principles:)X
3714(just)X
3855(what)X
2 f
555 2868(are)N
1 f
695(you)X
835(trying)X
1046(to)X
1128(do?)X
1264(What)X
1458(do)X
1558(you)X
1698(want)X
1874(the)X
1992(make\256le)X
2288(\256nally)X
2512(to)X
2594(produce?)X
755 2992(To)N
866(begin)X
1066(with)X
1230(a)X
1288(somewhat)X
1635(traditional)X
1986(example,)X
2299(let's)X
2458(say)X
2586(you)X
2727(need)X
2900(to)X
2983(write)X
3169(a)X
3226(make\256le)X
3523(to)X
3606(create)X
3820(a)X
3877(pro-)X
555 3088(gram,)N
7 f
761(expr)X
1 f
(,)S
994(that)X
1135(takes)X
1321(standard)X
1614(in\256x)X
1781(expressions)X
2176(and)X
2313(converts)X
2605(them)X
2785(to)X
2867(pre\256x)X
3074(form)X
3250(\(for)X
3391(no)X
3491(readily)X
3734(apparent)X
555 3184(reason\).)N
856(You've)X
1141(got)X
1287(three)X
1492(source)X
1746(\256les,)X
1943(in)X
2048(C,)X
2164(that)X
2327(make)X
2544(up)X
2667(the)X
2808(program:)X
7 f
3145(main.c)X
1 f
(,)S
7 f
3496(parse.c)X
1 f
(,)S
3895(and)X
7 f
555 3280(output.c)N
1 f
(.)S
999(Harking)X
1282(back)X
1454(to)X
1536(my)X
1658(pithy)X
1842(advice)X
2072(about)X
2270(dependency)X
2674(lines,)X
2865(you)X
3005(write)X
3190(the)X
3308(\256rst)X
3452(line)X
3592(of)X
3679(the)X
3797(\256le:)X
7 f
843 3424(expr)N
1611(:)X
1707(main.o)X
2043(parse.o)X
2427(output.o)X
1 f
555 3568(because)N
831(you)X
972(remember)X
7 f
1319(expr)X
1 f
1532(is)X
1606(made)X
1800(from)X
7 f
1976(.o)X
1 f
2092(\256les,)X
2265(not)X
7 f
2387(.c)X
1 f
2503(\256les.)X
2676(Similarly)X
2993(for)X
3107(the)X
7 f
3225(.o)X
1 f
3341(\256les)X
3494(you)X
3634(produce)X
3913(the)X
555 3664(lines:)N
7 f
843 3808(main.o)N
1611(:)X
1707(main.c)X
843 3904(parse.o)N
1611(:)X
1707(parse.c)X
843 4000(output.o)N
1611(:)X
1707(output.c)X
843 4096(main.o)N
1179(parse.o)X
1563(output.o)X
1995(:)X
2091(defs.h)X
1 f
755 4268(Great.)N
982(You've)X
1250(now)X
1415(got)X
1544(the)X
1669(dependencies)X
2129(speci\256ed.)X
2461(What)X
2662(you)X
2809(need)X
2988(now)X
3153(is)X
3233(commands.)X
3627(These)X
3846(com-)X
555 4364(mands,)N
814(remember,)X
1190(must)X
1375(produce)X
1663(the)X
1790(target)X
2002(on)X
2111(the)X
2238(dependency)X
2651(line,)X
2820(usually)X
3080(by)X
3189(using)X
3391(the)X
3518(sources)X
3788(you've)X
555 4460(listed.)N
788(You)X
946(remember)X
1292(about)X
1490(local)X
1666(variables?)X
2012(Good,)X
2230(so)X
2321(it)X
2385(should)X
2618(come)X
2812(to)X
2894(you)X
3034(as)X
3121(no)X
3221(surprise)X
3495(when)X
3689(you)X
3829(write)X
7 f
843 4604(expr)N
1611(:)X
1707(main.o)X
2043(parse.o)X
2427(output.o)X
1227 4700(cc)N
1371(-o)X
1515($\(.TARGET\))X
2043($\(.ALLSRC\))X
1 f
555 4844(Why)N
748(use)X
892(the)X
1027(variables?)X
1390(If)X
1481(your)X
1665(program)X
1973(grows)X
2205(to)X
2303(produce)X
2598(post\256x)X
2851(expressions)X
3261(too)X
3399(\(which,)X
3678(of)X
3781(course,)X
555 4940(requires)N
840(a)X
902(name)X
1102(change)X
1356(or)X
1449(two\),)X
1642(it)X
1712(is)X
1791(one)X
1933(fewer)X
2143(place)X
2338(you)X
2483(have)X
2660(to)X
2747(change)X
3000(the)X
3123(\256le.)X
3270(You)X
3433(cannot)X
3672(do)X
3777(this)X
3917(for)X
555 5036(the)N
682(object)X
907(\256les,)X
1089(however,)X
1415(because)X
1699(they)X
1866(depend)X
2127(on)X
2236(their)X
2412(corresponding)X
2900(source)X
3139(\256les)X
2 f
3301(and)X
7 f
3463(defs.h)X
1 f
(,)S
3800(thus)X
3962(if)X
555 5132(you)N
695(said)X
7 f
1043 5276(cc)N
1187(-c)X
1331($\(.ALLSRC\))X
1 f
555 5420(you'd)N
762(get)X
880(\(for)X
7 f
1021(main.o)X
1 f
(\):)S
7 f
1043 5564(cc)N
1187(-c)X
1331(main.c)X
1667(defs.h)X
1 f
555 5708(which)N
771(is)X
844(wrong.)X
1089(So)X
1193(you)X
1333(round)X
1540(out)X
1662(the)X
1780(make\256le)X
2076(with)X
2238(these)X
2423(lines:)X

9 p
%%Page: 9 10
10 s 10 xH 0 xS 1 f
7 f
1 f
2216 384(-)N
2263(9)X
2323(-)X
7 f
843 720(main.o)N
1611(:)X
1707(main.c)X
1227 816(cc)N
1371(-c)X
1515(main.c)X
843 912(parse.o)N
1611(:)X
1707(parse.c)X
1227 1008(cc)N
1371(-c)X
1515(parse.c)X
843 1104(output.o)N
1611(:)X
1707(output.c)X
1227 1200(cc)N
1371(-c)X
1515(output.c)X
1 f
755 1372(The)N
906(make\256le)X
1208(is)X
1287(now)X
1451(complete)X
1771(and)X
1913(will,)X
2083(in)X
2170(fact,)X
2336(create)X
2554(the)X
2677(program)X
2974(you)X
3119(want)X
3300(it)X
3369(to)X
3456(without)X
3725(unneces-)X
555 1468(sary)N
719(compilations)X
1162(or)X
1259(excessive)X
1597(typing)X
1831(on)X
1941(your)X
2118(part.)X
2293(There)X
2511(are)X
2640(two)X
2790(things)X
3015(wrong)X
3249(with)X
3420(it,)X
3513(however)X
3819(\(aside)X
555 1564(from)N
731(it)X
795(being)X
993(altogether)X
1334(too)X
1456(long,)X
1638(something)X
1991(I'll)X
2109(address)X
2370(in)X
2452(chapter)X
2709(3\):)X
555 1688(1\))N
755(The)X
907(string)X
1116(``)X
7 f
1170(main.o)X
1513(parse.o)X
1904(output.o)X
1 f
('')S
2368(is)X
2447(repeated)X
2746(twice,)X
2966(necessitating)X
3406(two)X
3552(changes)X
3837(when)X
755 1784(you)N
903(add)X
1047(post\256x)X
1292(\(you)X
1467(were)X
1652(planning)X
1960(on)X
2067(that,)X
2234(weren't)X
2507(you?\).)X
2737(This)X
2906(is)X
2986(in)X
3075(direct)X
3285(violation)X
3596(of)X
3690(de)X
3793(Boor's)X
755 1880(First)N
921(Rule)X
1092(of)X
1179(writing)X
1430(make\256les:)X
2 f
755 2004(Anything)N
1064(that)X
1208(needs)X
1411(to)X
1493(be)X
1589(written)X
1835(more)X
2020(than)X
2182(once)X
2354(should)X
2587(be)X
2683(placed)X
2917(in)X
2999(a)X
3059(variable.)X
1 f
755 2128(I)N
803(cannot)X
1038(emphasize)X
1398(this)X
1534(enough)X
1791(as)X
1879(being)X
2078(very)X
2242(important)X
2574(to)X
2657(the)X
2776(maintenance)X
3203(of)X
3291(a)X
3348(make\256le)X
3645(and)X
3782(its)X
3877(pro-)X
755 2224(gram.)N
555 2348(2\))N
755(There)X
968(is)X
1046(no)X
1151(way)X
1310(to)X
1397(alter)X
1564(the)X
1686(way)X
1844(compilations)X
2281(are)X
2404(performed)X
2763(short)X
2947(of)X
3038(editing)X
3284(the)X
3406(make\256le)X
3706(and)X
3846(mak-)X
755 2444(ing)N
881(the)X
1003(change)X
1255(in)X
1341(all)X
1445(places.)X
1690(This)X
1856(is)X
1933(evil)X
2077(and)X
2217(violates)X
2490(de)X
2590(Boor's)X
2831(Second)X
3090(Rule,)X
3284(which)X
3503(follows)X
3766(directly)X
755 2540(from)N
931(the)X
1049(\256rst:)X
2 f
755 2664(Any)N
908(\257ags)X
1087(or)X
1186(programs)X
1525(used)X
1700(inside)X
1919(a)X
1986(make\256le)X
2281(should)X
2521(be)X
2624(placed)X
2865(in)X
2954(a)X
3021(variable)X
3315(so)X
3413(they)X
3574(may)X
3735(be)X
755 2760(changed,)N
1067(temporarily)X
1465(or)X
1556(permanently,)X
1997(with)X
2154(the)X
2272(greatest)X
2550(ease.)X
1 f
755 2884(The)N
900(make\256le)X
1196(should)X
1429(more)X
1614(properly)X
1906(read:)X
7 f
843 3028(OBJS)N
1611(=)X
1707(main.o)X
2043(parse.o)X
2427(output.o)X
843 3124(expr)N
1611(:)X
1707($\(OBJS\))X
1227 3220($\(CC\))N
1515($\(CFLAGS\))X
1995(-o)X
2139($\(.TARGET\))X
2667($\(.ALLSRC\))X
843 3316(main.o)N
1611(:)X
1707(main.c)X
1227 3412($\(CC\))N
1515($\(CFLAGS\))X
1995(-c)X
2139(main.c)X
843 3508(parse.o)N
1611(:)X
1707(parse.c)X
1227 3604($\(CC\))N
1515($\(CFLAGS\))X
1995(-c)X
2139(parse.c)X
843 3700(output.o)N
1611(:)X
1707(output.c)X
1227 3796($\(CC\))N
1515($\(CFLAGS\))X
1995(-c)X
2139(output.c)X
843 3892($\(OBJS\))N
1611(:)X
1707(defs.h)X
1 f
555 4036(Alternatively,)N
1026(if)X
1103(you)X
1251(like)X
1399(the)X
1525(idea)X
1687(of)X
1782(dynamic)X
2086(sources)X
2355(mentioned)X
2721(in)X
2811(section)X
3066(2.3.1,)X
3274(you)X
3422(could)X
3628(write)X
3820(it)X
3891(like)X
555 4132(this:)N
7 f
843 4276(OBJS)N
1611(=)X
1707(main.o)X
2043(parse.o)X
2427(output.o)X
843 4372(expr)N
1611(:)X
1707($\(OBJS\))X
1227 4468($\(CC\))N
1515($\(CFLAGS\))X
1995(-o)X
2139($\(.TARGET\))X
2667($\(.ALLSRC\))X
843 4564($\(OBJS\))N
1611(:)X
1707($\(.PREFIX\).c)X
2331(defs.h)X
1227 4660($\(CC\))N
1515($\(CFLAGS\))X
1995(-c)X
2139($\(.PREFIX\).c)X
1 f
555 4804(These)N
767(two)X
907(rules)X
1083(and)X
1219(examples)X
1542(lead)X
1696(to)X
1778(de)X
1874(Boor's)X
2112(First)X
2278(Corollary:)X
2 f
755 4928(Variables)N
1086(are)X
1213(your)X
1380(friends.)X
1 f
755 5052(Once)N
951(you've)X
1200(written)X
1453(the)X
1577(make\256le)X
1879(comes)X
2110(the)X
2234(sometimes-dif\256cult)X
2882(task)X
3037(of)X
3130(making)X
3396(sure)X
3556(the)X
3679(darn)X
3847(thing)X
555 5148(works.)N
803(Your)X
999(most)X
1185(helpful)X
1443(tool)X
1598(to)X
1691(make)X
1896(sure)X
2061(the)X
2190(make\256le)X
2497(is)X
2581(at)X
2670(least)X
2848(syntactically)X
3284(correct)X
3539(is)X
3623(the)X
3 f
9 f
3752(-)X
3 f
3796(n)X
1 f
3871(\257ag,)X
555 5244(which)N
776(allows)X
1010(you)X
1155(to)X
1242(see)X
1370(if)X
1444(PMake)X
1696(will)X
1845(choke)X
2062(on)X
2167(the)X
2290(make\256le.)X
2611(The)X
2761(second)X
3009(thing)X
3197(the)X
3 f
9 f
3319(-)X
3 f
3363(n)X
1 f
3431(\257ag)X
3575(lets)X
3710(you)X
3854(do)X
3958(is)X
555 5340(see)N
683(what)X
863(PMake)X
1114(would)X
1338(do)X
1442(without)X
1710(it)X
1778(actually)X
2056(doing)X
2262(it,)X
2350(thus)X
2507(you)X
2651(can)X
2787(make)X
2985(sure)X
3143(the)X
3265(right)X
3440(commands)X
3811(would)X
555 5436(be)N
651(executed)X
957(were)X
1134(you)X
1274(to)X
1356(give)X
1514(PMake)X
1761(its)X
1856(head.)X
755 5560(When)N
980(you)X
1133(\256nd)X
1289(your)X
1468(make\256le)X
1776(isn't)X
1950(behaving)X
2276(as)X
2375(you)X
2527(hoped,)X
2775(the)X
2905(\256rst)X
3061(question)X
3364(that)X
3516(comes)X
3753(to)X
3847(mind)X
555 5656(\(after)N
755(``What)X
1008(time)X
1175(is)X
1253(it,)X
1342(anyway?''\))X
1734(is)X
1812(``Why)X
2047(not?'')X
2264(In)X
2356(answering)X
2711(this,)X
2871(two)X
3016(\257ags)X
3191(will)X
3339(serve)X
3533(you)X
3677(well:)X
3861(``)X
7 f
3915(-d)X
555 5752(m)N
1 f
('')S
678(and)X
815(``)X
7 f
869(-p)X
1014(2)X
1 f
(.'')S
1157(The)X
1303(\256rst)X
1448(causes)X
1679(PMake)X
1927(to)X
2010(tell)X
2133(you)X
2274(as)X
2362(it)X
2427(examines)X
2751(each)X
2920(target)X
3124(in)X
3207(the)X
3325(make\256le)X
3621(and)X
3757(indicate)X
555 5848(why)N
716(it)X
783(is)X
859(deciding)X
1157(whatever)X
1474(it)X
1540(is)X
1615(deciding.)X
1933(You)X
2093(can)X
2227(then)X
2387(use)X
2516(the)X
2636(information)X
3036(printed)X
3285(for)X
3401(other)X
3588(targets)X
3824(to)X
3908(see)X

10 p
%%Page: 10 11
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(10)X
2343(-)X
555 672(where)N
775(you)X
918(went)X
1097(wrong.)X
1345(The)X
1493(``)X
7 f
1547(-p)X
1694(2)X
1 f
('')S
1819(\257ag)X
1962(makes)X
2190(PMake)X
2440(print)X
2614(out)X
2738(its)X
2835(internal)X
3102(state)X
3271(when)X
3467(it)X
3533(is)X
3608(done,)X
3806(allow-)X
555 768(ing)N
684(you)X
831(to)X
920(see)X
1050(that)X
1197(you)X
1344(forgot)X
1567(to)X
1656(make)X
1857(that)X
2004(one)X
2147(chapter)X
2411(depend)X
2669(on)X
2775(that)X
2921(\256le)X
3049(of)X
3142(macros)X
3400(you)X
3546(just)X
3687(got)X
3815(a)X
3877(new)X
555 864(version)N
818(of.)X
931(The)X
1082(output)X
1312(from)X
1494(``)X
7 f
1548(-p)X
1698(2)X
1 f
('')S
1826(is)X
1905(intended)X
2207(to)X
2295(resemble)X
2611(closely)X
2864(a)X
2926(real)X
3073(make\256le,)X
3395(but)X
3523(with)X
3691(additional)X
555 960(information)N
953(provided)X
1258(and)X
1394(with)X
1556(variables)X
1866(expanded)X
2194(in)X
2276(those)X
2465(commands)X
2832(PMake)X
3079(actually)X
3353(printed)X
3600(or)X
3687(executed.)X
755 1084(Something)N
1121(to)X
1203(be)X
1299(especially)X
1640(careful)X
1884(about)X
2082(is)X
2155(circular)X
2421(dependencies.)X
2914(E.g.)X
7 f
843 1228(a)N
1243(:)X
1339(b)X
843 1324(b)N
1243(:)X
1339(c)X
1435(d)X
843 1420(d)N
1243(:)X
1339(a)X
1 f
555 1564(In)N
646(this)X
785(case,)X
968(because)X
1247(of)X
1338(how)X
1500(PMake)X
1751(works,)X
7 f
1991(c)X
1 f
2063(is)X
2140(the)X
2262(only)X
2428(thing)X
2616(PMake)X
2866(will)X
3013(examine,)X
3328(because)X
7 f
3606(d)X
1 f
3677(and)X
7 f
3816(a)X
1 f
3887(will)X
555 1660(effectively)N
925(fall)X
1058(off)X
1177(the)X
1300(edge)X
1477(of)X
1569(the)X
1692(universe,)X
2009(making)X
2274(it)X
2343(impossible)X
2714(to)X
2801(examine)X
7 f
3098(b)X
1 f
3171(\(or)X
3290(them,)X
3495(for)X
3614(that)X
3759(matter\).)X
555 1756(PMake)N
805(will)X
952(tell)X
1077(you)X
1220(\(if)X
1319(run)X
1449(in)X
1534(its)X
1632(normal)X
1882(mode\))X
2110(all)X
2213(the)X
2334(targets)X
2571(involved)X
2873(in)X
2957(any)X
3095(cycle)X
3287(it)X
3353(looked)X
3593(at)X
3673(\(i.e.)X
3820(if)X
3891(you)X
555 1852(have)N
731(two)X
875(cycles)X
1100(in)X
1186(the)X
1308(graph)X
1515(\(naughty,)X
1843(naughty\),)X
2171(but)X
2296(only)X
2461(try)X
2573(to)X
2658(make)X
2855(a)X
2914(target)X
3120(in)X
3205(one)X
3344(of)X
3434(them,)X
3637(PMake)X
3887(will)X
555 1948(only)N
724(tell)X
853(you)X
1000(about)X
1205(that)X
1352(one.)X
1515(You'll)X
1751(have)X
1930(to)X
2019(try)X
2135(to)X
2224(make)X
2424(the)X
2548(other)X
2739(to)X
2827(\256nd)X
2977(the)X
3101(second)X
3350(cycle\).)X
3593(When)X
3811(run)X
3944(as)X
555 2044(Make,)N
778(it)X
842(will)X
986(only)X
1148(print)X
1319(the)X
1437(\256rst)X
1581(target)X
1784(in)X
1866(the)X
1984(cycle.)X
3 f
555 2236(2.7.)N
715(Invoking)X
1040(PMake)X
1 f
755 2360(PMake)N
1012(comes)X
1247(with)X
1419(a)X
1485(wide)X
1671(variety)X
1924(of)X
2021(\257ags)X
2202(to)X
2294(choose)X
2547(from.)X
2773(They)X
2968(may)X
3136(appear)X
3381(in)X
3473(any)X
3618(order,)X
3837(inter-)X
555 2456(spersed)N
816(with)X
978(command-line)X
1461(variable)X
1740(assignments)X
2151(and)X
2287(targets)X
2521(to)X
2603(create.)X
2856(The)X
3001(\257ags)X
3172(are)X
3291(as)X
3378(follows:)X
3 f
9 f
555 2580(-)N
3 f
599(d)X
2 f
663(what)X
1 f
755 2676(This)N
926(causes)X
1165(PMake)X
1421(to)X
1512(spew)X
1706(out)X
1837(debugging)X
2204(information)X
2611(that)X
2760(may)X
2927(prove)X
3138(useful)X
3362(to)X
3452(you.)X
3620(If)X
3702(you)X
3850(can't)X
755 2772(\256gure)N
962(out)X
1084(why)X
1242(PMake)X
1489(is)X
1562(doing)X
1764(what)X
1940(it's)X
2062(doing,)X
2284(you)X
2424(might)X
2630(try)X
2739(using)X
2932(this)X
3067(\257ag.)X
3227(The)X
2 f
3372(what)X
1 f
3560(parameter)X
3902(is)X
3975(a)X
755 2868(string)N
958(of)X
1046(single)X
1258(characters)X
1606(that)X
1747(tell)X
1870(PMake)X
2118(what)X
2295(aspects)X
2548(you)X
2689(are)X
2809(interested)X
3142(in.)X
3245(Most)X
3430(of)X
3518(what)X
3695(I)X
3743(describe)X
755 2964(will)N
904(make)X
1103(little)X
1274(sense)X
1473(to)X
1559(you,)X
1723(unless)X
1947(you've)X
2194(dealt)X
2374(with)X
2540(Make)X
2747(before.)X
2997(Just)X
3145(remember)X
3495(where)X
3716(this)X
3855(table)X
755 3060(is)N
834(and)X
976(come)X
1176(back)X
1354(to)X
1442(it)X
1512(as)X
1605(you)X
1751(read)X
1916(on.)X
2062(The)X
2213(characters)X
2565(and)X
2706(the)X
2829(information)X
3232(they)X
3395(produce)X
3679(are)X
3803(as)X
3895(fol-)X
755 3156(lows:)N
755 3280(a)N
955(Archive)X
1234(searching)X
1562(and)X
1698(caching.)X
755 3404(c)N
955(Conditional)X
1352(evaluation.)X
755 3528(d)N
955(The)X
1100(searching)X
1428(and)X
1564(caching)X
1834(of)X
1921(directories.)X
755 3652(j)N
955(Various)X
1236(snippets)X
1525(of)X
1619(information)X
2024(related)X
2270(to)X
2359(the)X
2483(running)X
2758(of)X
2851(the)X
2975(multiple)X
3267(shells.)X
3495(Not)X
3641(particularly)X
955 3748(interesting.)N
755 3872(m)N
955(The)X
1104(making)X
1368(of)X
1459(each)X
1630(target:)X
1858(what)X
2037(target)X
2243(is)X
2319(being)X
2520(examined;)X
2877(when)X
3074(it)X
3141(was)X
3289(last)X
3423(modi\256ed;)X
3752(whether)X
955 3968(it)N
1019(is)X
1092(out-of-date;)X
1491(etc.)X
755 4092(p)N
955(Make\256le)X
1260(parsing.)X
755 4216(r)N
955(Remote)X
1224(execution.)X
755 4340(s)N
955(The)X
1100(application)X
1476(of)X
1563(suf\256x-transformation)X
2264(rules.)X
2460(\(See)X
2623(chapter)X
2880(3\))X
755 4464(t)N
955(The)X
1100(maintenance)X
1526(of)X
1613(the)X
1731(list)X
1848(of)X
1935(targets.)X
755 4588(v)N
955(Variable)X
1252(assignment.)X
755 4712(Of)N
869(these)X
1063(all,)X
1192(the)X
7 f
1319(m)X
1 f
1396(and)X
7 f
1541(s)X
1 f
1618(letters)X
1843(will)X
1996(be)X
2101(most)X
2284(useful)X
2508(to)X
2598(you.)X
2786(If)X
2868(the)X
3 f
9 f
2994(-)X
3 f
3038(d)X
1 f
3110(is)X
3191(the)X
3317(\256nal)X
3487(argument)X
3818(or)X
3913(the)X
755 4808(argument)N
1090(from)X
1278(which)X
1506(it)X
1582(would)X
1814(get)X
1944(these)X
2141(key)X
2289(letters)X
2517(\(see)X
2679(below)X
2907(for)X
3033(a)X
3101(note)X
3271(about)X
3481(which)X
3708(argument)X
755 4904(would)N
976(be)X
1073(used\))X
1268(begins)X
1498(with)X
1661(a)X
3 f
9 f
1718(-)X
1 f
1762(,)X
1803(all)X
1904(of)X
1992(these)X
2178(debugging)X
2537(\257ags)X
2709(will)X
2854(be)X
2951(set,)X
3080(resulting)X
3380(in)X
3462(massive)X
3740(amounts)X
755 5000(of)N
842(output.)X
3 f
9 f
555 5124(-)N
3 f
599(f)X
2 f
646(make\256le)X
1 f
755 5220(Specify)N
1032(a)X
1100(make\256le)X
1407(to)X
1500(read)X
1670(different)X
1978(from)X
2165(the)X
2294(standard)X
2597(make\256les)X
2935(\()X
7 f
2962(Makefile)X
1 f
3377(or)X
7 f
3475(makefile)X
1 f
(\).)S
3957(If)X
2 f
755 5316(make\256le)N
1 f
1081(is)X
1178(``)X
9 f
1232(-)X
1 f
1276('',)X
1394(PMake)X
1665(uses)X
1847(the)X
1989(standard)X
2305(input.)X
2533(This)X
2719(is)X
2816(useful)X
3056(for)X
3194(making)X
3478(quick)X
3700(and)X
3860(dirty)X
755 5412(make\256les.)N
1095(.)X
1128(.)X
3 f
9 f
555 5536(-)N
3 f
599(h)X
1 f
755(Prints)X
962(out)X
1085(a)X
1142(summary)X
1461(of)X
1549(the)X
1668(various)X
1925(\257ags)X
2097(PMake)X
2345(accepts.)X
2623(It)X
2693(can)X
2826(also)X
2976(be)X
3073(used)X
3241(to)X
3324(\256nd)X
3469(out)X
3592(what)X
3768(level)X
3944(of)X
755 5632(concurrency)N
1176(was)X
1324(compiled)X
1645(into)X
1792(the)X
1913(version)X
2172(of)X
2262(PMake)X
2512(you)X
2655(are)X
2777(using)X
2973(\(look)X
3165(at)X
3 f
9 f
3246(-)X
3 f
3290(J)X
1 f
3353(and)X
3 f
9 f
3491(-)X
3 f
3535(L)X
1 f
3588(\))X
3637(and)X
3775(various)X
755 5728(other)N
940(information)X
1338(on)X
1438(how)X
1596(PMake)X
1843(was)X
1988(con\256gured.)X

11 p
%%Page: 11 12
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(11)X
2343(-)X
3 f
9 f
555 672(-)N
3 f
599(i)X
1 f
755(If)X
831(you)X
973(give)X
1133(this)X
1270(\257ag,)X
1432(PMake)X
1681(will)X
1827(ignore)X
2054(non-zero)X
2362(status)X
2566(returned)X
2856(by)X
2958(any)X
3096(of)X
3185(its)X
3282(shells.)X
3506(It's)X
3634(like)X
3775(placing)X
755 768(a)N
811(`)X
9 f
838(-)X
1 f
882(')X
929(before)X
1155(all)X
1255(the)X
1373(commands)X
1740(in)X
1822(the)X
1940(make\256le.)X
3 f
9 f
555 892(-)N
3 f
599(k)X
1 f
755(This)X
924(is)X
1003(similar)X
1251(to)X
3 f
9 f
1339(-)X
3 f
1383(i)X
1 f
1431(in)X
1519(that)X
1665(it)X
1735(allows)X
1970(PMake)X
2223(to)X
2311(continue)X
2613(when)X
2813(it)X
2883(sees)X
3043(an)X
3145(error,)X
3348(but)X
3476(unlike)X
3 f
9 f
3702(-)X
3 f
3746(i)X
1 f
3768(,)X
3814(where)X
755 988(PMake)N
1004(continues)X
1333(blithely)X
1599(as)X
1688(if)X
1759(nothing)X
2025(went)X
2203(wrong,)X
3 f
9 f
2449(-)X
3 f
2493(k)X
1 f
2558(causes)X
2789(it)X
2854(to)X
2937(recognize)X
3271(the)X
3390(error)X
3568(and)X
3705(only)X
3868(con-)X
755 1084(tinue)N
949(work)X
1148(on)X
1262(those)X
1465(things)X
1694(that)X
1848(don't)X
2051(depend)X
2317(on)X
2431(the)X
2563(target,)X
2800(either)X
3017(directly)X
3295(or)X
3395(indirectly)X
3735(\(through)X
755 1180(depending)N
1112(on)X
1215(something)X
1571(that)X
1714(depends)X
1999(on)X
2101(it\),)X
2214(whose)X
2441(creation)X
2722(returned)X
3012(the)X
3132(error.)X
3351(The)X
3498(`k')X
3614(is)X
3689(for)X
3805(``keep)X
755 1276(going''.)N
1024(.)X
1057(.)X
3 f
9 f
555 1400(-)N
3 f
599(l)X
1 f
755(PMake)X
1007(has)X
1139(the)X
1261(ability)X
1489(to)X
1575(lock)X
1737(a)X
1797(directory)X
2111(against)X
2362(other)X
2551(people)X
2789(executing)X
3125(it)X
3193(in)X
3279(the)X
3401(same)X
3590(directory)X
3904(\(by)X
755 1496(means)N
983(of)X
1073(a)X
1132(\256le)X
1257(called)X
1472(``LOCK.make'')X
2015(that)X
2158(it)X
2225(creates)X
2472(and)X
2611(checks)X
2853(for)X
2970(in)X
3055(the)X
3176(directory\).)X
3536(This)X
3700(is)X
3775(a)X
3833(Good)X
755 1592(Thing)N
968(because)X
1245(two)X
1387(people)X
1623(doing)X
1827(the)X
1947(same)X
2134(thing)X
2320(in)X
2404(the)X
2524(same)X
2711(place)X
2903(can)X
3037(be)X
3135(disastrous)X
3477(for)X
3593(the)X
3713(\256nal)X
3877(pro-)X
755 1688(duct)N
915(\(too)X
1066(many)X
1266(cooks)X
1475(and)X
1613(all)X
1715(that\).)X
1924(Whether)X
2223(this)X
2360(locking)X
2622(is)X
2697(the)X
2817(default)X
3062(is)X
3136(up)X
3237(to)X
3320(your)X
3488(system)X
3731(adminis-)X
755 1784(trator.)N
974(If)X
1053(locking)X
1318(is)X
1396(on,)X
3 f
9 f
1520(-)X
3 f
1564(l)X
1 f
1610(will)X
1758(turn)X
1911(it)X
1979(off,)X
2117(and)X
2257(vice)X
2415(versa.)X
2629(Note)X
2809(that)X
2953(this)X
3092(locking)X
3356(will)X
3504(not)X
3630(prevent)X
2 f
3895(you)X
1 f
755 1880(from)N
942(invoking)X
1257(PMake)X
1515(twice)X
1719(in)X
1811(the)X
1939(same)X
2134(place)X
2334(\320)X
2444(if)X
2523(you)X
2673(own)X
2841(the)X
2969(lock)X
3137(\256le,)X
3289(PMake)X
3546(will)X
3700(warn)X
3891(you)X
755 1976(about)N
953(it)X
1017(but)X
1139(continue)X
1435(to)X
1517(execute.)X
3 f
9 f
555 2100(-)N
3 f
599(n)X
1 f
755(This)X
925(\257ag)X
1073(tells)X
1234(PMake)X
1489(not)X
1619(to)X
1709(execute)X
1983(the)X
2109(commands)X
2484(needed)X
2740(to)X
2830(update)X
3072(the)X
3198(out-of-date)X
3583(targets)X
3824(in)X
3913(the)X
755 2196(make\256le.)N
1080(Rather,)X
1343(PMake)X
1599(will)X
1752(simply)X
1998(print)X
2178(the)X
2305(commands)X
2681(it)X
2754(would)X
2982(have)X
3162(executed)X
3476(and)X
3620(exit.)X
3788(This)X
3958(is)X
755 2292(particularly)N
1145(useful)X
1361(for)X
1475(checking)X
1785(the)X
1903(correctness)X
2285(of)X
2372(a)X
2428(make\256le.)X
2744(If)X
2818(PMake)X
3065(doesn't)X
3321(do)X
3421(what)X
3597(you)X
3737(expect)X
3967(it)X
755 2388(to,)N
857(it's)X
979(a)X
1035(good)X
1215(chance)X
1459(the)X
1577(make\256le)X
1873(is)X
1946(wrong.)X
3 f
9 f
555 2512(-)N
3 f
599(p)X
2 f
663(number)X
1 f
755 2608(This)N
928(causes)X
1169(PMake)X
1427(to)X
1520(print)X
1702(its)X
1808(input)X
2003(in)X
2096(a)X
2163(reasonable)X
2538(form,)X
2744(though)X
2996(not)X
3128(necessarily)X
3515(one)X
3661(that)X
3811(would)X
755 2704(make)N
956(immediate)X
1321(sense)X
1522(to)X
1611(anyone)X
1870(but)X
1999(me.)X
2144(The)X
2 f
2296(number)X
1 f
2581(is)X
2661(a)X
2724(bitwise-or)X
3076(of)X
3170(1)X
3237(and)X
3380(2)X
3447(where)X
3670(1)X
3736(means)X
3967(it)X
755 2800(should)N
994(print)X
1171(the)X
1295(input)X
1485(before)X
1717(doing)X
1925(any)X
2067(processing)X
2436(and)X
2578(2)X
2644(says)X
2808(it)X
2878(should)X
3117(print)X
3294(it)X
3363(after)X
3536(everything)X
3904(has)X
755 2896(been)N
930 0.3375(re-created.)AX
1296(Thus)X
7 f
9 f
1479(-)X
7 f
1523(p)X
1622(3)X
1 f
1692(would)X
1914(print)X
2087(it)X
2153(twice\320once)X
2581(before)X
2809(processing)X
3174(and)X
3312(once)X
3486(after)X
3656(\(you)X
3825(might)X
755 2992(\256nd)N
910(the)X
1039(difference)X
1397(between)X
1696(the)X
1825(two)X
1976(interesting\).)X
2392(This)X
2565(is)X
2649(mostly)X
2897(useful)X
3123(to)X
3215(me,)X
3363(but)X
3495(you)X
3645(may)X
3813(\256nd)X
3967(it)X
755 3088(informative)N
1149(in)X
1231(some)X
1420(bizarre)X
1664(circumstances.)X
3 f
9 f
555 3212(-)N
3 f
599(q)X
1 f
755(If)X
833(you)X
977(give)X
1138(PMake)X
1388(this)X
1526(\257ag,)X
1689(it)X
1756(will)X
1903(not)X
2028(try)X
2140(to)X
2225 0.4219(re-create)AX
2531(anything.)X
2854(It)X
2926(will)X
3073(just)X
3211(see)X
3337(if)X
3409(anything)X
3712(is)X
3788(out-of-)X
755 3308(date)N
909(and)X
1045(exit)X
1185(non-zero)X
1491(if)X
1560(so.)X
3 f
9 f
555 3432(-)N
3 f
599(r)X
1 f
755(When)X
970(PMake)X
1220(starts)X
1412(up,)X
1535(it)X
1602(reads)X
1795(a)X
1853(default)X
2098(make\256le)X
2396(that)X
2538(tells)X
2693(it)X
2759(what)X
2937(sort)X
3079(of)X
3168(system)X
3412(it's)X
3536(on)X
3638(and)X
3776(gives)X
3967(it)X
755 3528(some)N
945(idea)X
1100(of)X
1188(what)X
1365(to)X
1448(do)X
1549(if)X
1619(you)X
1760(don't)X
1950(tell)X
2073(it)X
2138(anything.)X
2459(I'll)X
2578(tell)X
2701(you)X
2842(about)X
3041(it)X
3105(in)X
3187(chapter)X
3444(3.)X
3524(If)X
3598(you)X
3738(give)X
3896(this)X
755 3624(\257ag,)N
915(PMake)X
1162(won't)X
1369(read)X
1528(the)X
1646(default)X
1889(make\256le.)X
3 f
9 f
555 3748(-)N
3 f
599(s)X
1 f
755(This)X
921(causes)X
1155(PMake)X
1406(to)X
1492(not)X
1617(print)X
1791(commands)X
2161(before)X
2390(they're)X
2641(executed.)X
2970(It)X
3042(is)X
3118(the)X
3239(equivalent)X
3596(of)X
3686(putting)X
3935(an)X
755 3844(`@')N
903(before)X
1129(every)X
1328(command)X
1664(in)X
1746(the)X
1864(make\256le.)X
3 f
9 f
555 3968(-)N
3 f
599(t)X
1 f
755(Rather)X
997(than)X
1163(try)X
1280(to)X
1370 0.4219(re-create)AX
1681(a)X
1745(target,)X
1976(PMake)X
2231(will)X
2383(simply)X
2628(``touch'')X
2942(it)X
3014(so)X
3113(as)X
3208(to)X
3298(make)X
3500(it)X
3572(appear)X
3815(up-to-)X
755 4064(date.)N
935(If)X
1015(the)X
1139(target)X
1348(didn't)X
1565(exist)X
1742(before,)X
1994(it)X
2064(will)X
2214(when)X
2414(PMake)X
2667(\256nishes,)X
2957(but)X
3085(if)X
3160(the)X
3284(target)X
3493(did)X
3621(exist,)X
3818(it)X
3887(will)X
755 4160(appear)N
990(to)X
1072(have)X
1244(been)X
1416(updated.)X
3 f
9 f
555 4284(-)N
3 f
599(v)X
1 f
755(This)X
919(is)X
994(a)X
1052(mixed-compatibility)X
1726(\257ag)X
1867(intended)X
2164(to)X
2247(mimic)X
2472(the)X
2591(System)X
2847(V)X
2926(version)X
3183(of)X
3271(Make.)X
3495(It)X
3565(is)X
3639(the)X
3758(same)X
3944(as)X
755 4380(giving)N
3 f
9 f
987(-)X
3 f
1031(B)X
1 f
1084(,)X
1132(and)X
3 f
9 f
1276(-)X
3 f
1320(V)X
1 f
1406(as)X
1501(well)X
1667(as)X
1762(turning)X
2020(off)X
2141(directory)X
2458(locking.)X
2745(Targets)X
3013(can)X
3152(still)X
3298(be)X
3401(created)X
3661(in)X
3750(parallel,)X
755 4476(however.)N
1072(This)X
1234(is)X
1307(the)X
1425(mode)X
1623(PMake)X
1870(will)X
2014(enter)X
2195(if)X
2264(it)X
2328(is)X
2401(invoked)X
2679(either)X
2882(as)X
2969(``)X
7 f
3023(smake)X
1 f
('')S
3337(or)X
3424(``)X
7 f
3478(vmake)X
1 f
(''.)S
3 f
9 f
555 4600(-)N
3 f
599(x)X
1 f
755(This)X
918(tells)X
1072(PMake)X
1320(it's)X
1442(ok)X
1542(to)X
1624(export)X
1849(jobs)X
2002(to)X
2084(other)X
2269(machines,)X
2612(if)X
2681(they're)X
2929(available.)X
3259(It)X
3328(is)X
3401(used)X
3568(when)X
3762(running)X
755 4696(in)N
838(Make)X
1042(mode,)X
1261(as)X
1349(exporting)X
1677(in)X
1760(this)X
1896(mode)X
2095(tends)X
2285(to)X
2368(make)X
2563(things)X
2779(run)X
2907(slower)X
3142(than)X
3300(if)X
3369(the)X
3487(commands)X
3854(were)X
755 4792(just)N
890(executed)X
1196(locally.)X
3 f
9 f
555 4916(-)N
3 f
599(B)X
1 f
755(Forces)X
999(PMake)X
1256(to)X
1348(be)X
1454(as)X
1551(backwards-compatible)X
2308(with)X
2479(Make)X
2691(as)X
2787(possible)X
3078(while)X
3285(still)X
3433(being)X
3640(itself.)X
3869(This)X
755 5012(includes:)N
10 f
755 5136(g)N
1 f
835(Executing)X
1180(one)X
1316(shell)X
1487(per)X
1610(shell)X
1781(command)X
10 f
755 5260(g)N
1 f
835(Expanding)X
1209(anything)X
1516(that)X
1663(looks)X
1863(even)X
2042(vaguely)X
2323(like)X
2470(a)X
2533(variable,)X
2839(with)X
3008(the)X
3133(empty)X
3360(string)X
3569(replacing)X
3895(any)X
835 5356(variable)N
1114(PMake)X
1361(doesn't)X
1617(know.)X
10 f
755 5480(g)N
1 f
835(Refusing)X
1144(to)X
1226(allow)X
1424(you)X
1564(to)X
1646(escape)X
1881(a)X
1937(`#')X
2051(with)X
2213(a)X
2269(backslash.)X
10 f
755 5604(g)N
1 f
835(Permitting)X
1201(unde\256ned)X
1546(variables)X
1865(on)X
1974(dependency)X
2387(lines)X
2567(and)X
2712(conditionals)X
3132(\(see)X
3290(below\).)X
3561(Normally)X
3896(this)X
835 5700(causes)N
1065(PMake)X
1312(to)X
1394(abort.)X

12 p
%%Page: 12 13
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(12)X
2343(-)X
3 f
9 f
555 672(-)N
3 f
599(C)X
1 f
755(This)X
920(nulli\256es)X
1200(any)X
1339(and)X
1478(all)X
1581(compatibility)X
2030(mode)X
2231(\257ags)X
2405(you)X
2548(may)X
2709(have)X
2884(given)X
3085(or)X
3175(implied)X
3442(up)X
3545(to)X
3629(the)X
3749(time)X
3913(the)X
3 f
9 f
755 768(-)N
3 f
799(C)X
1 f
885(is)X
966(encountered.)X
1407(It)X
1484(is)X
1565(useful)X
1789(mostly)X
2034(in)X
2124(a)X
2188(make\256le)X
2492(that)X
2640(you)X
2788(wrote)X
2999(for)X
3121(PMake)X
3376(to)X
3466(avoid)X
3672(bad)X
3816(things)X
755 864(happening)N
1112(when)X
1309(someone)X
1617(runs)X
1778(PMake)X
2028(as)X
2118(``)X
7 f
2172(make)X
1 f
('')S
2440(or)X
2529(has)X
2658(things)X
2875(set)X
2986(in)X
3070(the)X
3190(environment)X
3617(that)X
3759(tell)X
3883(it)X
3949(to)X
755 960(be)N
869(compatible.)X
3 f
9 f
1303(-)X
3 f
1347(C)X
1 f
1443(is)X
2 f
1534(not)X
1 f
1687(placed)X
1935(in)X
2035(the)X
7 f
2171(PMAKE)X
1 f
2449(environment)X
2892(variable)X
3188(or)X
3292(the)X
7 f
3427(.MAKEFLAGS)X
1 f
3944(or)X
7 f
755 1056(MFLAGS)N
1 f
1063(global)X
1283(variables.)X
3 f
9 f
555 1180(-)N
3 f
599(D)X
2 f
677(variable)X
1 f
755 1276(Allows)N
1015(you)X
1164(to)X
1255(de\256ne)X
1480(a)X
1544(variable)X
1831(to)X
1921(have)X
2101(``)X
7 f
2155(1)X
1 f
('')S
2285(as)X
2380(its)X
2483(value.)X
2725(The)X
2878(variable)X
3165(is)X
3246(a)X
3310(global)X
3538(variable,)X
3845(not)X
3975(a)X
755 1372(command-line)N
1244(variable.)X
1549(This)X
1717(is)X
1796(useful)X
2018(mostly)X
2261(for)X
2381(people)X
2620(who)X
2783(are)X
2907(used)X
3079(to)X
3166(the)X
3289(C)X
3367(compiler)X
3677(arguments)X
755 1468(and)N
891(those)X
1080(using)X
1273(conditionals,)X
1704(which)X
1920(I'll)X
2038(get)X
2156(into)X
2300(in)X
2382(section)X
2629(4.3)X
3 f
9 f
555 1592(-)N
3 f
599(I)X
2 f
650(directory)X
1 f
755 1688(Tells)N
946(PMake)X
1204(another)X
1476(place)X
1677(to)X
1770(search)X
2007(for)X
2132(included)X
2439(make\256les.)X
2797(Yet)X
2944(another)X
3215(thing)X
3409(to)X
3501(be)X
3607(explained)X
3949(in)X
755 1784(chapter)N
1012(3)X
1072(\(section)X
1346(3.2,)X
1486(to)X
1568(be)X
1664(precise\).)X
3 f
9 f
555 1908(-)N
3 f
599(J)X
2 f
659(number)X
1 f
755 2004(Gives)N
962(the)X
1080(absolute)X
1367(maximum)X
1711(number)X
1976(of)X
2063(targets)X
2297(to)X
2379(create)X
2592(at)X
2670(once)X
2842(on)X
2942(both)X
3104(local)X
3280(and)X
3416(remote)X
3659(machines.)X
3 f
9 f
555 2128(-)N
3 f
599(L)X
2 f
672(number)X
1 f
755 2224(This)N
919(speci\256es)X
1217(the)X
1337(maximum)X
1683(number)X
1950(of)X
2038(targets)X
2273(to)X
2356(create)X
2570(on)X
2671(the)X
2790(local)X
2967(machine)X
3260(at)X
3339(once.)X
3532(This)X
3695(may)X
3854(be)X
3951(0,)X
755 2320(though)N
998(you)X
1139(should)X
1373(be)X
1470(wary)X
1652(of)X
1740(doing)X
1943(this,)X
2099(as)X
2187(PMake)X
2435(may)X
2594(hang)X
2770(until)X
2936(a)X
2992(remote)X
3235(machine)X
3527(becomes)X
3828(avail-)X
755 2416(able,)N
929(if)X
998(one)X
1134(is)X
1207(not)X
1329(available)X
1639(when)X
1833(it)X
1897(is)X
1970(started.)X
3 f
9 f
555 2540(-)N
3 f
599(M)X
1 f
755(This)X
923(is)X
1002(the)X
1126(\257ag)X
1272(that)X
1418(provides)X
1719(absolute,)X
2031(complete,)X
2370(full)X
2506(compatibility)X
2957(with)X
3124(Make.)X
3352(It)X
3426(still)X
3570(allows)X
3804(you)X
3949(to)X
755 2636(use)N
889(all)X
996(but)X
1125(a)X
1188(few)X
1336(of)X
1430(the)X
1555(features)X
1837(of)X
1931(PMake,)X
2205(but)X
2333(it)X
2403(is)X
2482(non-parallel.)X
2916(This)X
3084(is)X
3163(the)X
3287(mode)X
3491(PMake)X
3744(enters)X
3962(if)X
755 2732(you)N
895(call)X
1031(it)X
1095(``)X
7 f
1149(make)X
1 f
(.'')S
3 f
9 f
555 2856(-)N
3 f
599(P)X
1 f
755(When)X
971(creating)X
1254(targets)X
1492(in)X
1577(parallel,)X
1861(several)X
2112(shells)X
2317(are)X
2439(executing)X
2774(at)X
2855(once,)X
3050(each)X
3221(wanting)X
3502(to)X
3587(write)X
3775(its)X
3873(own)X
755 2952(two)N
905(cent's-worth)X
1341(to)X
1433(the)X
1561(screen.)X
1837(This)X
2009(output)X
2243(must)X
2428(be)X
2534(captured)X
2841(by)X
2951(PMake)X
3207(in)X
3298(some)X
3496(way)X
3659(in)X
3750(order)X
3949(to)X
755 3048(prevent)N
1026(the)X
1154(screen)X
1390(from)X
1576(being)X
1784(\256lled)X
1978(with)X
2150(garbage)X
2435(even)X
2616(more)X
2810(indecipherable)X
3312(than)X
3479(you)X
3628(usually)X
3888(see.)X
755 3144(PMake)N
1017(has)X
1159(two)X
1313(ways)X
1512(of)X
1613(doing)X
1829(this,)X
1998(one)X
2148(of)X
2249(which)X
2479(provides)X
2789(for)X
2917(much)X
3129(cleaner)X
3396(output)X
3634(and)X
3784(a)X
3854(clear)X
755 3240(separation)N
1122(between)X
1427(the)X
1562(output)X
1803(of)X
1907(different)X
2221(jobs,)X
2411(the)X
2546(other)X
2748(of)X
2852(which)X
3085(provides)X
3398(a)X
3471(more)X
3673(immediate)X
755 3336(response)N
1064(so)X
1163(one)X
1307(can)X
1447(tell)X
1577(what)X
1761(is)X
1842(really)X
2053(happpening.)X
2475(The)X
2628(former)X
2875(is)X
2955(done)X
3138(by)X
3245(notifying)X
3565(you)X
3712(when)X
3913(the)X
755 3432(creation)N
1040(of)X
1133(a)X
1195(target)X
1404(starts,)X
1618(capturing)X
1946(the)X
2069(output)X
2298(and)X
2439(transferring)X
2839(it)X
2908(to)X
2995(the)X
3118(screen)X
3349(all)X
3454(at)X
3537(once)X
3714(when)X
3913(the)X
755 3528(job)N
883(\256nishes.)X
1173(The)X
1324(latter)X
1515(is)X
1593(done)X
1774(by)X
1879(catching)X
2176(the)X
2299(output)X
2528(of)X
2620(the)X
2743(shell)X
2919(\(and)X
3087(its)X
3187(children\))X
3502(and)X
3643(buffering)X
3967(it)X
755 3624(until)N
930(an)X
1035(entire)X
1247(line)X
1396(is)X
1478(received,)X
1800(then)X
1967(printing)X
2249(that)X
2398(line)X
2547(preceded)X
2867(by)X
2976(an)X
3080(indication)X
3428(of)X
3523(which)X
3747(job)X
3877(pro-)X
755 3720(duced)N
974(the)X
1099(output.)X
1350(Since)X
1555(I)X
1609(prefer)X
1829(this)X
1971(second)X
2221(method,)X
2507(it)X
2577(is)X
2656(the)X
2780(one)X
2922(used)X
3095(by)X
3201(default.)X
3470(The)X
3621(\256rst)X
3771(method)X
755 3816(will)N
899(be)X
995(used)X
1162(if)X
1231(you)X
1371(give)X
1529(the)X
3 f
9 f
1647(-)X
3 f
1691(P)X
1 f
1760(\257ag)X
1900(to)X
1982(PMake.)X
3 f
9 f
555 3940(-)N
3 f
599(V)X
1 f
755(As)X
868(mentioned)X
1230(before,)X
1480(the)X
3 f
9 f
1602(-)X
3 f
1646(V)X
1 f
1728(\257ag)X
1872(tells)X
2029(PMake)X
2280(to)X
2366(use)X
2496(Make's)X
2760(style)X
2934(of)X
3024(expanding)X
3381(variables,)X
3714(substitut-)X
755 4036(ing)N
877(the)X
995(empty)X
1215(string)X
1417(for)X
1531(any)X
1667(variable)X
1946(it)X
2010(doesn't)X
2266(know.)X
3 f
9 f
555 4160(-)N
3 f
599(W)X
1 f
755(There)X
971(are)X
1098(several)X
1354(times)X
1555(when)X
1757(PMake)X
2011(will)X
2162(print)X
2340(a)X
2403(message)X
2702(at)X
2787(you)X
2934(that)X
3081(is)X
3161(only)X
3330(a)X
3393(warning,)X
3703(i.e.)X
3828(it)X
3899(can)X
755 4256(continue)N
1056(to)X
1143(work)X
1333(in)X
1420(spite)X
1596(of)X
1688(your)X
1860(having)X
2103(done)X
2284(something)X
2642(silly)X
2804(\(such)X
3003(as)X
3095(forgotten)X
3414(a)X
3475(leading)X
3735(tab)X
3857(for)X
3975(a)X
755 4352(shell)N
928(command\).)X
1313(Sometimes)X
1690(you)X
1832(are)X
1953(well)X
2113(aware)X
2328(of)X
2417(silly)X
2576(things)X
2793(you)X
2935(have)X
3108(done)X
3285(and)X
3422(would)X
3643(like)X
3784(PMake)X
755 4448(to)N
837(stop)X
990(bothering)X
1317(you.)X
1477(This)X
1639(\257ag)X
1779(tells)X
1932(it)X
1996(to)X
2078(shut)X
2231(up)X
2331(about)X
2529(anything)X
2829(non-fatal.)X
3 f
9 f
555 4572(-)N
3 f
599(X)X
1 f
755(This)X
917(\257ag)X
1057(causes)X
1287(PMake)X
1534(to)X
1616(not)X
1738(attempt)X
1998(to)X
2080(export)X
2305(any)X
2441(jobs)X
2594(to)X
2676(another)X
2937(machine.)X
755 4696(Several)N
1020(\257ags)X
1195(may)X
1357(follow)X
1590(a)X
1650(single)X
1865(`)X
9 f
1892(-)X
1 f
1936('.)X
2007(Those)X
2227(\257ags)X
2402(that)X
2545(require)X
2796(arguments)X
3153(take)X
3310(them)X
3493(from)X
3672(successive)X
555 4792(parameters.)N
948(E.g.)X
7 f
843 4936(pmake)N
1131(-fDnI)X
1419(server.mk)X
1899(DEBUG)X
2187(/chip2/X/server/include)X
1 f
555 5080(will)N
704(cause)X
908(PMake)X
1160(to)X
1247(read)X
7 f
1411(server.mk)X
1 f
1868(as)X
1960(the)X
2083(input)X
2272(make\256le,)X
2593(de\256ne)X
2814(the)X
2937(variable)X
7 f
3220(DEBUG)X
1 f
3484(as)X
3575(a)X
3635(global)X
3859(vari-)X
555 5176(able)N
709(and)X
845(look)X
1007(for)X
1121(included)X
1417(make\256les)X
1744(in)X
1826(the)X
1944(directory)X
7 f
2254(/chip2/X/server/include)X
1 f
(.)S
3 f
555 5368(2.8.)N
715(Summary)X
1 f
755 5492(A)N
833(make\256le)X
1129(is)X
1202(made)X
1396(of)X
1483(four)X
1637(types)X
1826(of)X
1913(lines:)X
10 f
755 5616(g)N
1 f
835(Dependency)X
1257(lines)X
10 f
755 5740(g)N
1 f
835(Creation)X
1131(commands)X

13 p
%%Page: 13 14
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(13)X
2343(-)X
10 f
755 672(g)N
1 f
835(Variable)X
1132(assignments)X
10 f
755 796(g)N
1 f
835(Comments,)X
1221(include)X
1477(statements)X
1835(and)X
1971(conditional)X
2351(directives)X
755 920(A)N
834(dependency)X
1239(line)X
1380(is)X
1454(a)X
1511(list)X
1629(of)X
1717(one)X
1854(or)X
1942(more)X
2128(targets,)X
2383(an)X
2480(operator)X
2769(\(`)X
7 f
2823(:)X
1 f
(',)S
2939(`)X
7 f
2966(::)X
1 f
(',)S
3130(or)X
3218(`)X
7 f
3245(!)X
1 f
('\),)S
3388(and)X
3525(a)X
3581(list)X
3698(of)X
3785(zero)X
3944(or)X
555 1016(more)N
740(sources.)X
1021(Sources)X
1295(may)X
1453(contain)X
1709(wildcards)X
2041(and)X
2177(certain)X
2416(local)X
2592(variables.)X
755 1140(A)N
836(creation)X
1118(command)X
1457(is)X
1533(a)X
1592(regular)X
1843(shell)X
2017(command)X
2356(preceded)X
2670(by)X
2773(a)X
2832(tab.)X
2973(In)X
3062(addition,)X
3366(if)X
3437(the)X
3557(\256rst)X
3703(two)X
3845(char-)X
555 1236(acters)N
765(after)X
935(the)X
1055(tab)X
1175(\(and)X
1340(other)X
1527(whitespace\))X
1933(are)X
2054(a)X
2112(combination)X
2534(of)X
2623(`)X
7 f
2650(@)X
1 f
(')S
2747(or)X
2836(`)X
7 f
2863(-)X
1 f
(',)S
2980(PMake)X
3229(will)X
3375(cause)X
3576(the)X
3695(command)X
555 1332(to)N
641(not)X
767(be)X
867(printed)X
1118(\(if)X
1218(the)X
1340(character)X
1660(is)X
1737(`)X
7 f
1764(@)X
1 f
('\))S
1890(or)X
1981(errors)X
2193(from)X
2372(it)X
2439(to)X
2524(be)X
2623(ignored)X
2891(\(if)X
2990(`)X
7 f
3017(-)X
1 f
('\).)S
3182(A)X
3263(blank)X
3464(line,)X
3627(dependency)X
555 1428(line)N
697(or)X
786(variable)X
1067(assignment)X
1449(terminates)X
1805(a)X
1863(creation)X
2144(script.)X
2364(There)X
2574(may)X
2734(be)X
2832(only)X
2996(one)X
3134(creation)X
3415(script)X
3615(for)X
3730(each)X
3899(tar-)X
555 1524(get)N
673(with)X
835(a)X
891(`)X
7 f
918(:)X
1 f
(')S
1013(or)X
1100(`)X
7 f
1127(!)X
1 f
(')S
1222(operator.)X
755 1648(Variables)N
1091(are)X
1218(places)X
1447(to)X
1537(store)X
1721(text.)X
1888(They)X
2080(may)X
2245(be)X
2348(unconditionally)X
2877(assigned-to)X
3269(using)X
3469(the)X
3594(`)X
7 f
3621(=)X
1 f
(')S
3723(operator,)X
555 1744(appended-to)N
977(using)X
1175(the)X
1298(`)X
7 f
1325(+=)X
1 f
(')S
1473(operator,)X
1786(conditionally)X
2233(\(if)X
2334(the)X
2457(variable)X
2740(is)X
2817(unde\256ned\))X
3184(assigned-to)X
3573(with)X
3739(the)X
3861(`)X
7 f
3888(?=)X
1 f
(')S
555 1840(operator,)N
869(and)X
1011(assigned-to)X
1402(with)X
1570(variable)X
1855(expansion)X
2206(with)X
2374(the)X
2498(`)X
7 f
2525(:=)X
1 f
(')S
2674(operator.)X
2987(The)X
3137(output)X
3366(of)X
3458(a)X
3519(shell)X
3695(command)X
555 1936(may)N
714(be)X
810(assigned)X
1106(to)X
1188(a)X
1244(variable)X
1523(using)X
1716(the)X
1834(`)X
7 f
1861(!=)X
1 f
(')S
2004(operator.)X
2332(Variables)X
2660(may)X
2818(be)X
2914(expanded)X
3242(\(their)X
3436(value)X
3630(inserted\))X
3931(by)X
555 2032(enclosing)N
896(their)X
1077(name)X
1285(in)X
1381(parentheses)X
1790(or)X
1891(curly)X
2090(braces,)X
2350(prceeded)X
2675(by)X
2789(a)X
2859(dollar)X
3080(sign.)X
3287(A)X
3378(dollar)X
3598(sign)X
3764(may)X
3935(be)X
555 2128(escaped)N
831(with)X
994(another)X
1256(dollar)X
1464(sign.)X
1638(Variables)X
1967(are)X
2086(not)X
2208(expanded)X
2536(if)X
2605(PMake)X
2852(doesn't)X
3108(know)X
3306(about)X
3504(them.)X
3704(There)X
3912(are)X
555 2224(seven)N
795(local)X
1007(variables:)X
7 f
1375(.TARGET)X
1 f
(,)S
7 f
1787(.ALLSRC)X
1 f
(,)S
7 f
2199(.OODATE)X
1 f
(,)S
7 f
2611(.PREFIX)X
1 f
(,)S
7 f
3023(.IMPSRC)X
1 f
(,)S
7 f
3435(.ARCHIVE)X
1 f
(,)S
3895(and)X
7 f
555 2320(.MEMBER)N
1 f
(.)S
964(Four)X
1148(of)X
1248(them)X
1441(\()X
7 f
1468(.TARGET)X
1 f
(,)S
7 f
1857(.PREFIX)X
1 f
(,)S
7 f
2246(.ARCHIVE)X
1 f
(,)S
2683(and)X
7 f
2832(.MEMBER)X
1 f
(\))S
3228(may)X
3398(be)X
3506(used)X
3685(to)X
3779(specify)X
555 2416(``dynamic)N
905(sources.'')X
1240(Variables)X
1568(are)X
1687(good.)X
1887(Know)X
2103(them.)X
2303(Love)X
2488(them.)X
2688(Live)X
2855(them.)X
755 2540(Debugging)N
1131(of)X
1218(make\256les)X
1545(is)X
1618(best)X
1767(accomplished)X
2228(using)X
2421(the)X
3 f
9 f
2539(-)X
3 f
2583(n)X
1 f
2627(,)X
3 f
9 f
2667(-)X
3 f
2711(d)X
2775(m)X
1 f
2842(,)X
2882(and)X
3 f
9 f
3018(-)X
3 f
3062(p)X
3126(2)X
1 f
3186(\257ags.)X
3 f
555 2732(2.9.)N
715(Exercises)X
14 s
2168 2828(TBA)N
10 s
555 3020(3.)N
655(Short-cuts)X
1031(and)X
1179(Other)X
1404(Nice)X
1576(Things)X
1 f
755 3144(Based)N
974(on)X
1077(what)X
1256(I've)X
1409(told)X
1556(you)X
1699(so)X
1793(far,)X
1926(you)X
2069(may)X
2230(have)X
2405(gotten)X
2628(the)X
2749(impression)X
3123(that)X
3266(PMake)X
3516(is)X
3592(just)X
3730(a)X
3788(way)X
3944(of)X
555 3240(storing)N
800(away)X
993(commands)X
1363(and)X
1501(making)X
1763(sure)X
1919(you)X
2061(don't)X
2252(forget)X
2466(to)X
2550(compile)X
2830(something.)X
3205(Good.)X
3425(That's)X
3652(just)X
3789(what)X
3967(it)X
555 3336(is.)N
670(However,)X
1007(the)X
1127(ways)X
1314(I've)X
1465(described)X
1794(have)X
1967(been)X
2140(inelegant,)X
2475(at)X
2554(best,)X
2724(and)X
2861(painful,)X
3129(at)X
3208(worst.)X
3447(This)X
3610(chapter)X
3868(con-)X
555 3432(tains)N
735(things)X
959(that)X
1108(make)X
1311(the)X
1438(writing)X
1698(of)X
1794(make\256les)X
2130(easier)X
2347(and)X
2492(the)X
2619(make\256les)X
2954(themselves)X
3338(shorter)X
3589(and)X
3733(easier)X
3949(to)X
555 3528(modify)N
810(\(and,)X
997(occasionally,)X
1442(simpler\).)X
1753(In)X
1844(this)X
1983(chapter,)X
2264(I)X
2315(assume)X
2575(you)X
2719(are)X
2842(somewhat)X
3190(more)X
3378(familiar)X
3655(with)X
3820(Sprite)X
555 3624(\(or)N
680(UNIX,)X
932(if)X
1011(that's)X
1219(what)X
1405(you're)X
1645(using\))X
1875(than)X
2043(I)X
2100(did)X
2232(in)X
2324(chapter)X
2591(2,)X
2681(just)X
2826(so)X
2927(you're)X
3167(on)X
3277(your)X
3454(toes.)X
3653(So)X
3767(without)X
555 3720(further)N
794(ado...)X
3 f
555 3912(3.1.)N
715(Transformation)X
1282(Rules)X
1 f
755 4036(As)N
867(you)X
1010(know,)X
1231(a)X
1290(\256le's)X
1473(name)X
1670(consists)X
1946(of)X
2036(two)X
2179(parts:)X
2380(a)X
2439(base)X
2605(name,)X
2822(which)X
3041(gives)X
3233(some)X
3425(hint)X
3572(as)X
3662(to)X
3747(the)X
3868(con-)X
555 4132(tents)N
731(of)X
823(the)X
946(\256le,)X
1093(and)X
1234(a)X
1295(suf\256x,)X
1522(which)X
1743(usually)X
1999(indicates)X
2309(the)X
2432(format)X
2671(of)X
2762(the)X
2884(\256le.)X
3050(Over)X
3235(the)X
3357(years,)X
3571(as)X
9 s
3660(UNIX)X
10 s
(\262)S
3904(has)X
555 4228(developed,)N
932(naming)X
1199(conventions,)X
1633(with)X
1801(regard)X
2033(to)X
2121(suf\256xes,)X
2416(have)X
2594(also)X
2749(developed)X
3105(that)X
3251(have)X
3429(become)X
3705(almost)X
3944(as)X
555 4324(incontrovertible)N
1089(as)X
1178(Law.)X
1363(E.g.)X
1514(a)X
1572(\256le)X
1696(ending)X
1936(in)X
7 f
2020(.c)X
1 f
2138(is)X
2213(assumed)X
2511(to)X
2595(contain)X
2853(C)X
2928(source)X
3160(code;)X
3355(one)X
3492(with)X
3655(a)X
7 f
3712(.o)X
1 f
3829(suf\256x)X
555 4420(is)N
631(assumed)X
930(to)X
1015(be)X
1114(a)X
1173(compiled,)X
1514(relocatable)X
1890(object)X
2109(\256le)X
2234(that)X
2377(may)X
2538(be)X
2637(linked)X
2860(into)X
3007(any)X
3146(program;)X
3463(a)X
3521(\256le)X
3645(with)X
3809(a)X
7 f
3867(.ms)X
1 f
555 4516(suf\256x)N
764(is)X
844(usually)X
1102(a)X
1165(text)X
1312(\256le)X
1441(to)X
1530(be)X
1633(processed)X
1977(by)X
2084(Troff)X
2281(with)X
2450(the)X
9 f
2575(-)X
1 f
2619(ms)X
2738(macro)X
2965(package,)X
3275(and)X
3417(so)X
3514(on.)X
3660(One)X
3820(of)X
3913(the)X
555 4612(best)N
705(aspects)X
958(of)X
1046(both)X
1209(Make)X
1413(and)X
1550(PMake)X
1798(comes)X
2024(from)X
2201(their)X
2369(understanding)X
2844(of)X
2932(how)X
3090(the)X
3208(suf\256x)X
3410(of)X
3497(a)X
3553(\256le)X
3675(pertains)X
3949(to)X
555 4708(its)N
651(contents)X
939(and)X
1076(their)X
1244(ability)X
1469(to)X
1552(do)X
1653(things)X
1869(with)X
2032(a)X
2089(\256le)X
2212(based)X
2416(soley)X
2606(on)X
2707(its)X
2803(suf\256x.)X
3026(This)X
3189(ability)X
3414(comes)X
3639(from)X
3815(some-)X
555 4804(thing)N
744(known)X
987(as)X
1079(a)X
1140(transformation)X
1637(rule.)X
1807(A)X
1890(transformation)X
2387(rule)X
2537(speci\256es)X
2837(how)X
2999(to)X
3085(change)X
3337(a)X
3397(\256le)X
3523(with)X
3689(one)X
3829(suf\256x)X
555 4900(into)N
699(a)X
755(\256le)X
877(with)X
1039(another)X
1300(suf\256x.)X
755 5024(A)N
841(transformation)X
1341(rule)X
1494(looks)X
1695(much)X
1901(like)X
2049(a)X
2113(dependency)X
2525(line,)X
2693(except)X
2931(the)X
3057(target)X
3268(is)X
3349(made)X
3551(of)X
3646(two)X
3793(known)X
555 5120(suf\256xes)N
831(stuck)X
1027(together.)X
1337(Suf\256xes)X
1626(are)X
1752(made)X
1953(known)X
2198(to)X
2287(PMake)X
2541(by)X
2648(placing)X
2911(them)X
3098(as)X
3192(sources)X
3459(on)X
3565(a)X
3627(dependency)X
555 5216(line)N
695(whose)X
920(target)X
1123(is)X
1196(the)X
1314(special)X
1557(target)X
7 f
1760(.SUFFIXES)X
1 f
(.)S
2252(E.g.)X
8 s
10 f
555 5584(hhhhhhhhhhhhhhhhhh)N
1 f
555 5664(\262)N
7 s
601(UNIX)X
8 s
756(is)X
815(a)X
859(trademark)X
1133(of)X
1202(Bell)X
1325(Laboratories.)X

14 p
%%Page: 14 15
8 s 8 xH 0 xS 1 f
10 s
7 f
1 f
2196 384(-)N
2243(14)X
2343(-)X
7 f
843 720(.SUFFIXES)N
1611(:)X
1707(.o)X
1851(.c)X
843 816(.c.o)N
1611(:)X
1227 912($\(CC\))N
1515($\(CFLAGS\))X
1995(-c)X
2139($\(.IMPSRC\))X
1 f
555 1056(The)N
707(creation)X
993(script)X
1198(attached)X
1493(to)X
1582(the)X
1707(target)X
1917(is)X
1997(used)X
2171(to)X
2260(transform)X
2599(a)X
2662(\256le)X
2791(with)X
2960(the)X
3085(\256rst)X
3236(suf\256x)X
3445(\(in)X
3561(this)X
3703(case,)X
7 f
3888(.c)X
1 f
(\))S
555 1152(into)N
705(a)X
767(\256le)X
895(with)X
1063(the)X
1187(second)X
1436(suf\256x)X
1644(\(here,)X
7 f
1856(.o)X
1 f
(\).)S
2044(In)X
2136(addition,)X
2443(the)X
2566(target)X
2774(inherits)X
3039(whatever)X
3359(attributes)X
3682(have)X
3859(been)X
555 1248(applied)N
816(to)X
902(the)X
1024(transformation)X
1520(rule.)X
1709(The)X
1858(simple)X
2095(rule)X
2244(given)X
2446(above)X
2662(says)X
2824(that)X
2968(to)X
3054(transform)X
3390(a)X
3450(C)X
3527(source)X
3761(\256le)X
3887(into)X
555 1344(an)N
667(object)X
899(\256le,)X
1057(you)X
1213(compile)X
1506(it)X
1585(using)X
7 f
1793(cc)X
1 f
1924(with)X
2101(the)X
7 f
9 f
2234(-)X
7 f
2278(c)X
1 f
2361(\257ag.)X
2556(This)X
2733(rule)X
2893(is)X
2981(taken)X
3190(straight)X
3465(from)X
3656(the)X
3789(system)X
555 1440(make\256le.)N
877(Many)X
1090(transformation)X
1588(rules)X
1769(\(and)X
1937(suf\256xes\))X
2238(are)X
2362(de\256ned)X
2623(there,)X
2829(and)X
2970(I)X
3022(refer)X
3200(you)X
3345(to)X
3432(it)X
3501(for)X
3620(more)X
3810(exam-)X
555 1536(ples)N
704(\(type)X
889(``)X
7 f
943(pmake)X
1231(-h)X
1 f
('')S
1401(to)X
1483(\256nd)X
1627(out)X
1749(where)X
1966(it)X
2030(is\).)X
755 1660(There)N
963(are)X
1082(several)X
1330(things)X
1545(to)X
1627(note)X
1785(about)X
1983(the)X
2101(transformation)X
2593(rule)X
2738(given)X
2936(above:)X
755 1784(1\))N
955(The)X
7 f
1104(.IMPSRC)X
1 f
1464(variable.)X
1787(This)X
1953(variable)X
2236(is)X
2313(set)X
2426(to)X
2512(the)X
2634(``implied)X
2955(source'')X
3242(\(the)X
3390(\256le)X
3515(from)X
3694(which)X
3913(the)X
955 1880(target)N
1158(is)X
1231(being)X
1429(created;)X
1704(the)X
1822(one)X
1958(with)X
2120(the)X
2238(\256rst)X
2382(suf\256x\),)X
2631(which,)X
2867(in)X
2949(this)X
3084(case,)X
3263(is)X
3336(the)X
3454(.c)X
3530(\256le.)X
755 2004(2\))N
955(The)X
7 f
1102(CFLAGS)X
1 f
1412(variable.)X
1713(Almost)X
1970(all)X
2072(of)X
2161(the)X
2281(transformation)X
2775(rules)X
2953(in)X
3037(the)X
3157(system)X
3401(make\256le)X
3699(are)X
3820(set)X
3931(up)X
955 2100(using)N
1151(variables)X
1464(that)X
1607(you)X
1750(can)X
1885(alter)X
2051(in)X
2136(your)X
2306(make\256le)X
2605(to)X
2690(tailor)X
2881(the)X
3001(rule)X
3148(to)X
3232(your)X
3401(needs.)X
3626(In)X
3715(this)X
3852(case,)X
955 2196(if)N
1031(you)X
1178(want)X
1361(all)X
1468(your)X
1642(C)X
1722(\256les)X
1882(to)X
1970(be)X
2072(compiled)X
2396(with)X
2564(the)X
3 f
9 f
2688(-)X
3 f
2732(g)X
1 f
2798(\257ag,)X
2964(to)X
3052(provide)X
3323(information)X
3727(for)X
7 f
3847(dbx)X
1 f
(,)S
955 2292(you)N
1105(would)X
1335(set)X
1454(the)X
7 f
1582(CFLAGS)X
1 f
1900(variable)X
2189(to)X
2281(contain)X
7 f
2547(-g)X
1 f
2673(\(``)X
7 f
2754(CFLAGS)X
3099(=)X
3204(-g)X
1 f
(''\))S
3410(and)X
3555(PMake)X
3811(would)X
955 2388(take)N
1109(care)X
1264(of)X
1351(the)X
1469(rest.)X
755 2512(To)N
864(give)X
1022(you)X
1162(a)X
1218(quick)X
1416(example,)X
1728(the)X
1846(make\256le)X
2142(in)X
2224(2.3.4)X
2404(could)X
2602(be)X
2698(changed)X
2986(to)X
3068(this:)X
7 f
843 2656(OBJS)N
1611(=)X
1707(a.o)X
1899(b.o)X
2091(c.o)X
843 2752(program)N
1611(:)X
1707($\(OBJS\))X
1227 2848($\(CC\))N
1515(-o)X
1659($\(.TARGET\))X
2187($\(.ALLSRC\))X
843 2944($\(OBJS\))N
1611(:)X
1707(defs.h)X
1 f
555 3088(The)N
700(transformation)X
1192(rule)X
1337(I)X
1384(gave)X
1556(above)X
1768(takes)X
1953(the)X
2071(place)X
2261(of)X
2348(the)X
2466(6)X
2526(lines)X
8 s
2677 3063(1)N
7 f
10 s
843 3232(a.o)N
1611(:)X
1707(a.c)X
1227 3328(cc)N
1371(-c)X
1515(a.c)X
843 3424(b.o)N
1611(:)X
1707(b.c)X
1227 3520(cc)N
1371(-c)X
1515(b.c)X
843 3616(c.o)N
1611(:)X
1707(c.c)X
1227 3712(cc)N
1371(-c)X
1515(c.c)X
1 f
755 3884(Now)N
941(you)X
1091(may)X
1259(be)X
1365(wondering)X
1738(about)X
1946(the)X
2074(dependency)X
2488(between)X
2786(the)X
7 f
2914(.o)X
1 f
3040(and)X
7 f
3186(.c)X
1 f
3312(\256les)X
3475(\320)X
3584(it's)X
3715(not)X
3846(men-)X
555 3980(tioned)N
788(anywhere)X
1134(in)X
1229(the)X
1360(new)X
1527(make\256le.)X
1856(This)X
2031(is)X
2117(because)X
2405(it)X
2482(isn't)X
2657(needed:)X
2940(one)X
3088(of)X
3187(the)X
3317(effects)X
3564(of)X
3663(applying)X
3975(a)X
555 4076(transformation)N
1051(rule)X
1200(is)X
1277(the)X
1399(target)X
1605(comes)X
1833(to)X
1918(depend)X
2173(on)X
2276(the)X
2397(implied)X
2664(source.)X
2917(That's)X
3145(why)X
3306(it's)X
3431(called)X
3646(the)X
3767(implied)X
2 f
555 4172(source)N
1 f
782(.)X
755 4296(For)N
886(a)X
942(more)X
1127(detailed)X
1401(example.)X
1713(Say)X
1853(you)X
1993(have)X
2165(a)X
2221(make\256le)X
2517(like)X
2657(this:)X
7 f
843 4440(a.out)N
1611(:)X
1707(a.o)X
1899(b.o)X
1227 4536($\(CC\))N
1515($\(.ALLSRC\))X
1 f
555 4680(and)N
691(a)X
747(directory)X
1057(set)X
1166(up)X
1266(like)X
1406(this:)X
7 f
843 4824(total)N
1131(4)X
843 4920(-rw-rw-r--)N
1419(1)X
1515(deboor)X
2235(34)X
2379(Sep)X
2619(7)X
2715(00:43)X
3003(Makefile)X
843 5016(-rw-rw-r--)N
1419(1)X
1515(deboor)X
2187(119)X
2379(Oct)X
2619(3)X
2715(19:39)X
3003(a.c)X
843 5112(-rw-rw-r--)N
1419(1)X
1515(deboor)X
2187(201)X
2379(Sep)X
2619(7)X
2715(00:43)X
3003(a.o)X
843 5208(-rw-rw-r--)N
1419(1)X
1515(deboor)X
2235(69)X
2379(Sep)X
2619(7)X
2715(00:43)X
3003(b.c)X
1 f
555 5352(While)N
773(just)X
910(typing)X
1136(``)X
7 f
1190(pmake)X
1 f
('')S
1506(will)X
1652(do)X
1754(the)X
1874(right)X
2047(thing,)X
2253(it's)X
2377(much)X
2577(more)X
2764(informative)X
3159(to)X
3242(type)X
3401(``)X
7 f
3455(pmake)X
3744(-d)X
3889(s)X
1 f
(''.)S
555 5448(This)N
717(will)X
861(show)X
1050(you)X
1190(what)X
1366(PMake)X
1613(is)X
1686(up)X
1786(to)X
1868(as)X
1955(it)X
2019(processes)X
2347(the)X
2465(\256les.)X
2638(In)X
2725(this)X
2860(case,)X
3039(PMake)X
3286(prints)X
3488(the)X
3606(following:)X
8 s
10 f
555 5570(hhhhhhhhhhhhhhhhhh)N
1 f
555 5664(X)N
617(REF)X
751(2003)X
895(4)X
943(2.6)X
6 s
635 5725(1)N
8 s
691 5744(This)N
821(is)X
880(also)X
999(somewhat)X
1274(cleaner,)X
1489(I)X
1526(think,)X
1690(than)X
1816(the)X
1910(dynamic)X
2146(source)X
2328(solution)X
2551(presented)X
2811(in)X
2877(2.6)X

15 p
%%Page: 15 16
8 s 8 xH 0 xS 1 f
10 s
7 f
1 f
2196 384(-)N
2243(15)X
2343(-)X
7 f
843 720(Suff_FindDeps)N
1515(\(a.out\))X
1043 816(using)N
1331(existing)X
1763(source)X
2099(a.o)X
1043 912(applying)N
1475(.o)X
1619(->)X
1763(.out)X
2003(to)X
2147("a.o")X
843 1008(Suff_FindDeps)N
1515(\(a.o\))X
1043 1104(trying)N
1379(a.c...got)X
1859(it)X
1043 1200(applying)N
1475(.c)X
1619(->)X
1763(.o)X
1907(to)X
2051("a.c")X
843 1296(Suff_FindDeps)N
1515(\(b.o\))X
1043 1392(trying)N
1379(b.c...got)X
1859(it)X
1043 1488(applying)N
1475(.c)X
1619(->)X
1763(.o)X
1907(to)X
2051("b.c")X
843 1584(Suff_FindDeps)N
1515(\(a.c\))X
1043 1680(trying)N
1379(a.y...not)X
1859(there)X
1043 1776(trying)N
1379(a.l...not)X
1859(there)X
1043 1872(trying)N
1379(a.c,v...not)X
1955(there)X
1043 1968(trying)N
1379(a.y,v...not)X
1955(there)X
1043 2064(trying)N
1379(a.l,v...not)X
1955(there)X
843 2160(Suff_FindDeps)N
1515(\(b.c\))X
1043 2256(trying)N
1379(b.y...not)X
1859(there)X
1043 2352(trying)N
1379(b.l...not)X
1859(there)X
1043 2448(trying)N
1379(b.c,v...not)X
1955(there)X
1043 2544(trying)N
1379(b.y,v...not)X
1955(there)X
1043 2640(trying)N
1379(b.l,v...not)X
1955(there)X
843 2736(---)N
1035(a.o)X
1227(---)X
843 2832(cc)N
1035(-c)X
1179(a.c)X
843 2928(---)N
1035(b.o)X
1227(---)X
843 3024(cc)N
1035(-c)X
1179(b.c)X
843 3120(---)N
1035(a.out)X
1323(---)X
843 3216(cc)N
987(a.o)X
1179(b.o)X
755 3388(Suff_FindDeps)N
1 f
1401(is)X
1476(the)X
1596(name)X
1791(of)X
1879(a)X
1936(function)X
2224(in)X
2307(PMake)X
2555(that)X
2696(is)X
2770(called)X
2983(to)X
3066(check)X
3275(for)X
3390(implied)X
3655(sources)X
3917(for)X
555 3484(a)N
618(target)X
828(using)X
1028(transformation)X
1526(rules.)X
1748(The)X
1899(transformations)X
2428(it)X
2498(tries)X
2662(are,)X
2807(naturally)X
3118(enough,)X
3400(limited)X
3652(to)X
3740(the)X
3864(ones)X
555 3580(that)N
706(have)X
889(been)X
1072(de\256ned)X
1339(\(a)X
1433(transformation)X
1936(may)X
2105(be)X
2212(de\256ned)X
2479(multiple)X
2776(times,)X
3000(by)X
3111(the)X
3240(way,)X
3424(but)X
3556(only)X
3728(the)X
3856(most)X
555 3676(recent)N
782(one)X
928(will)X
1082(be)X
1188(used\).)X
1412(You)X
1580(will)X
1734(notice,)X
1980(however,)X
2307(that)X
2457(there)X
2648(is)X
2731(a)X
2797(de\256nite)X
3067(order)X
3267(to)X
3358(the)X
3485(suf\256xes)X
3763(that)X
3912(are)X
555 3772(tried.)N
752(This)X
924(order)X
1124(is)X
1207(set)X
1326(by)X
1436(the)X
1564(relative)X
1835(positions)X
2153(of)X
2250(the)X
2378(suf\256xes)X
2657(on)X
2767(the)X
7 f
2894(.SUFFIXES)X
1 f
3355(line)X
3504(\320)X
3613(the)X
3740(earlier)X
3975(a)X
555 3868(suf\256x)N
764(appears,)X
1057(the)X
1181(earlier)X
1413(it)X
1483(is)X
1562(checked)X
1852(as)X
1945(the)X
2069(source)X
2305(of)X
2398(a)X
2460(transformation.)X
2978(Once)X
3174(a)X
3236(suf\256x)X
3444(has)X
3577(been)X
3755(de\256ned,)X
555 3964(the)N
678(only)X
845(way)X
1004(to)X
1091(change)X
1344(its)X
1444(position)X
1726(in)X
1813(the)X
1936(pecking)X
2215(order)X
2410(is)X
2488(to)X
2574(remove)X
2839(all)X
2943(the)X
3065(suf\256xes)X
3338(\(by)X
3469(having)X
3711(a)X
7 f
3771(.SUF-)X
555 4060(FIXES)N
1 f
826(dependency)X
1241(line)X
1392(with)X
1565(no)X
1676(sources\))X
1975(and)X
2122(rede\256ne)X
2412(them)X
2603(in)X
2695(the)X
2823(order)X
3023(you)X
3173(want.)X
3379(\(Previously-de\256ned)X
555 4156(transformation)N
1047(rules)X
1223(will)X
1367(be)X
1463(automatically)X
1919(rede\256ned)X
2238(as)X
2325(the)X
2443(suf\256xes)X
2712(they)X
2870(involve)X
3130(are)X
3249 0.2955(re-entered.\))AX
755 4280(Another)N
1047(way)X
1210(to)X
1301(affect)X
1514(the)X
1641(search)X
1876(order)X
2075(is)X
2157(to)X
2248(make)X
2451(the)X
2578(dependency)X
2990(explicit.)X
3278(In)X
3373(the)X
3499(above)X
3719(example,)X
7 f
555 4376(a.out)N
1 f
823(depends)X
1114(on)X
7 f
1222(a.o)X
1 f
1394(and)X
7 f
1537(b.o)X
1 f
(.)S
1748(Since)X
1953(a)X
2016(transformation)X
2515(exists)X
2724(from)X
7 f
2907(.o)X
1 f
3030(to)X
7 f
3119(.out)X
1 f
(,)S
3358(PMake)X
3612(uses)X
3777(that,)X
3944(as)X
555 4472(indicated)N
869(by)X
969(the)X
1087(``)X
7 f
1141(using)X
1429(existing)X
1861(source)X
2197(a.o)X
1 f
('')S
2415(message.)X
755 4596(The)N
913(search)X
1152(for)X
1279(a)X
1348(transformation)X
1853(starts)X
2055(from)X
2244(the)X
2375(suf\256x)X
2589(of)X
2688(the)X
2818(target)X
3033(and)X
3181(continues)X
3520(through)X
3801(all)X
3913(the)X
555 4692(de\256ned)N
813(transformations,)X
1358(in)X
1442(the)X
1562(order)X
1754(dictated)X
2030(by)X
2131(the)X
2250(suf\256x)X
2453(ranking,)X
2739(until)X
2906(an)X
3003(existing)X
3277(\256le)X
3400(with)X
3563(the)X
3682(same)X
3868(base)X
555 4788(\(the)N
702(target)X
907(name)X
1103(minus)X
1320(the)X
1440(suf\256x)X
1644(and)X
1782(any)X
1920(leading)X
2178(directories\))X
2566(is)X
2641(found.)X
2870(At)X
2972(that)X
3114(point,)X
3320(one)X
3458(or)X
3547(more)X
3734(transfor-)X
555 4884(mation)N
797(rules)X
973(will)X
1117(have)X
1289(been)X
1461(found)X
1668(to)X
1750(change)X
1998(the)X
2116(one)X
2252(existing)X
2525(\256le)X
2647(into)X
2791(the)X
2909(target.)X
755 5008(For)N
886(example,)X
1198(ignoring)X
1489(what's)X
1723(in)X
1805(the)X
1923(system)X
2165(make\256le)X
2461(for)X
2575(now,)X
2753(say)X
2880(you)X
3020(have)X
3192(a)X
3248(make\256le)X
3544(like)X
3684(this:)X

16 p
%%Page: 16 17
10 s 10 xH 0 xS 1 f
7 f
1 f
2196 384(-)N
2243(16)X
2343(-)X
7 f
843 720(.SUFFIXES)N
1611(:)X
1707(.out)X
1947(.o)X
2091(.c)X
2235(.y)X
2379(.l)X
843 816(.l.c)N
1611(:)X
1227 912(lex)N
1419($\(.IMPSRC\))X
1227 1008(mv)N
1371(lex.yy.c)X
1803($\(.TARGET\))X
843 1104(.y.c)N
1611(:)X
1227 1200(yacc)N
1467($\(.IMPSRC\))X
1227 1296(mv)N
1371(y.tab.c)X
1755($\(.TARGET\))X
843 1392(.c.o)N
1611(:)X
1227 1488(cc)N
1371(-c)X
1515($\(.IMPSRC\))X
843 1584(.o.out)N
1611(:)X
1227 1680(cc)N
1371(-o)X
1515($\(.TARGET\))X
2043($\(.IMPSRC\))X
1 f
555 1824(and)N
693(the)X
813(single)X
1026(\256le)X
7 f
1150(jive.l)X
1 f
(.)S
1499(If)X
1574(you)X
1715(were)X
1893(to)X
1976(type)X
2135(``)X
7 f
2189(pmake)X
2478(-rd)X
2671(ms)X
2816(jive.out)X
1 f
(,'')S
3295(you)X
3436(would)X
3657(get)X
3776(the)X
3895(fol-)X
555 1920(lowing)N
797(output)X
1021(for)X
7 f
1135(jive.out)X
1 f
(:)S
7 f
843 2064(Suff_FindDeps)N
1515(\(jive.out\))X
1043 2160(trying)N
1379(jive.o...not)X
2003(there)X
1043 2256(trying)N
1379(jive.c...not)X
2003(there)X
1043 2352(trying)N
1379(jive.y...not)X
2003(there)X
1043 2448(trying)N
1379(jive.l...got)X
2003(it)X
1043 2544(applying)N
1475(.l)X
1619(->)X
1763(.c)X
1907(to)X
2051("jive.l")X
1043 2640(applying)N
1475(.c)X
1619(->)X
1763(.o)X
1907(to)X
2051("jive.c")X
1043 2736(applying)N
1475(.o)X
1619(->)X
1763(.out)X
2003(to)X
2147("jive.o")X
1 f
555 2880(and)N
693(this)X
830(is)X
905(why:)X
1087(PMake)X
1336(starts)X
1527(with)X
1691(the)X
1811(target)X
7 f
2016(jive.out)X
1 f
(,)S
2442(\256gures)X
2681(out)X
2804(its)X
2900(suf\256x)X
3103(\()X
7 f
3130(.out)X
1 f
(\))S
3370(and)X
3507(looks)X
3701(for)X
3816(things)X
555 2976(it)N
620(can)X
753(transform)X
1086(to)X
1169(a)X
7 f
1226(.out)X
1 f
1439(\256le.)X
1582(In)X
1670(this)X
1806(case,)X
1986(it)X
2051(only)X
2214(\256nds)X
7 f
2390(.o)X
1 f
(,)S
2527(so)X
2619(it)X
2683(looks)X
2876(for)X
2990(the)X
3108(\256le)X
7 f
3230(jive.o)X
1 f
(.)S
3578(It)X
3647(fails)X
3805(to)X
3887(\256nd)X
555 3072(it,)N
642(so)X
736(it)X
803(looks)X
999(for)X
1116(transformations)X
1642(into)X
1789(a)X
7 f
1848(.o)X
1 f
1967(\256le.)X
2112(Again)X
2331(it)X
2397(has)X
2526(only)X
2690(one)X
2828(choice:)X
7 f
3082(.c)X
1 f
(.)S
3240(So)X
3346(it)X
3412(looks)X
3607(for)X
7 f
3723(jive.c)X
1 f
555 3168(and,)N
714(as)X
804(you)X
947(know,)X
1168(fails)X
1329(to)X
1414(\256nd)X
1561(it.)X
1648(At)X
1751(this)X
1889(point)X
2075(it)X
2141(has)X
2270(two)X
2412(choices:)X
2697(it)X
2763(can)X
2897(create)X
3112(the)X
7 f
3232(.c)X
1 f
3350(\256le)X
3474(from)X
3652(either)X
3857(a)X
7 f
3915(.y)X
1 f
555 3264(\256le)N
680(or)X
770(a)X
7 f
829(.l)X
1 f
948(\256le.)X
1093(Since)X
7 f
1294(.y)X
1 f
1413(came)X
1606(\256rst)X
1753(on)X
1856(the)X
7 f
1977(.SUFFIXES)X
1 f
2432(line,)X
2595(it)X
2661(checks)X
2902(for)X
7 f
3018(jive.y)X
1 f
3328(\256rst,)X
3494(but)X
3618(can't)X
3801(\256nd)X
3947(it,)X
555 3360(so)N
648(it)X
714(looks)X
909(for)X
7 f
1025(jive.l)X
1 f
1335(and,)X
1493(lo)X
1577(and)X
1715(behold,)X
1975(there)X
2158(it)X
2224(is.)X
2339(At)X
2441(this)X
2578(point,)X
2784(it)X
2850(has)X
2978(de\256ned)X
3235(a)X
3292(transformation)X
3785(path)X
3944(as)X
555 3456(follows:)N
7 f
838(.l)X
1 f
9 f
955 MX
(->)174 987 oc
7 f
1055(.c)X
1 f
9 f
1172 MX
(->)174 987 oc
7 f
1272(.o)X
1 f
9 f
1389 MX
(->)174 987 oc
7 f
1489(.out)X
1 f
1702(and)X
1839(applies)X
2087(the)X
2205(transformation)X
2697(rules)X
2873(accordingly.)X
3292(For)X
3423(completeness,)X
3895(and)X
555 3552(to)N
637(give)X
795(you)X
935(a)X
991(better)X
1194(idea)X
1348(of)X
1435(what)X
1611(PMake)X
1858(actually)X
2132(did)X
2254(with)X
2416(this)X
2551(three-step)X
2888(transformation,)X
3400(this)X
3535(is)X
3608(what)X
3784(PMake)X
555 3648(printed)N
802(for)X
916(the)X
1034(rest)X
1170(of)X
1257(the)X
1375(process:)X
7 f
843 3792(Suff_FindDeps)N
1515(\(jive.o\))X
1043 3888(using)N
1331(existing)X
1763(source)X
2099(jive.c)X
1043 3984(applying)N
1475(.c)X
1619(->)X
1763(.o)X
1907(to)X
2051("jive.c")X
843 4080(Suff_FindDeps)N
1515(\(jive.c\))X
1043 4176(using)N
1331(existing)X
1763(source)X
2099(jive.l)X
1043 4272(applying)N
1475(.l)X
1619(->)X
1763(.c)X
1907(to)X
2051("jive.l")X
843 4368(Suff_FindDeps)N
1515(\(jive.l\))X
843 4464(Examining)N
1323(jive.l...modified)X
2187(17:16:01)X
2619(Oct)X
2811(4,)X
2955(1987...up-to-date)X
843 4560(Examining)N
1323(jive.c...non-existent...out-of-date)X
843 4656(---)N
1035(jive.c)X
1371(---)X
843 4752(lex)N
1035(jive.l)X
843 4848(mv)N
987(lex.yy.c)X
1419(jive.c)X
843 4944(Examining)N
1323(jive.o...non-existent...out-of-date)X
843 5040(---)N
1035(jive.o)X
1371(---)X
843 5136(cc)N
987(-c)X
1131(jive.c)X
843 5232(Examining)N
1323(jive.out...non-existent...out-of-date)X
843 5328(---)N
1035(jive.out)X
1467(---)X
843 5424(cc)N
987(-o)X
1131(jive.out)X
1563(jive.o)X
1 f
755 5596(One)N
920(\256nal)X
1093(question)X
1394(remains:)X
1700(what)X
1886(does)X
2063(PMake)X
2320(do)X
2430(with)X
2602(targets)X
2846(that)X
2996(have)X
3178(no)X
3288(known)X
3536(suf\256x?)X
3784(PMake)X
555 5692(simply)N
797(pretends)X
1094(it)X
1163(actually)X
1442(has)X
1574(a)X
1635(known)X
1878(suf\256x)X
2085(and)X
2226(searches)X
2524(for)X
2642(transformations)X
3169(accordingly.)X
3612(The)X
3761(suf\256x)X
3967(it)X
555 5788(chooses)N
832(is)X
908(the)X
1029(source)X
1262(for)X
1379(the)X
7 f
1500(.NULL)X
1 f
1763(target)X
1969(mentioned)X
2330(later.)X
2516(In)X
2606(the)X
2727(system)X
2972(make\256le,)X
7 f
3290(.out)X
1 f
3504(is)X
3579(chosen)X
3824(as)X
3913(the)X

17 p
%%Page: 17 18
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(17)X
2343(-)X
555 672(``null)N
756(suf\256x'')X
1015(because)X
1293(most)X
1471(people)X
1708(use)X
1838(PMake)X
2087(to)X
2171(create)X
2386(programs.)X
2731(You)X
2891(are,)X
3032(however,)X
3351(free)X
3499(and)X
3637(welcome)X
3949(to)X
555 768(change)N
806(it)X
873(to)X
957(a)X
1015(suf\256x)X
1219(of)X
1308(your)X
1477(own)X
1637(choosing.)X
1988(The)X
2135(null)X
2281(suf\256x)X
2485(is)X
2560(ignored,)X
2847(however,)X
3166(when)X
3362(PMake)X
3611(is)X
3686(in)X
3770(compa-)X
555 864(tibility)N
787(mode)X
985(\(see)X
1135(chapter)X
1392(4\).)X
3 f
555 1056(3.2.)N
715(Including)X
1062(Other)X
1287(Make\256les)X
1 f
755 1180(Just)N
904(as)X
995(for)X
1113(programs,)X
1460(it)X
1528(is)X
1605(often)X
1794(useful)X
2014(to)X
2100(extract)X
2343(certain)X
2586(parts)X
2766(of)X
2857(a)X
2917(make\256le)X
3217(into)X
3365(another)X
3630(\256le)X
3756(and)X
3896(just)X
555 1276(include)N
811(it)X
875(in)X
957(other)X
1142(make\256les)X
1469(somehow.)X
1816(Many)X
2023(compilers)X
2359(allow)X
2557(you)X
2697(say)X
2824(something)X
3177(like)X
7 f
843 1420(#include)N
1275("defs.h")X
1 f
555 1564(to)N
639(include)X
896(the)X
1015(contents)X
1303(of)X
7 f
1391(defs.h)X
1 f
1700(in)X
1783(the)X
1902(source)X
2133(\256le.)X
2276(PMake)X
2524(allows)X
2754(you)X
2895(to)X
2978(do)X
3079(the)X
3198(same)X
3384(thing)X
3569(for)X
3684(make\256les,)X
555 1660(with)N
719(the)X
839(added)X
1053(ability)X
1279(to)X
1363(use)X
1492(variables)X
1804(in)X
1888(the)X
2008(\256lenames.)X
2377(An)X
2497(include)X
2754(directive)X
3056(in)X
3139(a)X
3196(make\256le)X
3493(looks)X
3687(either)X
3891(like)X
555 1756(this:)N
7 f
843 1900(#include)N
1275(<file>)X
1 f
555 2044(or)N
642(this)X
7 f
843 2188(#include)N
1275("file")X
1 f
555 2332(The)N
701(difference)X
1049(between)X
1338(the)X
1457(two)X
1598(is)X
1672(where)X
1890(PMake)X
2138(searches)X
2432(for)X
2547(the)X
2666(\256le:)X
2810(the)X
2928(\256rst)X
3072(way,)X
3246(PMake)X
3493(will)X
3637(look)X
3799(for)X
3913(the)X
555 2428(\256le)N
682(only)X
849(in)X
936(the)X
1059(system)X
1306(make\256le)X
1606(directory)X
1920(\(to)X
2033(\256nd)X
2181(out)X
2307(what)X
2487(that)X
2631(directory)X
2945(is,)X
3042(give)X
3204(PMake)X
3455(the)X
3 f
9 f
3577(-)X
3 f
3621(h)X
1 f
3689(\257ag\).)X
3900(For)X
555 2524(\256les)N
708(in)X
790(double-quotes,)X
1284(the)X
1402(search)X
1628(is)X
1701(more)X
1886(complex:)X
755 2648(1\))N
955(The)X
1100(directory)X
1410(of)X
1497(the)X
1615(make\256le)X
1911(that's)X
2109(including)X
2431(the)X
2549(\256le.)X
755 2772(2\))N
955(The)X
1100(current)X
1348(directory)X
1658(\(the)X
1803(one)X
1939(in)X
2021(which)X
2237(you)X
2377(invoked)X
2655(PMake\).)X
755 2896(3\))N
955(The)X
1100(directories)X
1459(given)X
1657(by)X
1757(you)X
1897(using)X
3 f
9 f
2090(-)X
3 f
2134(I)X
1 f
2185(\257ags,)X
2376(in)X
2458(the)X
2576(order)X
2766(in)X
2848(which)X
3064(you)X
3204(gave)X
3376(them.)X
755 3020(4\))N
955(Directories)X
1332(given)X
1530(by)X
7 f
1630(.PATH)X
1 f
1890(dependency)X
2294(lines)X
2465(\(see)X
2615(chapter)X
2872(4\).)X
755 3144(5\))N
955(The)X
1100(system)X
1342(make\256le)X
1638(directory.)X
555 3268(in)N
637(that)X
777(order.)X
755 3392(You)N
916(are)X
1038(free)X
1187(to)X
1272(use)X
1402(PMake)X
1652(variables)X
1965(in)X
2050(the)X
2171(\256lename\320PMake)X
2777(will)X
2923(expand)X
3177(them)X
3359(before)X
3587(searching)X
3917(for)X
555 3488(the)N
680(\256le.)X
829(You)X
994(must)X
1176(specify)X
1435(the)X
1560(searching)X
1895(method)X
2162(with)X
2331(either)X
2541(angle)X
2742(brackets)X
3037(or)X
3131(double-quotes)X
2 f
3612(outside)X
1 f
3882(of)X
3975(a)X
555 3584(variable)N
834(expansion.)X
1199(I.e.)X
1322(the)X
1440(following)X
7 f
843 3728(SYSTEM)N
1243(=)X
1339(<command.mk>)X
843 3920(#include)N
1275($\(SYSTEM\))X
1 f
555 4064(won't)N
762(work.)X
3 f
555 4256(3.3.)N
715(Saving)X
965(Commands)X
1 f
755 4380(There)N
969(may)X
1133(come)X
1333(a)X
1395(time)X
1562(when)X
1761(you)X
1906(will)X
2055(want)X
2236(to)X
2323(save)X
2491(certain)X
2735(commands)X
3107(to)X
3194(be)X
3295(executed)X
3606(when)X
3805(every-)X
555 4476(thing)N
740(else)X
885(is)X
958(done.)X
1154(For)X
1285(instance:)X
1590(you're)X
1820(making)X
2080(several)X
2328(different)X
2625(libraries)X
2908(at)X
2986(one)X
3122(time)X
3284(and)X
3420(you)X
3560(want)X
3736(to)X
3818(create)X
555 4572(the)N
675(members)X
991(in)X
1075(parallel.)X
1358(Problem)X
1651(is,)X
7 f
1746(ranlib)X
1 f
2056(is)X
2131(another)X
2394(one)X
2532(of)X
2621(those)X
2812(programs)X
3137(that)X
3279(can't)X
3462(be)X
3559(run)X
3687(more)X
3873(than)X
555 4668(once)N
729(in)X
813(the)X
933(same)X
1120(directory)X
1432(at)X
1512(the)X
1632(same)X
1819(time)X
1982(\(each)X
2178(one)X
2315(creates)X
2560(a)X
2617(\256le)X
2740(called)X
7 f
2953(__.SYMDEF)X
1 f
3406(into)X
3551(which)X
3768(it)X
3833(stuffs)X
555 4764(information)N
953(for)X
1067(the)X
1185(linker)X
1392(to)X
1474(use.)X
1621(Two)X
1788(of)X
1875(them)X
2055(running)X
2324(at)X
2402(once)X
2574(will)X
2718(overwrite)X
3046(each)X
3214(other's)X
3457(\256le)X
3579(and)X
3715(the)X
3833(result)X
555 4860(will)N
706(be)X
809(garbage)X
1091(for)X
1212(both)X
1381(parties\).)X
1669(You)X
1834(might)X
2047(want)X
2229(a)X
2291(way)X
2451(to)X
2539(save)X
2708(the)X
2832(ranlib)X
3045(commands)X
3418(til)X
3510(the)X
3634(end)X
3776(so)X
3873(they)X
555 4956(can)N
692(be)X
793(run)X
925(one)X
1066(after)X
1239(the)X
1362(other,)X
1571(thus)X
1728(keeping)X
2006(them)X
2190(from)X
2370(trashing)X
2652(each)X
2824(other's)X
3071(\256le.)X
3217(PMake)X
3468(allows)X
3701(you)X
3845(to)X
3931(do)X
555 5052(this)N
691(by)X
792(inserting)X
1093(an)X
1190(ellipsis)X
1437(\(``.)X
1551(.)X
1584(.''\))X
1706(as)X
1794(a)X
1851(command)X
2188(between)X
2477(commands)X
2845(to)X
2928(be)X
3024(run)X
3151(at)X
3229(once)X
3401(and)X
3537(those)X
3726(to)X
3808(be)X
3904(run)X
555 5148(later.)N
755 5272(So)N
859(for)X
973(the)X
7 f
1091(ranlib)X
1 f
1399(case)X
1558(above,)X
1790(you)X
1930(might)X
2136(do)X
2236(this:)X

18 p
%%Page: 18 19
10 s 10 xH 0 xS 1 f
7 f
1 f
2196 384(-)N
2243(18)X
2343(-)X
7 f
843 720(lib1.a)N
1611(:)X
1707($\(LIB1OBJS\))X
1227 816(rm)N
1371(-f)X
1515($\(.TARGET\))X
1227 912(ar)N
1371(cr)X
1515($\(.TARGET\))X
2043($\(.ALLSRC\))X
1227 1008(...)N
1227 1104(ranlib)N
1563($\(.TARGET\))X
843 1296(lib2.a)N
1611(:)X
1707($\(LIB2OBJS\))X
1227 1392(rm)N
1371(-f)X
1515($\(.TARGET\))X
1227 1488(ar)N
1371(cr)X
1515($\(.TARGET\))X
2043($\(.ALLSRC\))X
1227 1584(...)N
1227 1680(ranlib)N
1563($\(.TARGET\))X
1 f
555 1824(This)N
717(would)X
937(save)X
1100(both)X
7 f
843 1968(ranlib)N
1179($\(.TARGET\))X
1 f
555 2112(commands)N
928(until)X
1100(the)X
1224(end,)X
1386(when)X
1586(they)X
1749(would)X
1974(run)X
2106(one)X
2247(after)X
2420(the)X
2543(other)X
2733(\(using)X
2958(the)X
3081(correct)X
3330(value)X
3529(for)X
3648(the)X
7 f
3771(.TAR-)X
555 2208(GET)N
1 f
719(variable,)X
1018(of)X
1105(course\).)X
755 2332(Commands)N
1159(saved)X
1382(in)X
1484(this)X
1639(manner)X
1920(are)X
2059(only)X
2241(executed)X
2567(if)X
2656(PMake)X
2923(manages)X
3244(to)X
3346 0.4219(re-create)AX
3668(everything)X
555 2428(without)N
819(an)X
915(error.)X
3 f
555 2620(3.4.)N
715(Target)X
967(Attributes)X
1 f
755 2744(PMake)N
1014(allows)X
1255(you)X
1407(to)X
1501(give)X
1671(attributes)X
2001(to)X
2095(targets)X
2341(by)X
2453(means)X
2690(of)X
2788(special)X
3042(sources.)X
3334(Like)X
3512(everything)X
3886(else)X
555 2840(PMake)N
805(uses,)X
986(these)X
1174(sources)X
1438(begin)X
1638(with)X
1802(a)X
1860(period)X
2087(and)X
2225(are)X
2346(made)X
2542(up)X
2644(of)X
2733(all)X
2835(upper-case)X
3206(letters.)X
3444(There)X
3654(are)X
3775(various)X
555 2936(reasons)N
817(for)X
932(using)X
1126(them,)X
1327(and)X
1464(I)X
1512(will)X
1657(try)X
1767(to)X
1850(give)X
2009(examples)X
2333(for)X
2448(most)X
2624(of)X
2712(them.)X
2913(Others)X
3148(you'll)X
3360(have)X
3533(to)X
3615(\256nd)X
3759(uses)X
3917(for)X
555 3032(yourself.)N
865(Think)X
1083(of)X
1177(it)X
1248(as)X
1341(``an)X
1497(exercise)X
1787(for)X
1907(the)X
2031(reader.'')X
2333(By)X
2452(placing)X
2714(one)X
2856(\(or)X
2976(more\))X
3194(of)X
3287(these)X
3478(as)X
3571(a)X
3633(source)X
3869(on)X
3975(a)X
555 3128(dependency)N
961(line,)X
1122(you)X
1263(are)X
1383(``marking)X
1725(the)X
1844(target\(s\))X
2133(with)X
2296(that)X
2437(attribute.'')X
2799(That's)X
3025(just)X
3161(the)X
3280(way)X
3435(I)X
3483(phrase)X
3714(it,)X
3799(so)X
3891(you)X
555 3224(know.)N
755 3348(Any)N
918(attributes)X
1241(given)X
1444(as)X
1536(sources)X
1802(for)X
1920(a)X
1980(transformation)X
2476(rule)X
2625(are)X
2748(applied)X
3008(to)X
3094(the)X
3216(target)X
3423(of)X
3514(the)X
3636(transforma-)X
555 3444(tion)N
699(rule)X
844(when)X
1038(the)X
1156(rule)X
1301(is)X
1374(applied.)X
555 3568(.DONTCARE)N
1148(If)X
1231(a)X
1296(target)X
1507(is)X
1588(marked)X
1857(with)X
2027(this)X
2170(attribute)X
2465(and)X
2609(PMake)X
2864(can't)X
3053(\256gure)X
3268(out)X
3398(how)X
3564(to)X
3654(create)X
3875(it,)X
3967(it)X
1148 3664(will)N
1296(ignore)X
1525(this)X
1663(fact)X
1807(and)X
1946(assume)X
2205(the)X
2326(\256le)X
2451(isn't)X
2616(really)X
2822(needed)X
3073(or)X
3163(actually)X
3440(exists)X
3645(and)X
3784(PMake)X
1148 3760(just)N
1290(can't)X
1478(\256nd)X
1629(it.)X
1720(This)X
1889(may)X
2054(prove)X
2264(wrong,)X
2516(but)X
2645(the)X
2770(error)X
2954(will)X
3105(be)X
3208(noted)X
3413(later)X
3583(on,)X
3709(not)X
3837(when)X
1148 3856(PMake)N
1409(tries)X
1581(to)X
1677(create)X
1904(the)X
2036(target)X
2253(so)X
2358(marked.)X
2653(This)X
2828(attribute)X
3128(also)X
3290(prevents)X
3595(PMake)X
3855(from)X
1148 3952(attempting)N
1510(to)X
1592(touch)X
1790(the)X
1908(target)X
2111(if)X
2180(it)X
2244(is)X
2317(given)X
2515(the)X
3 f
9 f
2633(-)X
3 f
2677(t)X
1 f
2724(\257ag.)X
555 4076(.EXEC)N
1148(This)X
1313(attribute)X
1603(causes)X
1836(its)X
1934(shell)X
2108(script)X
2309(to)X
2394(be)X
2493(executed)X
2802(while)X
3003(having)X
3244(no)X
3347(effect)X
3553(on)X
3655(targets)X
3891(that)X
1148 4172(depend)N
1402(on)X
1504(it.)X
1590(This)X
1754(makes)X
1981(the)X
2101(target)X
2306(into)X
2452(a)X
2510(sort)X
2652(of)X
2741(subroutine.)X
3141(An)X
3261(example.)X
3575(Say)X
3717(you)X
3859(have)X
1148 4268(some)N
1338(LISP)X
1523(\256les)X
1677(that)X
1818(need)X
1991(to)X
2074(be)X
2171(compiled)X
2490(and)X
2627(loaded)X
2862(into)X
3006(a)X
3062(LISP)X
3246(process.)X
3527(To)X
3636(do)X
3736(this,)X
3891(you)X
1148 4364(echo)N
1334(LISP)X
1532(commands)X
1913(into)X
2071(a)X
2141(\256le)X
2277(and)X
2427(execute)X
2707(a)X
2777(LISP)X
2974(with)X
3149(this)X
3297(\256le)X
3432(as)X
3532(its)X
3640(input)X
3837(when)X
1148 4460(everything's)N
1572(done.)X
1771(Say)X
1914(also)X
2066(that)X
2209(you)X
2352(have)X
2527(to)X
2612(load)X
2773(other)X
2961(\256les)X
3117(from)X
3296(another)X
3560(system)X
3805(before)X
1148 4556(you)N
1289(can)X
1422(compile)X
1701(your)X
1869(\256les)X
2023(and)X
2160(further,)X
2420(that)X
2561(you)X
2701(don't)X
2890(want)X
3066(to)X
3148(go)X
3248(through)X
3517(the)X
3635(loading)X
3895(and)X
1148 4652(dumping)N
1456(unless)X
1680(one)X
1820(of)X
2 f
1911(your)X
1 f
2095(\256les)X
2252(has)X
2383(changed.)X
2695(Your)X
2883(make\256le)X
3182(might)X
3391(look)X
3556(a)X
3615(little)X
3784(bit)X
3891(like)X
1148 4748(this)N
1295(\(remember,)X
1700(this)X
1847(is)X
1932(an)X
2040(educational)X
2442(example,)X
2766(and)X
2914(don't)X
3114(worry)X
3337(about)X
3546(the)X
7 f
3675(COMPILE)X
1 f
1148 4844(rule,)N
1313(all)X
1413(will)X
1557(soon)X
1728(become)X
1998(clear,)X
2195(grasshopper\):)X

19 p
%%Page: 19 20
10 s 10 xH 0 xS 1 f
7 f
1 f
2196 384(-)N
2243(19)X
2343(-)X
7 f
1436 720(system)N
2204(:)X
2300(init)X
2540(a.fasl)X
2876(b.fasl)X
3212(c.fasl)X
1820 816(for)N
2012(i)X
2108(in)X
2252($\(.ALLSRC\);)X
1820 912(do)N
2204 1008(echo)N
2444(-n)X
2588('\(load)X
2924("')X
3068(>>)X
3212(input)X
2204 1104(echo)N
2444(-n)X
2588(${i})X
2828(>>)X
2972(input)X
2204 1200(echo)N
2444('"\)')X
2684(>>)X
2828(input)X
1820 1296(done)N
1820 1392(echo)N
2060('\(dump)X
2396("$\(.TARGET\)"\)')X
3116(>>)X
3260(input)X
1820 1488(lisp)N
2060(<)X
2156(input)X
1436 1680(a.fasl)N
2204(:)X
2300(a.l)X
2492(init)X
2732(COMPILE)X
1436 1776(b.fasl)N
2204(:)X
2300(b.l)X
2492(init)X
2732(COMPILE)X
1436 1872(c.fasl)N
2204(:)X
2300(c.l)X
2492(init)X
2732(COMPILE)X
1436 1968(COMPILE)N
2204(:)X
2300(.USE)X
1820 2064(echo)N
2060('\(compile)X
2540("$\(.ALLSRC\)"\)')X
3260(>>)X
3404(input)X
1436 2160(init)N
2204(:)X
2300(.EXEC)X
1820 2256(echo)N
2060('\(load-system\)')X
2828(>)X
2924(input)X
1148 2428(.EXEC)N
1 f
1415(sources,)X
1702(don't)X
1897(appear)X
2138(in)X
2226(the)X
2350(local)X
2532(variables)X
2848(of)X
2941(targets)X
3181(that)X
3327(depend)X
3585(on)X
3691(them)X
3877(\(nor)X
1148 2524(are)N
1272(they)X
1435(touched)X
1714(if)X
1788(PMake)X
2040(is)X
2118(given)X
2321(the)X
3 f
9 f
2444(-)X
3 f
2488(t)X
1 f
2540(\257ag\).)X
2752(Note)X
2933(that)X
3078(all)X
3183(the)X
3306(rules,)X
3507(not)X
3634(just)X
3773(that)X
3917(for)X
7 f
1148 2620(system)N
1 f
(,)S
1487(include)X
7 f
1754(init)X
1 f
1977(as)X
2075(a)X
2142(source.)X
2403(This)X
2575(is)X
2658(because)X
2943(none)X
3129(of)X
3226(the)X
3354(other)X
3549(targets)X
3793(can)X
3935(be)X
1148 2716(made)N
1342(until)X
7 f
1508(init)X
1 f
1720(has)X
1847(been)X
2019(made,)X
2233(thus)X
2386(they)X
2544(depend)X
2796(on)X
2896(it.)X
555 2840(.EXPORT)N
1148(This)X
1312(is)X
1387(used)X
1556(to)X
1640(mark)X
1827(those)X
2018(targets)X
2254(whose)X
2481(creation)X
2762(should)X
2997(be)X
3095(sent)X
3245(to)X
3328(another)X
3590(machine)X
3883(if)X
3953(at)X
1148 2936(all)N
1252(possible.)X
1558(This)X
1724(may)X
1886(be)X
1986(used)X
2157(by)X
2260(some)X
2452(exportation)X
2840(schemes)X
3135(if)X
3207(the)X
3328(exportation)X
3716(is)X
3792(expen-)X
1148 3032(sive.)N
1317(You)X
1475(should)X
1708(ask)X
1835(your)X
2002(system)X
2244(administrator)X
2691(if)X
2760(it)X
2824(is)X
2897(necessary.)X
555 3156(.EXPORTSAME)N
1148(Tells)X
1333(the)X
1456(export)X
1686(system)X
1933(that)X
2078(the)X
2201(job)X
2328(should)X
2566(be)X
2667(exported)X
2973(to)X
3060(a)X
3121(machine)X
3418(of)X
3510(the)X
3633(same)X
3823(archi-)X
1148 3252(tecture)N
1391(as)X
1482(the)X
1604(current)X
1856(one.)X
2016(Certain)X
2276(operations)X
2634(\(e.g.)X
2821(running)X
3094(text)X
3238(through)X
7 f
3510(nroff)X
1 f
(\))S
3800(can)X
3935(be)X
1148 3348(performed)N
1509(the)X
1633(same)X
1824(on)X
1930(any)X
2072(architecture)X
2478(\(CPU)X
2686(and)X
2827(operating)X
3155(system)X
3402(type\),)X
3612(while)X
3815(others)X
1148 3444(\(e.g.)N
1325(compiling)X
1683(a)X
1752(program)X
2057(with)X
7 f
2232(cc)X
1 f
(\))S
2388(must)X
2576(be)X
2685(performed)X
3053(on)X
3166(a)X
3235(machine)X
3540(with)X
3715(the)X
3846(same)X
1148 3540(architecture.)N
1568(Not)X
1708(all)X
1808(export)X
2033(systems)X
2306(will)X
2450(support)X
2710(this)X
2845(attribute.)X
555 3664(.IGNORE)N
1148(Giving)X
1398(a)X
1462(target)X
1673(the)X
7 f
1799(.IGNORE)X
1 f
2163(attribute)X
2458(causes)X
2696(PMake)X
2951(to)X
3041(ignore)X
3274(errors)X
3490(from)X
3674(any)X
3818(of)X
3913(the)X
1148 3760(target's)N
1409(commands,)X
1796(as)X
1883(if)X
1952(they)X
2110(all)X
2210(had)X
2346(`)X
9 f
2373(-)X
1 f
2417(')X
2464(before)X
2690(them.)X
555 3884(.INVISIBLE)N
1148(This)X
1318(allows)X
1555(you)X
1702(to)X
1791(specify)X
2050(one)X
2193(target)X
2403(as)X
2497(a)X
2560(source)X
2797(for)X
2918(another)X
3186(without)X
3457(the)X
3582(one)X
3725(affecting)X
1148 3980(the)N
1268(other's)X
1512(local)X
1689(variables.)X
2020(Useful)X
2255(if,)X
2345(say,)X
2493(you)X
2634(have)X
2807(a)X
2864(make\256le)X
3161(that)X
3302(creates)X
3547(two)X
3688(programs,)X
1148 4076(one)N
1287(of)X
1377(which)X
1596(is)X
1672(used)X
1842(to)X
1927(create)X
2143(the)X
2264(other,)X
2472(so)X
2566(it)X
2633(must)X
2811(exist)X
2985(before)X
3214(the)X
3335(other)X
3523(is)X
3598(created.)X
3873(You)X
1148 4172(could)N
1346(say)X
7 f
1436 4316(prog1)N
2204(:)X
2300($\(PROG1OBJS\))X
2924(prog2)X
3212(MAKEINSTALL)X
1436 4412(prog2)N
2204(:)X
2300($\(PROG2OBJS\))X
2924(.INVISIBLE)X
3452(MAKEINSTALL)X
1 f
1148 4556(where)N
7 f
1381(MAKEINSTALL)X
1 f
1945(is)X
2034(some)X
2239(complex)X
2551(.USE)X
2758(rule)X
2919(\(see)X
3085(below\))X
3344(that)X
3500(depends)X
3798(on)X
3913(the)X
7 f
1148 4652(.ALLSRC)N
1 f
1511(variable)X
1797(containing)X
2162(the)X
2287(right)X
2465(things.)X
2706(Without)X
2994(the)X
7 f
3118(.INVISIBLE)X
1 f
3624(attribute)X
3917(for)X
7 f
1148 4748(prog2)N
1 f
(,)S
1435(the)X
7 f
1560(MAKEINSTALL)X
1 f
2115(rule)X
2267(couldn't)X
2561(be)X
2663(applied.)X
2945(This)X
3113(is)X
3192(not)X
3320(as)X
3413(useful)X
3635(as)X
3728(it)X
3798(should)X
1148 4844(be,)N
1274(and)X
1420(the)X
1548(semantics)X
1894(may)X
2062(change)X
2319(\(or)X
2442(the)X
2569(whole)X
2794(thing)X
2987(go)X
3096(away\))X
3322(in)X
3413(the)X
3540(not-too-distant)X
1148 4940(future.)N
555 5064(.JOIN)N
1148(This)X
1322(is)X
1407(another)X
1680(way)X
1845(to)X
1938(avoid)X
2147(performing)X
2539(some)X
2739(operations)X
3104(in)X
3197(parallel)X
3469(while)X
3678(permitting)X
1148 5160(everything)N
1521(else)X
1676(to)X
1768(be)X
1874(done)X
2060(so.)X
2181(Speci\256cally)X
2589(it)X
2663(forces)X
2890(the)X
3018(target's)X
3289(shell)X
3469(script)X
3676(to)X
3767(be)X
3872(exe-)X
1148 5256(cuted)N
1346(only)X
1512(if)X
1585(one)X
1725(or)X
1816(more)X
2005(of)X
2096(the)X
2218(sources)X
2483(was)X
2632(out-of-date.)X
3033(In)X
3124(addition,)X
3430(the)X
3552(target's)X
3817(name,)X
1148 5352(in)N
1233(both)X
1398(its)X
7 f
1496(.TARGET)X
1 f
1855(variable)X
2136(and)X
2274(all)X
2376(the)X
2496(local)X
2674(variables)X
2986(of)X
3075(any)X
3213(target)X
3418(that)X
3560(depends)X
3845(on)X
3947(it,)X
1148 5448(is)N
1227(replaced)X
1526(by)X
1632(the)X
1756(value)X
1956(of)X
2049(its)X
7 f
2150(.ALLSRC)X
1 f
2512(variable.)X
2837(As)X
2952(an)X
3053(example,)X
3370(suppose)X
3653(you)X
3798(have)X
3975(a)X
1148 5544(program)N
1445(that)X
1590(has)X
1721(four)X
1879(libraries)X
2166(that)X
2310(compile)X
2592(in)X
2678(the)X
2800(same)X
2989(directory)X
3303(along)X
3505(with,)X
3691(and)X
3831(at)X
3913(the)X
1148 5640(same)N
1337(time)X
1503(as,)X
1614(the)X
1736(program.)X
2052(You)X
2214(again)X
2412(have)X
2588(the)X
2710(problem)X
3001(with)X
7 f
3167(ranlib)X
1 f
3479(that)X
3623(I)X
3673(mentioned)X
1148 5736(earlier,)N
1399(only)X
1566(this)X
1706(time)X
1873(it's)X
2000(more)X
2190(severe:)X
2443(you)X
2588(can't)X
2774(just)X
2914(put)X
3041(the)X
3164(ranlib)X
3376(off)X
3495(to)X
3582(the)X
3705(end)X
3846(since)X
1148 5832(the)N
1271(program)X
1568(will)X
1717(need)X
1894(those)X
2088(libraries)X
2375(before)X
2605(it)X
2673(can)X
2809(be)X
2909 0.3375(re-created.)AX
3276(You)X
3438(can)X
3574(do)X
3678(something)X

20 p
%%Page: 20 21
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(20)X
2343(-)X
1148 672(like)N
1288(this:)X
7 f
1436 816(program)N
2204(:)X
2300($\(OBJS\))X
2684(libraries)X
1820 912(cc)N
1964(-o)X
2108($\(.TARGET\))X
2636($\(.ALLSRC\))X
1436 1104(libraries)N
2204(:)X
2300(lib1.a)X
2636(lib2.a)X
2972(lib3.a)X
3308(lib4.a)X
3644(.JOIN)X
1820 1200(ranlib)N
2156($\(.OODATE\))X
1 f
1148 1344(In)N
1251(this)X
1402(case,)X
1597(PMake)X
1860(will)X
2020 0.4219(re-create)AX
2339(the)X
7 f
2472($\(OBJS\))X
1 f
2843(as)X
2945(necessary,)X
3313(along)X
3526(with)X
7 f
3703(lib1.a)X
1 f
(,)S
7 f
1148 1440(lib2.a)N
1 f
(,)S
7 f
1481(lib3.a)X
1 f
1794(and)X
7 f
1935(lib4.a)X
1 f
(.)S
2288(It)X
2362(will)X
2511(then)X
2674(execute)X
7 f
2945(ranlib)X
1 f
3258(on)X
3363(any)X
3504(library)X
3742(that)X
3886(was)X
1148 1536(changed)N
1451(and)X
1601(set)X
7 f
1724(program)X
1 f
('s)S
7 f
2152(.ALLSRC)X
1 f
2522(variable)X
2815(to)X
2911(contain)X
3181(what's)X
3429(in)X
7 f
3525($\(OBJS\))X
1 f
3895(fol-)X
1148 1632(lowed)N
1379(by)X
1494(``)X
7 f
1548(lib1.a)X
1898(lib2.a)X
2248(lib3.a)X
2598(lib4.a)X
1 f
(.'')S
2994(In)X
3095(case)X
3268(you're)X
3512(wondering,)X
3909(it's)X
1148 1728(called)N
7 f
1363(.JOIN)X
1 f
1626(because)X
1904(it)X
1971(joins)X
2149(together)X
2435(different)X
2735(threads)X
2990(of)X
3080(the)X
3200(``input)X
3440(graph'')X
3699(at)X
3779(the)X
3899(tar-)X
1148 1824(get)N
1268(marked)X
1531(with)X
1695(the)X
1815(attribute.)X
2144(Another)X
2429(aspect)X
2652(of)X
2741(the)X
2861(.JOIN)X
3077(attribute)X
3366(is)X
3440(it)X
3505(keeps)X
3709(the)X
3828(target)X
1148 1920(from)N
1324(being)X
1522(created)X
1775(if)X
1844(the)X
3 f
9 f
1962(-)X
3 f
2006(t)X
1 f
2053(\257ag)X
2193(was)X
2338(given.)X
555 2044(.MAKE)N
1148(The)X
7 f
1303(.MAKE)X
1 f
1573(attribute)X
1870(marks)X
2096(its)X
2201(target)X
2414(as)X
2511(being)X
2719(a)X
2785(recursive)X
3110(invocation)X
3477(of)X
3573(PMake.)X
3869(This)X
1148 2140(forces)N
1370(PMake)X
1622(to)X
1709(execute)X
1979(the)X
2101(script)X
2303(associated)X
2657(with)X
2823(the)X
2945(target)X
3152(\(if)X
3252(it's)X
3378(out-of-date\))X
3786(even)X
3962(if)X
1148 2236(you)N
1288(gave)X
1460(the)X
3 f
9 f
1578(-)X
3 f
1622(n)X
1 f
1686(or)X
3 f
9 f
1773(-)X
3 f
1817(t)X
1 f
1864(\257ag.)X
2024(By)X
2137(doing)X
2339(this,)X
2494(you)X
2634(can)X
2766(start)X
2924(at)X
3002(the)X
3120(top)X
3242(of)X
3329(a)X
3385(system)X
3627(and)X
3763(type)X
7 f
1436 2380(pmake)N
1724(-n)X
1 f
1148 2524(and)N
1293(have)X
1474(it)X
1547(descend)X
1835(the)X
1962(directory)X
2281(tree)X
2431(\(if)X
2535(your)X
2710(make\256les)X
3045(are)X
3172(set)X
3289(up)X
3397(correctly\),)X
3758(printing)X
1148 2620(what)N
1324(it)X
1388(would)X
1608(have)X
1780(executed)X
2086(if)X
2155(you)X
2295(hadn't)X
2520(included)X
2816(the)X
3 f
9 f
2934(-)X
3 f
2978(n)X
1 f
3042(\257ag.)X
555 2744(.NOEXPORT)N
1148(If)X
1229(possible,)X
1538(PMake)X
1792(will)X
1943(attempt)X
2210(to)X
2299(export)X
2531(the)X
2656(creation)X
2942(of)X
3036(all)X
3143(targets)X
3384(to)X
3472(another)X
3739(machine)X
1148 2840(\(this)N
1313(depends)X
1599(on)X
1702(how)X
1863(PMake)X
2112(was)X
2259(con\256gured\).)X
2671(Sometimes,)X
3068(the)X
3188(creation)X
3469(is)X
3544(so)X
3637(simple,)X
3892(it)X
3958(is)X
1148 2936(pointless)N
1455(to)X
1540(send)X
1710(it)X
1777(to)X
1862(another)X
2126(machine.)X
2441(If)X
2518(you)X
2661(give)X
2822(the)X
2943(target)X
3149(the)X
7 f
3270(.NOEXPORT)X
1 f
3724(attribute,)X
1148 3032(it)N
1212(will)X
1356(be)X
1452(run)X
1579(locally,)X
1837(even)X
2009(if)X
2078(you've)X
2321(given)X
2519(PMake)X
2766(the)X
3 f
9 f
2884(-)X
3 f
2928(L)X
3001(0)X
1 f
3061(\257ag.)X
555 3156(.NOTMAIN)N
1148(Normally,)X
1500(if)X
1574(you)X
1719(do)X
1824(not)X
1951(specify)X
2208(a)X
2269(target)X
2477(to)X
2564(make)X
2763(in)X
2849(any)X
2989(other)X
3178(way,)X
3356(PMake)X
3607(will)X
3755(take)X
3913(the)X
1148 3252(\256rst)N
1294(target)X
1499(on)X
1601(the)X
1721(\256rst)X
1867(dependency)X
2273(line)X
2415(of)X
2504(a)X
2561(make\256le)X
2858(as)X
2946(the)X
3065(target)X
3269(to)X
3352(create.)X
3586(That)X
3754(target)X
3958(is)X
1148 3348(known)N
1395(as)X
1491(the)X
1618(``Main)X
1870(Target'')X
2163(and)X
2307(is)X
2388(labeled)X
2648(as)X
2743(such)X
2918(if)X
2995(you)X
3143(print)X
3322(the)X
3448(dependencies)X
3909(out)X
1148 3444(using)N
1348(the)X
3 f
9 f
1473(-)X
3 f
1517(p)X
1 f
1588(\257ag.)X
1775(Giving)X
2024(a)X
2087(target)X
2297(this)X
2439(attribute)X
2733(tells)X
2893(PMake)X
3147(that)X
3294(the)X
3419(target)X
3629(is)X
3709(de\256nitely)X
2 f
1148 3540(not)N
1 f
1287(the)X
1409(Main)X
1602(Target.)X
1876(This)X
2042(allows)X
2275(you)X
2419(to)X
2505(place)X
2699(targets)X
2937(in)X
3023(an)X
3122(included)X
3421(make\256le)X
3720(and)X
3859(have)X
1148 3636(PMake)N
1395(create)X
1608(something)X
1961(else)X
2106(by)X
2206(default.)X
555 3760(.PRECIOUS)N
1148(When)X
1362(PMake)X
1611(is)X
1686(interrupted)X
2060(\(you)X
2229(type)X
2389(control-C)X
2718(at)X
2798(the)X
2918(keyboard\),)X
3286(it)X
3352(will)X
3497(attempt)X
3758(to)X
3841(clean)X
1148 3856(up)N
1253(after)X
1426(itself)X
1611(by)X
1716(removing)X
2048(any)X
2189(half-made)X
2540(targets.)X
2799(If)X
2878(a)X
2939(target)X
3146(has)X
3277(the)X
7 f
3399(.PRECIOUS)X
1 f
3855(attri-)X
1148 3952(bute,)N
1329(however,)X
1649(PMake)X
1899(will)X
2046(leave)X
2238(it)X
2304(alone.)X
2520(An)X
2640(additional)X
2982(side)X
3133(effect)X
3339(of)X
3428(the)X
3548(`::')X
3668(operator)X
3958(is)X
1148 4048(to)N
1230(mark)X
1415(the)X
1533(targets)X
1767(as)X
7 f
1854(.PRECIOUS)X
1 f
(.)S
555 4172(.SILENT)N
1148(Marking)X
1446(a)X
1504(target)X
1709(with)X
1872(this)X
2008(attribute)X
2296(keeps)X
2500(its)X
2596(commands)X
2964(from)X
3141(being)X
3340(printed)X
3588(when)X
3783(they're)X
1148 4268(executed,)N
1474(just)X
1609(as)X
1696(if)X
1765(they)X
1923(had)X
2059(an)X
2155(`@')X
2303(in)X
2385(front)X
2561(of)X
2648(them.)X
555 4392(.USE)N
1148(By)X
1268(giving)X
1499(a)X
1562(target)X
1771(this)X
1912(attribute,)X
2225(you)X
2371(turn)X
2526(it)X
2596(into)X
2746(PMake's)X
3057(equivalent)X
3417(of)X
3510(a)X
3572(macro.)X
3819(When)X
1148 4488(the)N
1270(target)X
1477(is)X
1554(used)X
1725(as)X
1816(a)X
1876(source)X
2110(for)X
2227(another)X
2491(target,)X
2717(the)X
2838(other)X
3026(target)X
3232(acquires)X
3523(the)X
3644(commands,)X
1148 4584(sources)N
1412(and)X
1551(attributes)X
1872(\(except)X
7 f
2132(.USE)X
1 f
(\))S
2374(of)X
2463(the)X
2583(source.)X
2855(If)X
2931(the)X
3051(target)X
3256(already)X
3515(has)X
3644(commands,)X
1148 4680(the)N
7 f
1284(.USE)X
1 f
1514(target's)X
1793(commands)X
2178(are)X
2315(added)X
2544(to)X
2643(the)X
2778(end.)X
2951(If)X
3042(more)X
3244(than)X
3419(one)X
3572(.USE-marked)X
1148 4776(source)N
1378(is)X
1451(given)X
1649(to)X
1731(a)X
1787(target,)X
2010(the)X
2128(rules)X
2304(are)X
2423(applied)X
2679(sequentially.)X
1148 4900(The)N
1303(typical)X
1551(.USE)X
1752(rule)X
1907(\(as)X
2031(I)X
2087(call)X
2232(them\))X
2448(will)X
2601(use)X
2737(the)X
2864(sources)X
3134(of)X
3230(the)X
3357(target)X
3569(to)X
3660(which)X
3885(it)X
3958(is)X
1148 4996(applied)N
1414(\(as)X
1537(stored)X
1762(in)X
1853(the)X
7 f
1980(.ALLSRC)X
1 f
2345(variable)X
2633(for)X
2756(the)X
2883(target\))X
3122(as)X
3218(its)X
3322(``arguments,'')X
3813(if)X
3891(you)X
1148 5092(will.)N
1340(For)X
1479(example,)X
1799(you)X
1947(probably)X
2260(noticed)X
2524(that)X
2672(the)X
2798(commands)X
3173(for)X
3294(creating)X
7 f
3580(lib1.a)X
1 f
3895(and)X
7 f
1148 5188(lib2.a)N
1 f
1465(in)X
1556(the)X
1683(example)X
1984(in)X
2075(section)X
2331(3.3)X
2460(were)X
2646(exactly)X
2907(the)X
3034(same.)X
3248(You)X
3415(can)X
3556(use)X
3692(the)X
7 f
3819(.USE)X
1 f
1148 5284(attribute)N
1435(to)X
1517(eliminate)X
1835(the)X
1953(repetition,)X
2300(like)X
2440(so:)X

21 p
%%Page: 21 22
10 s 10 xH 0 xS 1 f
7 f
1 f
2196 384(-)N
2243(21)X
2343(-)X
7 f
1436 720(lib1.a)N
2204(:)X
2300($\(LIB1OBJS\))X
2876(MAKELIB)X
1436 816(lib2.a)N
2204(:)X
2300($\(LIB2OBJS\))X
2876(MAKELIB)X
1436 1008(MAKELIB)N
2204(:)X
2300(.USE)X
1820 1104(rm)N
1964(-f)X
2108($\(.TARGET\))X
1820 1200(ar)N
1964(cr)X
2108($\(.TARGET\))X
2636($\(.ALLSRC\))X
1820 1296(...)N
1820 1392(ranlib)N
2156($\(.TARGET\))X
1 f
1148 1564(Several)N
1418(system)X
1669(make\256les)X
2005(\(not)X
2163(to)X
2254(be)X
2359(confused)X
2678(with)X
2849(The)X
3003(System)X
3267(Make\256le\))X
3607(make)X
3809(use)X
3944(of)X
1148 1660(these)N
1373(.USE)X
1584(rules)X
1780(to)X
1882(make)X
2096(your)X
2283(life)X
2430(easier)X
2658(\(they're)X
2953(in)X
3055(the)X
3192(default,)X
3474(system)X
3735(make\256le)X
1148 1756(directory...take)N
1670(a)X
1744(look\).)X
1991(Note)X
2185(that)X
2343(the)X
2479(.USE)X
2688(rule)X
2851(source)X
3099(itself)X
3297(\()X
7 f
3324(MAKELIB)X
1 f
(\))S
3725(does)X
3909(not)X
1148 1852(appear)N
1387(in)X
1473(any)X
1613(of)X
1704(the)X
1826(targets's)X
2122(local)X
2302(variables.)X
2656(There)X
2868(is)X
2945(no)X
3049(limit)X
3223(to)X
3309(the)X
3430(number)X
3698(of)X
3788(times)X
3984(I)X
1148 1948(could)N
1369(use)X
1519(the)X
7 f
1660(MAKELIB)X
1 f
2039(rule.)X
2227(If)X
2324(there)X
2528(were)X
2728(more)X
2936(libraries,)X
3262(I)X
3331(could)X
3551(continue)X
3869(with)X
1148 2044(``)N
7 f
1202(lib3.a)X
1538(:)X
1634($\(LIB3OBJS\))X
2210(MAKELIB)X
1 f
('')S
2620(and)X
2756(so)X
2847(on)X
2947(and)X
3083(so)X
3174(forth.)X
3 f
555 2236(3.5.)N
715(Special)X
979(Targets)X
1 f
755 2360(As)N
870(there)X
1057(were)X
1240(in)X
1327(Make,)X
1555(so)X
1651(there)X
1837(are)X
1961(certain)X
2205(targets)X
2444(that)X
2589(have)X
2766(special)X
3014(meaning)X
3315(to)X
3402(PMake.)X
3674(When)X
3891(you)X
555 2456(use)N
683(one)X
820(on)X
921(a)X
978(dependency)X
1383(line,)X
1544(it)X
1609(is)X
1683(the)X
1802(only)X
1965(target)X
2169(that)X
2310(may)X
2469(appear)X
2705(on)X
2805(the)X
2923(left-hand-side)X
3389(of)X
3476(the)X
3594(operator.)X
3922(As)X
555 2552(for)N
674(the)X
797(attributes)X
1120(and)X
1261(variables,)X
1596(all)X
1701(the)X
1823(special)X
2070(targets)X
2308(begin)X
2510(with)X
2676(a)X
2736(period)X
2965(and)X
3105(consist)X
3351(of)X
3442(upper-case)X
3815(letters)X
555 2648(only.)N
758(I)X
806(won't)X
1014(describe)X
1303(them)X
1484(all)X
1585(in)X
1668(detail)X
1867(because)X
2143(some)X
2333(of)X
2421(them)X
2602(are)X
2722(rather)X
2931(complex)X
3227(and)X
3363(I'll)X
3481(describe)X
3769(them)X
3949(in)X
555 2744(more)N
740(detail)X
938(than)X
1096(you'll)X
1307(want)X
1483(in)X
1565(chapter)X
1822(4.)X
1922(The)X
2067(targets)X
2301(are)X
2420(as)X
2507(follows:)X
555 2868(.BEGIN)N
1104(Any)X
1265(commands)X
1635(attached)X
1926(to)X
2011(this)X
2149(target)X
2355(are)X
2477(executed)X
2786(before)X
3015(anything)X
3318(else)X
3466(is)X
3541(done.)X
3739(You)X
3899(can)X
1104 2964(use)N
1231(it)X
1295(for)X
1409(any)X
1545(initialization)X
1969(that)X
2109(needs)X
2312(doing.)X
555 3088(.DEFAULT)N
1104(This)X
1267(is)X
1341(sort)X
1482(of)X
1570(a)X
1627(.USE)X
1819(rule)X
1965(for)X
2080(any)X
2217(target)X
2421(\(that)X
2589(was)X
2734(used)X
2901(only)X
3063(as)X
3150(a)X
3206(source\))X
3463(that)X
3603(PMake)X
3850(can't)X
1104 3184(\256gure)N
1315(out)X
1441(any)X
1581(other)X
1770(way)X
1928(to)X
2014(create.)X
2251(It's)X
2382(only)X
2548(``sort)X
2746(of'')X
2891(a)X
2951(.USE)X
3146(rule)X
3295(because)X
3574(only)X
3739(the)X
3860(shell)X
1104 3280(script)N
1310(attached)X
1606(to)X
1696(the)X
7 f
1822(.DEFAULT)X
1 f
2234(target)X
2445(is)X
2526(used.)X
2721(The)X
7 f
2874(.IMPSRC)X
1 f
3238(variable)X
3524(of)X
3618(a)X
3681(target)X
3891(that)X
1104 3376(inherits)N
7 f
1364(.DEFAULT)X
1 f
('s)S
1826(commands)X
2193(is)X
2266(set)X
2375(to)X
2457(the)X
2575(target's)X
2836(own)X
2994(name.)X
555 3500(.END)N
1104(This)X
1274(serves)X
1503(a)X
1567(function)X
1861(similar)X
2110(to)X
7 f
2199(.BEGIN)X
1 f
(,)S
2534(in)X
2623(that)X
2770(commands)X
3144(attached)X
3439(to)X
3528(it)X
3599(are)X
3725(executed)X
1104 3596(once)N
1289(everything)X
1664(has)X
1803(been)X
1987 0.3750(re-created)AX
2342(\(so)X
2472(long)X
2646(as)X
2745(no)X
2857(errors)X
3077(occurred\).)X
3438(It)X
3519(also)X
3680(serves)X
3913(the)X
1104 3692(extra)N
1293(function)X
1588(of)X
1682(being)X
1887(a)X
1950(place)X
2147(on)X
2254(which)X
2477(PMake)X
2731(can)X
2870(hang)X
3053(commands)X
3427(you)X
3574(put)X
3703(off)X
3824(to)X
3913(the)X
1104 3788(end.)N
1263(Thus)X
1446(the)X
1567(script)X
1768(for)X
1885(this)X
2023(target)X
2229(will)X
2376(be)X
2474(executed)X
2782(before)X
3010(any)X
3148(of)X
3237(the)X
3357(commands)X
3726(you)X
3868(save)X
1104 3884(with)N
1266(the)X
1384(``.)X
1471(.)X
1504(.''.)X
555 4008(.EXPORT)N
1104(The)X
1262(sources)X
1536(for)X
1663(this)X
1811(target)X
2026(are)X
2157(passed)X
2403(to)X
2497(the)X
2627(exportation)X
3024(system)X
3278(compiled)X
3608(into)X
3764(PMake.)X
1104 4104(Some)N
1307(systems)X
1581(will)X
1726(use)X
1854(these)X
2040(sources)X
2302(to)X
2385(con\256gure)X
2708(themselves.)X
3104(You)X
3262(should)X
3495(ask)X
3622(your)X
3789(system)X
1104 4200(administrator)N
1551(about)X
1749(this.)X
555 4324(.IGNORE)N
1104(This)X
1268(target)X
1473(marks)X
1691(each)X
1861(of)X
1950(its)X
2047(sources)X
2310(with)X
2474(the)X
7 f
2594(.IGNORE)X
1 f
2952(attribute.)X
3261(If)X
3337(you)X
3479(don't)X
3670(give)X
3830(it)X
3895(any)X
1104 4420(sources,)N
1387(then)X
1546(it)X
1611(is)X
1685(like)X
1826(giving)X
2051(the)X
3 f
9 f
2170(-)X
3 f
2214(i)X
1 f
2257(\257ag)X
2398(when)X
2593(you)X
2734(invoke)X
2973(PMake)X
3221(\320)X
3322(errors)X
3531(are)X
3651(ignored)X
3917(for)X
1104 4516(all)N
1204(commands.)X
555 4640(.INCLUDES)N
1104(The)X
1253(sources)X
1518(for)X
1635(this)X
1773(target)X
1979(are)X
2101(taken)X
2298(to)X
2383(be)X
2482(suf\256xes)X
2754(that)X
2897(indicate)X
3174(a)X
3233(\256le)X
3358(that)X
3501(can)X
3636(be)X
3735(included)X
1104 4736(in)N
1192(a)X
1254(program)X
1552(source)X
1788(\256le.)X
1956(The)X
2107(suf\256x)X
2315(must)X
2496(have)X
2674(already)X
2937(been)X
3114(declared)X
3412(with)X
7 f
3579(.SUFFIXES)X
1 f
1104 4832(\(see)N
1273(below\).)X
1575(Any)X
1752(suf\256x)X
1973(so)X
2083(marked)X
2363(will)X
2526(have)X
2717(the)X
2853(directories)X
3230(on)X
3348(its)X
3461(search)X
3705(path)X
3881(\(see)X
7 f
1104 4928(.PATH)N
1 f
(,)S
1398(below\))X
1655(placed)X
1898(in)X
1993(the)X
7 f
2124(.INCLUDES)X
1 f
2589(variable,)X
2901(each)X
3082(preceded)X
3406(by)X
3519(a)X
3 f
9 f
3588(-)X
3 f
3632(I)X
1 f
3696(\257ag.)X
3869(This)X
1104 5024(variable)N
1388(can)X
1525(then)X
1688(be)X
1789(used)X
1960(as)X
2051(an)X
2151(argument)X
2478(for)X
2596(the)X
2718(compiler)X
3027(in)X
3113(the)X
3235(normal)X
3486(fashion.)X
3766(The)X
7 f
3915(.h)X
1 f
1104 5120(suf\256x)N
1306(is)X
1379(already)X
1636(marked)X
1897(in)X
1979(this)X
2114(way)X
2268(in)X
2350(the)X
2468(system)X
2710(make\256le.)X
3046(E.g.)X
3195(if)X
3264(you)X
3404(have)X
7 f
1392 5264(.SUFFIXES)N
2160(:)X
2256(.bitmap)X
1392 5360(.PATH.bitmap)N
2160(:)X
2256(/usr/local/X/lib/bitmaps)X
1392 5456(.INCLUDES)N
2160(:)X
2256(.bitmap)X
1 f
1104 5600(PMake)N
1357(will)X
1507(place)X
1703(``)X
7 f
1757(-I/usr/local/X/lib/bitmaps)X
1 f
('')S
3085(in)X
3172(the)X
7 f
3295(.INCLUDES)X
1 f
3752(variable)X
1104 5696(and)N
1240(you)X
1380(can)X
1512(then)X
1670(say)X

22 p
%%Page: 22 23
10 s 10 xH 0 xS 1 f
7 f
1 f
2196 384(-)N
2243(22)X
2343(-)X
7 f
1392 720(cc)N
1536($\(.INCLUDES\))X
2160(-c)X
2304(xprogram.c)X
1 f
1104 864(\(Note:)N
1332(the)X
7 f
1453(.INCLUDES)X
1 f
1908(variable)X
2190(is)X
2266(not)X
2391(actually)X
2668(\256lled)X
2855(in)X
2939(until)X
3107(the)X
3227(entire)X
3432(make\256le)X
3730(has)X
3859(been)X
1104 960(read.\))N
555 1084(.INTERRUPT)N
1104(When)X
1319(PMake)X
1569(is)X
1645(interrupted,)X
2040(it)X
2107(will)X
2254(execute)X
2523(the)X
2644(commands)X
3014(in)X
3098(the)X
3218(script)X
3418(for)X
3534(this)X
3671(target,)X
3896(if)X
3967(it)X
1104 1180(exists.)N
555 1304(.LIBS)N
1104(This)X
1276(does)X
1453(for)X
1577(libraries)X
1870(what)X
7 f
2056(.INCLUDES)X
1 f
2518(does)X
2695(for)X
2819(include)X
3085(\256les,)X
3267(except)X
3506(the)X
3633(\257ag)X
3782(used)X
3958(is)X
3 f
9 f
1104 1400(-)N
3 f
1148(L)X
1 f
1201(,)X
1250(as)X
1346(required)X
1643(by)X
1752(those)X
1950(linkers)X
2197(that)X
2346(allow)X
2552(you)X
2700(to)X
2790(tell)X
2920(them)X
3108(where)X
3333(to)X
3423(\256nd)X
3575(libraries.)X
3886(The)X
1104 1496(variable)N
1391(used)X
1566(is)X
7 f
1647(.LIBS)X
1 f
(.)S
1954(Be)X
2070(forewarned)X
2464(that)X
2611(PMake)X
2865(may)X
3030(not)X
3159(have)X
3338(been)X
3517(compiled)X
3842(to)X
3931(do)X
1104 1592(this)N
1247(if)X
1324(the)X
1450(linker)X
1665(on)X
1773(your)X
1948(system)X
2198(doesn't)X
2462(accept)X
2695(the)X
3 f
9 f
2820(-)X
3 f
2864(L)X
1 f
2944(\257ag,)X
3111(though)X
3360(the)X
7 f
3485(.LIBS)X
1 f
3752(variable)X
1104 1688(will)N
1248(always)X
1491(be)X
1587(de\256ned)X
1843(once)X
2015(the)X
2133(make\256le)X
2429(has)X
2556(been)X
2728(read.)X
555 1812(.MAIN)N
1104(If)X
1182(you)X
1326(didn't)X
1541(give)X
1703(a)X
1763(target)X
1969(\(or)X
2086(targets\))X
2350(to)X
2435(create)X
2651(when)X
2848(you)X
2991(invoked)X
3272(PMake,)X
3542(it)X
3609(will)X
3756(take)X
3913(the)X
1104 1908(sources)N
1365(of)X
1452(this)X
1587(target)X
1790(as)X
1877(the)X
1995(targets)X
2229(to)X
2311(create.)X
555 2032(.MAKEFLAGS)N
1104(This)X
1267(target)X
1471(provides)X
1768(a)X
1825(way)X
1980(for)X
2095(you)X
2236(to)X
2319(always)X
2563(specify)X
2816(\257ags)X
2988(for)X
3103(PMake)X
3350(when)X
3544(the)X
3662(make\256le)X
3958(is)X
1104 2128(used.)N
1301(The)X
1456(\257ags)X
1637(are)X
1766(just)X
1911(as)X
2008(they)X
2176(would)X
2406(be)X
2512(typed)X
2720(to)X
2812(the)X
2939(shell)X
3119(\(except)X
3385(you)X
3534(can't)X
3724(use)X
3860(shell)X
1104 2224(variables)N
1414(unless)X
1634(they're)X
1882(in)X
1964(the)X
2082(environment\),)X
2554(though)X
2796(the)X
3 f
9 f
2914(-)X
3 f
2958(f)X
1 f
3005(and)X
3 f
9 f
3141(-)X
3 f
3185(r)X
1 f
3241(\257ags)X
3412(have)X
3584(no)X
3684(effect.)X
555 2348(.NULL)N
1104(This)X
1267(allows)X
1497(you)X
1638(to)X
1721(specify)X
1974(what)X
2151(suf\256x)X
2354(PMake)X
2602(should)X
2836(pretend)X
3098(a)X
3155(\256le)X
3278(has)X
3406(if,)X
3496(in)X
3579(fact,)X
3740(it)X
3804(has)X
3931(no)X
1104 2444(known)N
1349(suf\256x.)X
1578(Only)X
1765(one)X
1908(suf\256x)X
2117(may)X
2282(be)X
2385(so)X
2483(designated.)X
2873(The)X
3024(last)X
3161(source)X
3397(on)X
3503(the)X
3627(dependency)X
1104 2540(line)N
1244(is)X
1317(the)X
1435(suf\256x)X
1637(that)X
1777(is)X
1850(used)X
2017(\(you)X
2184(should,)X
2437(however,)X
2754(only)X
2916(give)X
3074(one)X
3210(suf\256x.)X
3425(.)X
3458(.\).)X
555 2664(.PATH)N
1104(If)X
1181(you)X
1324(give)X
1485(sources)X
1749(for)X
1866(this)X
2004(target,)X
2230(PMake)X
2480(will)X
2627(take)X
2784(them)X
2967(as)X
3057(directories)X
3419(in)X
3503(which)X
3721(to)X
3805(search)X
1104 2760(for)N
1221(\256les)X
1377(it)X
1444(cannot)X
1681(\256nd)X
1828(in)X
1913(the)X
2034(current)X
2285(directory.)X
2617(If)X
2693(you)X
2835(give)X
2995(no)X
3097(sources,)X
3380(it)X
3446(will)X
3592(clear)X
3771(out)X
3895(any)X
1104 2856(directories)N
1467(added)X
1683(to)X
1769(the)X
1891(search)X
2121(path)X
2283(before.)X
2533(Since)X
2735(the)X
2857(effects)X
3096(of)X
3187(this)X
3325(all)X
3428(get)X
3549(very)X
3715(complex,)X
1104 2952(I'll)N
1222(leave)X
1412(it)X
1476(til)X
1562(chapter)X
1819(four)X
1973(to)X
2055(give)X
2213(you)X
2353(a)X
2409(complete)X
2723(explanation.)X
555 3076(.PATH)N
2 f
784(suf\256x)X
1 f
1104(This)X
1270(does)X
1441(a)X
1501(similar)X
1747(thing)X
1935(to)X
7 f
2021(.PATH)X
1 f
(,)S
2305(but)X
2431(it)X
2499(does)X
2669(it)X
2736(only)X
2901(for)X
3018(\256les)X
3174(with)X
3339(the)X
3460(given)X
3661(suf\256x.)X
3886(The)X
1104 3172(suf\256x)N
1308(must)X
1485(have)X
1658(been)X
1831(de\256ned)X
2088(already.)X
2366(Look)X
2556(at)X
3 f
2635(Search)X
2892(Paths)X
1 f
3104(\(section)X
3379(4.1\))X
3527(for)X
3642(more)X
3828(infor-)X
1104 3268(mation.)N
555 3392(.PRECIOUS)N
1104(Similar)X
1367(to)X
7 f
1457(.IGNORE)X
1 f
(,)S
1841(this)X
1984(gives)X
2181(the)X
7 f
2306(.PRECIOUS)X
1 f
2765(attribute)X
3059(to)X
3148(each)X
3323(source)X
3560(on)X
3667(the)X
3792(depen-)X
1104 3488(dency)N
1321(line,)X
1486(unless)X
1711(there)X
1897(are)X
2021(no)X
2126(sources,)X
2412(in)X
2499(which)X
2720(case)X
2884(the)X
7 f
3007(.PRECIOUS)X
1 f
3464(attribute)X
3756(is)X
3833(given)X
1104 3584(to)N
1186(every)X
1385(target)X
1588(in)X
1670(the)X
1788(\256le.)X
555 3708(.RECURSIVE)N
1104(This)X
1269(target)X
1475(applies)X
1725(the)X
7 f
1846(.MAKE)X
1 f
2108(attribute)X
2397(to)X
2481(all)X
2583(its)X
2680(sources.)X
2963(It)X
3034(does)X
3203(nothing)X
3469(if)X
3540(you)X
3682(don't)X
3873(give)X
1104 3804(it)N
1168(any)X
1304(sources.)X
555 3928(.SHELL)N
1104(PMake)X
1353(is)X
1428(not)X
1552(constrained)X
1943(to)X
2026(only)X
2189(using)X
2383(the)X
2502(Bourne)X
2759(shell)X
2931(to)X
3014(execute)X
3281(the)X
3400(commands)X
3768(you)X
3909(put)X
1104 4024(in)N
1189(the)X
1310(make\256le.)X
1629(You)X
1790(can)X
1925(tell)X
2050(it)X
2117(some)X
2309(other)X
2497(shell)X
2671(to)X
2756(use)X
2886(with)X
3050(this)X
3187(target.)X
3412(Check)X
3639(out)X
3 f
3763(A)X
3843(Shell)X
1104 4120(is)N
1177(a)X
1237(Shell)X
1425(is)X
1498(a)X
1558(Shell)X
1 f
1746(\(section)X
2020(4.4\))X
2167(for)X
2281(more)X
2466(information.)X
555 4244(.SILENT)N
1104(When)X
1331(you)X
1486(use)X
7 f
1628(.SILENT)X
1 f
1999(as)X
2101(a)X
2172(target,)X
2410(it)X
2489(applies)X
2751(the)X
7 f
2884(.SILENT)X
1 f
3255(attribute)X
3557(to)X
3653(each)X
3835(of)X
3936(its)X
1104 4340(sources.)N
1386(If)X
1460(there)X
1641(are)X
1760(no)X
1860(sources)X
2121(on)X
2221(the)X
2339(dependency)X
2743(line,)X
2903(then)X
3061(it)X
3125(is)X
3198(as)X
3285(if)X
3354(you)X
3494(gave)X
3666(PMake)X
3913(the)X
3 f
9 f
1104 4436(-)N
3 f
1148(s)X
1 f
1199(\257ag)X
1339(and)X
1475(no)X
1575(commands)X
1942(will)X
2086(be)X
2182(echoed.)X
555 4560(.SUFFIXES)N
1104(This)X
1270(is)X
1347(used)X
1518(to)X
1603(give)X
1764(new)X
1921(\256le)X
2046(suf\256xes)X
2318(for)X
2435(PMake)X
2685(to)X
2770(handle.)X
3027(Each)X
3211(source)X
3444(is)X
3520(a)X
3579(suf\256x)X
3784(PMake)X
1104 4656(should)N
1348(recognize.)X
1712(If)X
1797(you)X
1948(give)X
2117(a)X
7 f
2184(.SUFFIXES)X
1 f
2647(dependency)X
3061(line)X
3211(with)X
3383(no)X
3493(sources,)X
3784(PMake)X
1104 4752(will)N
1250(forget)X
1464(about)X
1664(all)X
1766(the)X
1886(suf\256xes)X
2157(it)X
2223(knew)X
2419(\(this)X
2583(also)X
2733(nukes)X
2941(the)X
3060(null)X
3205(suf\256x\).)X
3475(For)X
3607(those)X
3797(targets)X
1104 4848(that)N
1244(need)X
1416(to)X
1498(have)X
1670(suf\256xes)X
1939(de\256ned,)X
2215(this)X
2350(is)X
2423(how)X
2581(you)X
2721(do)X
2821(it.)X
755 4972(In)N
842(addition)X
1124(to)X
1206(these)X
1391(targets,)X
1645(a)X
1701(line)X
1841(of)X
1928(the)X
2046(form)X
2 f
843 5116(attribute)N
7 f
1166(:)X
2 f
1262(sources)X
1 f
555 5260(applies)N
802(the)X
2 f
920(attribute)X
1 f
1228(to)X
1310(all)X
1410(the)X
1528(targets)X
1762(listed)X
1955(as)X
2 f
2042(sources)X
1 f
2300(.)X
3 f
555 5452(3.6.)N
715(Modifying)X
1090(Variable)X
1408(Expansion)X
1 f
755 5576(Variables)N
1099(need)X
1287(not)X
1425(always)X
1684(be)X
1796(expanded)X
2140(verbatim.)X
2481(PMake)X
2744(de\256nes)X
3007(several)X
3270(modi\256ers)X
3607(that)X
3762(may)X
3935(be)X
555 5672(applied)N
820(to)X
911(a)X
976(variable's)X
1322(value)X
1525(before)X
1760(it)X
1833(is)X
1915(expanded.)X
2271(You)X
2437(apply)X
2643(a)X
2707(modi\256er)X
3006(by)X
3114(placing)X
3378(it)X
3450(after)X
3626(the)X
3752(variable)X
555 5768(name)N
749(with)X
911(a)X
967(colon)X
1165(between)X
1453(the)X
1571(two,)X
1731(like)X
1871(so:)X

23 p
%%Page: 23 24
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(23)X
2343(-)X
7 f
843 720(${)N
2 f
(VARIABLE)S
7 f
1304(:)X
2 f
(modi\256er)S
7 f
1619(})X
1 f
555 864(Each)N
737(modi\256er)X
1029(is)X
1103(a)X
1160(single)X
1372(character)X
1689(followed)X
1995(by)X
2096(something)X
2450(speci\256c)X
2716(to)X
2799(the)X
2918(modi\256er)X
3210(itself.)X
3430(You)X
3588(may)X
3746(apply)X
3944(as)X
555 960(many)N
758(modi\256ers)X
1085(as)X
1177(you)X
1322(want)X
1503(\320)X
1608(each)X
1781(one)X
1922(is)X
2000(applied)X
2261(to)X
2348(the)X
2471(result)X
2674(of)X
2766(the)X
2888(previous)X
3188(and)X
3328(is)X
3405(separated)X
3733(from)X
3913(the)X
555 1056(previous)N
851(by)X
951(another)X
1212(colon.)X
755 1180(There)N
969(are)X
1094(seven)X
1303(ways)X
1493(to)X
1580(modify)X
1836(a)X
1897(variable's)X
2239(expansion,)X
2609(most)X
2789(of)X
2881(which)X
3102(come)X
3301(from)X
3482(the)X
3605(C)X
3683(shell)X
3859(vari-)X
555 1276(able)N
709(modi\256cation)X
1133(characters:)X
755 1400(M)N
2 f
826(pattern)X
1 f
955 1496(This)N
1121(is)X
1198(used)X
1368(to)X
1453(select)X
1659(only)X
1824(those)X
2016(words)X
2235(\(a)X
2321(word)X
2509(is)X
2585(a)X
2644(series)X
2850(of)X
2940(characters)X
3290(that)X
3433(are)X
3555(neither)X
3801(spaces)X
955 1592(nor)N
1087(tabs\))X
1268(that)X
1413(match)X
1633(the)X
1755(given)X
2 f
1957(pattern)X
1 f
2201(.)X
2265(The)X
2414(pattern)X
2661(is)X
2738(a)X
2798(wildcard)X
3103(pattern)X
3350(like)X
3494(that)X
3638(used)X
3809(by)X
3913(the)X
955 1688(shell,)N
1156(where)X
7 f
1383(*)X
1 f
1461(means)X
1696(0)X
1766(or)X
1863(more)X
2058(characters)X
2415(of)X
2512(any)X
2658(sort;)X
7 f
2830(?)X
1 f
2928(is)X
3011(any)X
3156(single)X
3376(character;)X
7 f
3723([abcd])X
1 f
955 1784(matches)N
1239(any)X
1376(single)X
1588(character)X
1904(that)X
2044(is)X
2117(either)X
2320(`a',)X
2450(`b',)X
2584(`c')X
2694(or)X
2781(`d')X
2895(\(there)X
3103(may)X
3261(be)X
3357(any)X
3493(number)X
3758(of)X
3845(char-)X
955 1880(acters)N
1167(between)X
1459(the)X
1581(brackets\);)X
7 f
1922([0-9])X
1 f
2186(matches)X
2473(any)X
2613(single)X
2828(character)X
3148(that)X
3292(is)X
3369(between)X
3661(`0')X
3778(and)X
3917(`9')X
955 1976(\(i.e.)N
1105(any)X
1246(digit.)X
1437(This)X
1604(form)X
1785(may)X
1948(be)X
2049(freely)X
2262(mixed)X
2487(with)X
2654(the)X
2777(other)X
2967(bracket)X
3229(form\),)X
3457(and)X
3598(`\\')X
3699(is)X
3777(used)X
3949(to)X
955 2072(escape)N
1201(any)X
1348(of)X
1446(the)X
1575(characters)X
1933(`*',)X
2078(`?',)X
2219(`[')X
2331(or)X
2429(`:',)X
2555(leaving)X
2821(them)X
3011(as)X
3108(regular)X
3366(characters)X
3723(to)X
3815(match)X
955 2168(themselves)N
1365(in)X
1481(a)X
1571(word.)X
1830(For)X
1995(example,)X
2341(the)X
2493(system)X
2769(make\256le)X
7 f
3099(<makedepend.mk>)X
1 f
3873(uses)X
955 2264(``)N
7 f
1009($\(CFLAGS:M-[ID]*\))X
1 f
('')S
1906(to)X
1995(extract)X
2241(all)X
2348(the)X
7 f
9 f
2473(-)X
7 f
2517(I)X
1 f
2592(and)X
7 f
9 f
2735(-)X
7 f
2779(D)X
1 f
2854(\257ags)X
3032(that)X
3178(would)X
3404(be)X
3506(passed)X
3746(to)X
3834(the)X
3958(C)X
955 2360(compiler.)N
1280(This)X
1442(allows)X
1671(it)X
1735(to)X
1817(properly)X
2109(locate)X
2321(include)X
2577(\256les)X
2730(and)X
2866(generate)X
3159(the)X
3277(correct)X
3521(dependencies.)X
755 2484(N)N
2 f
813(pattern)X
1 f
955 2580(This)N
1117(is)X
1190(identical)X
1486(to)X
7 f
1568(:M)X
1 f
1684(except)X
1914(it)X
1978(substitutes)X
2335(all)X
2435(words)X
2651(that)X
2791(don't)X
2980(match)X
3196(the)X
3314(given)X
3512(pattern.)X
755 2704(S/)N
2 f
821(search-string)X
1 f
1248(/)X
2 f
1270(replacement-string)X
1 f
1880(/[g])X
955 2800(Causes)N
1214(the)X
1344(\256rst)X
1500 0.3611(occurrence)AX
1886(of)X
2 f
1985(search-string)X
1 f
2457(in)X
2551(the)X
2681(variable)X
2972(to)X
3065(be)X
3172(replaced)X
3476(by)X
2 f
3587(replacement-)X
955 2896(string)N
1 f
1154(,)X
1206(unless)X
1438(the)X
7 f
1568(g)X
1 f
1648(\257ag)X
1800(is)X
1885(given)X
2095(at)X
2185(the)X
2315(end,)X
2482(in)X
2575(which)X
2802(case)X
2972(all)X
3083(occurences)X
3472(of)X
3570(the)X
3699(string)X
3912(are)X
955 2992(replaced.)N
1274(The)X
1425(substitution)X
1823(is)X
1902(performed)X
2263(on)X
2369(each)X
2543(word)X
2734(in)X
2822(the)X
2946(variable)X
3231(in)X
3318(turn.)X
3492(If)X
2 f
3571(search-string)X
1 f
955 3088(begins)N
1190(with)X
1358(a)X
7 f
1420(\303)X
1 f
(,)S
1514(the)X
1638(string)X
1846(must)X
2027(match)X
2249(starting)X
2515(at)X
2599(the)X
2722(beginning)X
3067(of)X
3159(the)X
3282(word.)X
3492(If)X
2 f
3571(search-string)X
1 f
955 3184(ends)N
1130(with)X
1300(a)X
7 f
1364($)X
1 f
(,)S
1460(the)X
1586(string)X
1796(must)X
1979(match)X
2203(to)X
2293(the)X
2418(end)X
2561(of)X
2655(the)X
2780(word)X
2972(\(these)X
3191(two)X
3338(may)X
3503(be)X
3606(combined)X
3949(to)X
955 3280(force)N
1148(an)X
1251(exact)X
1448(match\).)X
1718(If)X
1799(a)X
1862(backslash)X
2201(preceeds)X
2510(these)X
2702(two)X
2849(characters,)X
3222(however,)X
3545(they)X
3709(lose)X
3864(their)X
955 3376(special)N
1202(meaning.)X
1522(Variable)X
1822(expansion)X
2170(also)X
2322(occurs)X
2555(in)X
2640(the)X
2761(normal)X
3011(fashion)X
3270(inside)X
3484(both)X
3649(the)X
2 f
3770(search-)X
955 3472(string)N
1 f
1177(and)X
1316(the)X
2 f
1437(replacement-string)X
1 f
2060(,)X
3 f
2103(except)X
1 f
2345(that)X
2488(a)X
2547(backslash)X
2882(is)X
2958(used)X
3128(to)X
3213(prevent)X
3477(the)X
3597(expansion)X
3944(of)X
955 3568(a)N
7 f
1016($)X
1 f
(,)S
1109(not)X
1236(another)X
1502(dollar)X
1714(sign,)X
1891(as)X
1982(is)X
2059(usual.)X
2292(Note)X
2472(that)X
2 f
2616(search-string)X
1 f
3080(is)X
3157(just)X
3296(a)X
3356(string,)X
3582(not)X
3708(a)X
3768(pattern,)X
955 3664(so)N
1052(none)X
1234(of)X
1327(the)X
1451(usual)X
1646 0.1394(regular-expression/wildcard)AX
2573(characters)X
2926(have)X
3104(any)X
3246(special)X
3494(meaning)X
3795(save)X
7 f
3963(\303)X
1 f
955 3760(and)N
7 f
1097($)X
1 f
(.)S
1211(In)X
1304(the)X
1428(replacement)X
1847(string,)X
2075(the)X
7 f
2199(&)X
1 f
2273(character)X
2595(is)X
2673(replaced)X
2971(by)X
3076(the)X
2 f
3199(search-string)X
1 f
3664(unless)X
3889(it)X
3958(is)X
955 3856(preceded)N
1273(by)X
1380(a)X
1443(backslash.)X
1822(You)X
1987(are)X
2113(allowed)X
2394(to)X
2483(use)X
2617(any)X
2760(character)X
3083(except)X
3320(colon)X
3525(or)X
3619(exclamation)X
955 3952(point)N
1150(to)X
1243(separate)X
1538(the)X
1667(two)X
1818(strings.)X
2082(This)X
2255(so-called)X
2576(delimiter)X
2896(character)X
3222(may)X
3390(be)X
3496(placed)X
3736(in)X
3828(either)X
955 4048(string)N
1157(by)X
1257(preceeding)X
1630(it)X
1694(with)X
1856(a)X
1912(backslash.)X
755 4172(T)N
955(Replaces)X
1269(each)X
1441(word)X
1630(in)X
1715(the)X
1836(variable)X
2118(expansion)X
2466(by)X
2569(its)X
2667(last)X
2801(component)X
3180(\(its)X
3305(``tail''\).)X
3585(For)X
3719(example,)X
955 4268(given)N
7 f
1243 4412(OBJS)N
1483(=)X
1579(../lib/a.o)X
2107(b)X
2203(/usr/lib/libm.a)X
1243 4508(TAILS)N
1531(=)X
1627($\(OBJS:T\))X
1 f
955 4652(the)N
1073(variable)X
7 f
1352(TAILS)X
1 f
1612(would)X
1832(expand)X
2084(to)X
2166(``)X
7 f
2220(a.o)X
2412(b)X
2508(libm.a)X
1 f
(.'')S
755 4776(H)N
955(This)X
1136(is)X
1228(similar)X
1489(to)X
7 f
1589(:T)X
1 f
(,)S
1743(except)X
1991(that)X
2149(every)X
2366(word)X
2569(is)X
2660(replaced)X
2971(by)X
3089(everything)X
3470(but)X
3610(the)X
3746(tail)X
3886(\(the)X
955 4872(``head''\).)N
1295(Using)X
1519(the)X
1650(same)X
1848(de\256nition)X
2187(of)X
7 f
2287(OBJS)X
1 f
(,)S
2532(the)X
2663(string)X
2878(``)X
7 f
2932($\(OBJS:H\))X
1 f
('')S
3451(would)X
3684(expand)X
3949(to)X
955 4968(``)N
7 f
1009(../lib)X
1361(/usr/lib)X
1 f
(.'')S
1855(Note)X
2047(that)X
2203(the)X
2337(\256nal)X
2515(slash)X
2710(on)X
2825(the)X
2958(heads)X
3176(is)X
3264(removed)X
3580(and)X
3731(anything)X
955 5064(without)N
1219(a)X
1275(head)X
1447(is)X
1520(replaced)X
1813(by)X
1913(the)X
2031(empty)X
2251(string.)X
755 5188(E)N
7 f
955(:E)X
1 f
1076(replaces)X
1365(each)X
1538(word)X
1728(by)X
1833(its)X
1933(suf\256x)X
2140(\(``extension''\).)X
2654(So)X
2763(``)X
7 f
2817($\(OBJS:E\))X
1 f
('')S
3328(would)X
3553(give)X
3716(you)X
3861(``)X
7 f
3915(.o)X
955 5284(.a)N
1 f
(.'')S
755 5408(R)N
955(This)X
1158(replaces)X
1483(each)X
1692(word)X
1918(by)X
2059(everything)X
2463(but)X
2626(the)X
2785(suf\256x)X
3028(\(the)X
3214(``root'')X
3512(of)X
3640(the)X
3799(word\).)X
955 5504(``)N
7 f
1009($\(OBJS:R\))X
1 f
('')S
1515(expands)X
1798(to)X
1880(``)X
7 f
1954(../lib/a)X
2386(b)X
2482(/usr/lib/libm)X
1 f
(.'')S
755 5628(In)N
842(addition,)X
1144(the)X
1262(System)X
1517(V)X
1595(style)X
1766(of)X
1853(substitution)X
2245(is)X
2318(also)X
2467(supported.)X
2843(This)X
3005(looks)X
3198(like:)X

24 p
%%Page: 24 25
10 s 10 xH 0 xS 1 f
7 f
1 f
2196 384(-)N
2243(24)X
2343(-)X
7 f
843 720($\()N
2 f
(VARIABLE)S
7 f
1304(:)X
2 f
(search-string)S
7 f
1779(=)X
2 f
(replacement)S
7 f
2224(\))X
1 f
555 864(It)N
625(must)X
801(be)X
898(the)X
1017(last)X
1149(modi\256er)X
1441(in)X
1524(the)X
1643(chain.)X
1858(The)X
2004(search)X
2231(is)X
2305(anchored)X
2621(at)X
2700(the)X
2819(end)X
2956(of)X
3044(each)X
3213(word,)X
3419(so)X
3511(only)X
3674(suf\256xes)X
3944(or)X
555 960(whole)N
771(words)X
987(may)X
1145(be)X
1241(replaced.)X
3 f
555 1152(3.7.)N
715(More)X
923(on)X
1027(Debugging)X
555 1344(3.8.)N
715(More)X
923(Exercises)X
1 f
555 1468(\(3.1\))N
755(You've)X
1028(got)X
1162(a)X
1230(set)X
1351(programs,)X
1706(each)X
1886(of)X
1985(which)X
2212(is)X
2296(created)X
2560(from)X
2747(its)X
2853(own)X
3022(assembly-language)X
3668(source)X
3909(\256le)X
755 1564(\(suf\256x)N
7 f
997(.asm)X
1 f
(\).)S
1289(Each)X
1482(program)X
1786(can)X
1930(be)X
2038(assembled)X
2404(into)X
2560(two)X
2712(versions,)X
3031(one)X
3179(with)X
3353 0.2500(error-checking)AX
3859(code)X
755 1660(assembled)N
1118(in)X
1209(and)X
1354(one)X
1499(without.)X
1792(You)X
1959(could)X
2166(assemble)X
2489(them)X
2678(into)X
2831(\256les)X
2992(with)X
3162(different)X
3467(suf\256xes)X
3744(\()X
7 f
3771(.eobj)X
1 f
755 1756(and)N
7 f
898(.obj)X
1 f
(,)S
1137(for)X
1258(instance\),)X
1594(but)X
1722(your)X
1895(linker)X
2108(only)X
2276(understands)X
2685(\256les)X
2844(that)X
2990(end)X
3132(in)X
7 f
3220(.obj)X
1 f
(.)S
3478(To)X
3593(top)X
3721(it)X
3791(all)X
3897(off,)X
755 1852(the)N
874(\256nal)X
1037(executables)X
2 f
1433(must)X
1 f
1618(have)X
1791(the)X
1910(suf\256x)X
7 f
2113(.exe)X
1 f
(.)S
2366(How)X
2543(can)X
2676(you)X
2817(still)X
2957(use)X
3085(transformation)X
3578(rules)X
3755(to)X
3837(make)X
755 1948(your)N
922(life)X
1049(easier)X
1257(\(Hint:)X
1468(assume)X
1724(the)X
1842 0.2500(error-checking)AX
2336(versions)X
2623(have)X
7 f
2795(ec)X
1 f
2911(tacked)X
3141(onto)X
3303(their)X
3470(pre\256x\)?)X
555 2072(\(3.2\))N
755(Assume,)X
1055(for)X
1171(a)X
1229(moment)X
1513(or)X
1602(two,)X
1764(you)X
1906(want)X
2084(to)X
2168(perform)X
2449(a)X
2507(sort)X
2649(of)X
2738(``indirection'')X
3215(by)X
3316(placing)X
3573(the)X
3692(name)X
3887(of)X
3975(a)X
755 2168(variable)N
1051(into)X
1212(another)X
1490(one,)X
1663(then)X
1838(you)X
1995(want)X
2188(to)X
2287(get)X
2422(the)X
2557(value)X
2768(of)X
2872(the)X
3007(\256rst)X
3168(by)X
3284(expanding)X
3654(the)X
3788(second)X
755 2264(somehow.)N
1102(Unfortunately,)X
1592(PMake)X
1839(doesn't)X
2095(allow)X
2293(constructs)X
2638(like)X
7 f
1043 2408($\($\(FOO\)\))N
1 f
755 2552(What)N
951(do)X
1053(you)X
1195(do?)X
1333(Hint:)X
1519(no)X
1621(further)X
1862(variable)X
2143(expansion)X
2489(is)X
2563(performed)X
2919(after)X
3088(modi\256ers)X
3411(are)X
3531(applied,)X
3808(thus)X
3962(if)X
755 2648(you)N
895(cause)X
1094(a)X
1150($)X
1210(to)X
1292(occur)X
1491(in)X
1573(the)X
1691(expansion,)X
2056(that's)X
2254(what)X
2430(will)X
2574(be)X
2670(in)X
2752(the)X
2870(result.)X
3 f
555 2840(4.)N
655(PMake)X
920(for)X
1043(Gods)X
1 f
755 2964(This)N
924(chapter)X
1188(is)X
1268(devoted)X
1549(to)X
1638(those)X
1834(facilities)X
2137(in)X
2226(PMake)X
2480(that)X
2627(allow)X
2832(you)X
2979(to)X
3068(do)X
3175(a)X
3238(great)X
3425(deal)X
3585(in)X
3673(a)X
3735(make\256le)X
555 3060(with)N
719(very)X
884(little)X
1052(work,)X
1259(as)X
1348(well)X
1508(as)X
1597(do)X
1699(some)X
1890(things)X
2107(you)X
2249(couldn't)X
2538(do)X
2640(in)X
2724(Make)X
2929(without)X
3195(a)X
3253(great)X
3436(deal)X
3592(of)X
3681(work)X
3868(\(and)X
555 3156(perhaps)N
828(the)X
949(use)X
1079(of)X
1169(other)X
1357(programs\).)X
1730(The)X
1878(problem)X
2168(with)X
2333(these)X
2520(features,)X
2817(is)X
2892(they)X
3052(must)X
3229(be)X
3327(handled)X
3603(with)X
3767(care,)X
3944(or)X
555 3252(you)N
695(will)X
839(end)X
975(up)X
1075(with)X
1237(a)X
1293(mess.)X
755 3376(Once)N
963(more,)X
1186(I)X
1251(assume)X
1525(a)X
1599(greater)X
1861(familiarity)X
2237(with)X
9 s
2415(UNIX)X
10 s
2633(or)X
2738(Sprite)X
2966(than)X
3141(I)X
3205(did)X
3344(in)X
3443(the)X
3578(previous)X
3891(two)X
555 3472(chapters.)N
3 f
555 3664(4.1.)N
715(Search)X
971(Paths)X
1 f
755 3788(PMake)N
1010(supports)X
1309(the)X
1435(dispersal)X
1748(of)X
1842(\256les)X
2002(into)X
2153(multiple)X
2446(directories)X
2812(by)X
2919(allowing)X
3226(you)X
3373(to)X
3462(specify)X
3721(places)X
3949(to)X
555 3884(look)N
722(for)X
841(sources)X
1107(with)X
7 f
1274(.PATH)X
1 f
1539(targets)X
1778(in)X
1865(the)X
1988(make\256le.)X
2309(The)X
2459(directories)X
2823(you)X
2968(give)X
3131(as)X
3223(sources)X
3489(for)X
3608(these)X
3797(targets)X
555 3980(make)N
750(up)X
851(a)X
908(``search)X
1189(path.'')X
1422(Only)X
1603(those)X
1792(\256les)X
1945(used)X
2112(exclusively)X
2497(as)X
2584(sources)X
2845(are)X
2964(actually)X
3238(sought)X
3471(on)X
3571(a)X
3627(search)X
3853(path,)X
555 4076(the)N
674(assumption)X
1059(being)X
1258(that)X
1399(anything)X
1700(listed)X
1894(as)X
1982(a)X
2039(target)X
2243(in)X
2326(the)X
2445(make\256le)X
2742(can)X
2875(be)X
2972(created)X
3226(by)X
3327(the)X
3446(make\256le)X
3742(and)X
3878(thus)X
555 4172(should)N
788(be)X
884(in)X
966(the)X
1084(current)X
1332(directory.)X
755 4296(There)N
969(are)X
1094(two)X
1240(types)X
1435(of)X
1528(search)X
1760(paths)X
1955(in)X
2043(PMake:)X
2318(one)X
2460(is)X
2539(used)X
2712(for)X
2832(all)X
2937(types)X
3131(of)X
3223(\256les)X
3381(\(including)X
3735(included)X
555 4392(make\256les\))N
911(and)X
1049(is)X
1124(speci\256ed)X
1431(with)X
1595(a)X
1652(plain)X
7 f
1833(.PATH)X
1 f
2094(target)X
2298(\(e.g.)X
2482(``)X
7 f
2536(.PATH)X
2825(:)X
2922(RCS)X
1 f
(''\),)S
3188(while)X
3387(the)X
3506(other)X
3692(is)X
3766(speci\256c)X
555 4488(to)N
640(a)X
699(certain)X
941(type)X
1102(of)X
1192(\256le,)X
1337(as)X
1427(indicated)X
1744(by)X
1847(the)X
1968(\256le's)X
2151(suf\256x.)X
2376(A)X
2457(speci\256c)X
2725(search)X
2954(path)X
3115(is)X
3191(indicated)X
3508(by)X
3611(immediately)X
555 4584(following)N
886(the)X
7 f
1004(.PATH)X
1 f
1264(with)X
1426(the)X
1544(suf\256x)X
1746(of)X
1833(the)X
1951(\256le.)X
2093(For)X
2224(instance)X
7 f
843 4728(.PATH.h)N
1611(:)X
1707(/sprite/lib/include)X
2667(/sprite/att/lib/include)X
1 f
555 4872(would)N
888(tell)X
1123(PMake)X
1483(to)X
1678(look)X
1953(in)X
2148(the)X
2379(directories)X
7 f
2851(/sprite/lib/include)X
1 f
3895(and)X
7 f
555 4968(/sprite/att/lib/include)N
1 f
1679(for)X
1793(any)X
1929(\256les)X
2082(whose)X
2307(suf\256x)X
2509(is)X
7 f
2582(.h)X
1 f
(.)S
755 5092(The)N
905(current)X
1158(directory)X
1473(is)X
1551(always)X
1799(consulted)X
2130(\256rst)X
2278(to)X
2364(see)X
2491(if)X
2564(a)X
2624(\256le)X
2750(exists.)X
2976(Only)X
3160(if)X
3233(it)X
3301(cannot)X
3539(be)X
3639(found)X
3850(there)X
555 5188(are)N
674(the)X
792(directories)X
1151(in)X
1233(the)X
1351(speci\256c)X
1616(search)X
1842(path,)X
2020(followed)X
2325(by)X
2425(those)X
2614(in)X
2696(the)X
2814(general)X
3071(search)X
3297(path,)X
3475(consulted.)X
755 5312(A)N
844(search)X
1081(path)X
1250(is)X
1334(also)X
1494(used)X
1672(when)X
1877(expanding)X
2242(wildcard)X
2554(characters.)X
2932(If)X
3016(the)X
3144(pattern)X
3397(has)X
3534(a)X
3600(recognizable)X
555 5408(suf\256x)N
767(on)X
877(it,)X
971(the)X
1099(path)X
1267(for)X
1391(that)X
1541(suf\256x)X
1753(will)X
1907(be)X
2013(used)X
2190(for)X
2314(the)X
2442(expansion.)X
2817(Otherwise)X
3177(the)X
3304(default)X
3556(search)X
3791(path)X
3958(is)X
555 5504(employed.)N
755 5628(When)N
970(a)X
1029(\256le)X
1154(is)X
1230(found)X
1440(in)X
1525(some)X
1716(directory)X
2028(other)X
2215(than)X
2375(the)X
2495(current)X
2745(one,)X
2903(all)X
3005(local)X
3183(variables)X
3495(that)X
3637(would)X
3859(have)X
555 5724(contained)N
893(the)X
1017(target's)X
1284(name)X
1484(\()X
7 f
1511(.ALLSRC)X
1 f
(,)S
1893(and)X
7 f
2035(.IMPSRC)X
1 f
(\))S
2424(will)X
2574(instead)X
2827(contain)X
3089(the)X
3212(path)X
3375(to)X
3462(the)X
3585(\256le,)X
3732(as)X
3824(found)X
555 5820(by)N
655(PMake.)X
942(Thus)X
1122(if)X
1191(you)X
1331(have)X
1503(a)X
1559(\256le)X
7 f
1681(../lib/mumble.c)X
1 f
2421(and)X
2557(a)X
2613(make\256le)X

25 p
%%Page: 25 26
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(25)X
2343(-)X
7 f
843 720(.PATH.c)N
1611(:)X
1707(../lib)X
843 816(mumble)N
1611(:)X
1707(mumble.c)X
1227 912($\(CC\))N
1515(-o)X
1659($\(.TARGET\))X
2187($\(.ALLSRC\))X
1 f
555 1056(the)N
679(command)X
1021(executed)X
1333(to)X
1421(create)X
7 f
1640(mumble)X
1 f
1954(would)X
2180(be)X
2282(``)X
7 f
2336(cc)X
2485(-o)X
2634(mumble)X
2975(../lib/mumble.c)X
1 f
(.'')S
3794(\(As)X
3935(an)X
555 1152(aside,)N
764(the)X
886(command)X
1226(in)X
1312(this)X
1451(case)X
1614(isn't)X
1780(strictly)X
2025(necessary,)X
2381(since)X
2569(it)X
2636(will)X
2783(be)X
2882(found)X
3092(using)X
3288(transformation)X
3783(rules)X
3962(if)X
555 1248(it)N
628(isn't)X
799(given.)X
1026(This)X
1197(is)X
1279(because)X
7 f
1563(.out)X
1 f
1784(is)X
1866(the)X
1993(null)X
2146(suf\256x)X
2357(by)X
2466(default)X
2718(and)X
2863(a)X
2928(transformation)X
3429(exists)X
3640(from)X
7 f
3825(.c)X
1 f
3949(to)X
7 f
555 1344(.out)N
1 f
(.)S
807(Just)X
951(thought)X
1215(I'd)X
1329(throw)X
1536(that)X
1676(in.\))X
755 1468(If)N
830(a)X
887(\256le)X
1010(exists)X
1213(in)X
1296(two)X
1437(directories)X
1797(on)X
1898(the)X
2017(same)X
2203(search)X
2430(path,)X
2609(the)X
2728(\256le)X
2851(in)X
2934(the)X
3053(\256rst)X
3198(directory)X
3509(on)X
3610(the)X
3729(path)X
3887(will)X
555 1564(be)N
654(the)X
775(one)X
914(PMake)X
1164(uses.)X
1345(So)X
1452(if)X
1524(you)X
1667(have)X
1842(a)X
1901(large)X
2085(system)X
2330(spread)X
2563(over)X
2728(many)X
2928(directories,)X
3309(it)X
3375(would)X
3597(behoove)X
3891(you)X
555 1660(to)N
637(follow)X
866(a)X
922(naming)X
1182(convention)X
1558(that)X
1698(avoids)X
1927(such)X
2094(con\257icts.)X
755 1784(Something)N
1130(you)X
1279(should)X
1521(know)X
1728(about)X
1935(the)X
2062(way)X
2225(search)X
2460(paths)X
2658(are)X
2786(implemented)X
3233(is)X
3315(that)X
3464(each)X
3640(directory)X
3958(is)X
555 1880(read,)N
739(and)X
880(its)X
980(contents)X
1272(cached,)X
1541(exactly)X
1798(once)X
1975(\320)X
2080(when)X
2279(it)X
2348(is)X
2426(\256rst)X
2575(encountered)X
2993(\320)X
3097(so)X
3192(any)X
3332(changes)X
3615(to)X
3701(the)X
3823(direc-)X
555 1976(tories)N
754(while)X
953(PMake)X
1201(is)X
1275(running)X
1545(will)X
1690(not)X
1813(be)X
1910(noted)X
2109(when)X
2304(searching)X
2633(for)X
2748(implicit)X
3017(sources,)X
3299(nor)X
3426(will)X
3570(they)X
3728(be)X
3824(found)X
555 2072(when)N
751(PMake)X
1000(attempts)X
1293(to)X
1377(discover)X
1671(when)X
1867(the)X
1987(\256le)X
2111(was)X
2258(last)X
2391(modi\256ed,)X
2717(unless)X
2939(the)X
3058(\256le)X
3181(was)X
3327(created)X
3581(in)X
3664(the)X
3783(current)X
555 2168(directory.)N
892(While)X
1115(people)X
1356(have)X
1535(suggested)X
1878(that)X
2025(PMake)X
2279(should)X
2518(read)X
2683(the)X
2807(directories)X
3172(each)X
3346(time,)X
3534(my)X
3662(experience)X
555 2264(suggests)N
858(that)X
1010(the)X
1140(caching)X
1422(seldom)X
1685(causes)X
1927(problems.)X
2277(In)X
2376(addition,)X
2690(not)X
2823(caching)X
3104(the)X
3233(directories)X
3603(slows)X
3816(things)X
555 2360(down)N
756(enormously)X
1157(because)X
1435(of)X
1525(PMake's)X
1833(attempts)X
2127(to)X
2212(apply)X
2413(transformation)X
2908(rules)X
3087(through)X
3358(non-existent)X
3776(\256les)X
3931(\320)X
555 2456(the)N
682(number)X
956(of)X
1052(extra)X
1242(\256le-system)X
1622(searches)X
1924(is)X
2006(truly)X
2186(staggering,)X
2569(especially)X
2919(if)X
2996(many)X
3202(\256les)X
3363(without)X
3635(suf\256xes)X
3912(are)X
555 2552(used)N
722(and)X
858(the)X
976(null)X
1120(suf\256x)X
1322(isn't)X
1484(changed)X
1772(from)X
7 f
1948(.out)X
1 f
(.)S
3 f
555 2744(4.2.)N
715(Archives)X
1038(and)X
1186(Libraries)X
1 f
9 s
755 2868(UNIX)N
10 s
958(and)X
1097(Sprite)X
1311(allow)X
1511(you)X
1653(to)X
1737(merge)X
1960(\256les)X
2115(into)X
2261(an)X
2359(archive)X
2618(using)X
2813(the)X
7 f
2933(ar)X
1 f
3051(command.)X
3409(Further,)X
3687(if)X
3758(the)X
3878(\256les)X
555 2964(are)N
676(relocatable)X
1051(object)X
1269(\256les,)X
1444(you)X
1586(can)X
1720(run)X
7 f
1849(ranlib)X
1 f
2159(on)X
2261(the)X
2381(archive)X
2640(and)X
2777(get)X
2896(yourself)X
3180(a)X
3237(library)X
3472(that)X
3613(you)X
3754(can)X
3887(link)X
555 3060(into)N
701(any)X
839(program)X
1133(you)X
1275(want.)X
1473(The)X
1620(main)X
1802(problem)X
2091(with)X
2255(archives)X
2545(is)X
2620(they)X
2780(double)X
3020(the)X
3139(space)X
3339(you)X
3480(need)X
3653(to)X
3736(store)X
3913(the)X
555 3156(archived)N
856(\256les,)X
1033(since)X
1222(there's)X
1465(one)X
1605(copy)X
1785(in)X
1871(the)X
1993(archive)X
2254(and)X
2394(one)X
2534(copy)X
2714(out)X
2839(by)X
2942(itself.)X
3145(The)X
3293(problem)X
3583(with)X
3748(libraries)X
555 3252(is)N
631(you)X
774(usually)X
1028(think)X
1215(of)X
1305(them)X
1487(as)X
7 f
1576(-lm)X
1 f
1742(rather)X
1952(than)X
7 f
2112(/usr/lib/libm.a)X
1 f
2854(and)X
2992(the)X
3112(linker)X
3321(thinks)X
3538(they're)X
3788(out-of-)X
555 3348(date)N
709(if)X
778(you)X
918(so)X
1009(much)X
1207(as)X
1294(look)X
1456(at)X
1534(them.)X
755 3472(PMake)N
1003(solves)X
1224(the)X
1343(problem)X
1631(with)X
1794(archives)X
2083(by)X
2184(allowing)X
2485(you)X
2626(to)X
2709(tell)X
2832(it)X
2897(to)X
2980(examine)X
3272(the)X
3390(\256les)X
3543(in)X
3625(the)X
3743(archives)X
555 3568(\(so)N
682(you)X
831(can)X
972(remove)X
1242(the)X
1368(individual)X
1720(\256les)X
1881(without)X
2153(having)X
2399(to)X
2489(regenerate)X
2853(them)X
3041(later\).)X
3259(To)X
3376(handle)X
3618(the)X
3744(problem)X
555 3664(with)N
717(libraries,)X
1020(PMake)X
1267(adds)X
1434(an)X
1530(additional)X
1870(way)X
2024(of)X
2111(deciding)X
2407(if)X
2476(a)X
2532(library)X
2766(is)X
2839(out-of-date:)X
10 f
555 3788(g)N
1 f
635(If)X
709(the)X
827(table)X
1003(of)X
1090(contents)X
1377(is)X
1450(older)X
1635(than)X
1793(the)X
1911(library,)X
2165(or)X
2252(is)X
2325(missing,)X
2613(the)X
2731(library)X
2965(is)X
3038(out-of-date.)X
555 3912(A)N
635(library)X
871(is)X
946(any)X
1084(target)X
1289(that)X
1431(looks)X
1626(like)X
1768(``)X
7 f
9 f
1822(-)X
7 f
1866(l)X
1 f
(name'')S
2164(or)X
2253(that)X
2395(ends)X
2564(in)X
2648(a)X
2706(suf\256x)X
2909(that)X
3050(was)X
3196(marked)X
3458(as)X
3546(a)X
3603(library)X
3838(using)X
555 4008(the)N
7 f
673(.LIBS)X
1 f
933(target.)X
7 f
1176(.a)X
1 f
1292(is)X
1365(so)X
1456(marked)X
1717(in)X
1799(the)X
1917(system)X
2159(make\256le.)X
755 4132(Members)N
1157(of)X
1323(an)X
1498(archive)X
1834(are)X
2032(speci\256ed)X
2415(as)X
2580(``)X
2 f
2634(archive)X
1 f
2875(\()X
2 f
2902(member)X
1 f
3161([)X
2 f
3286(member)X
1 f
3545(...]\)''.)X
3851(Thus)X
555 4228(``')N
7 f
636(libdix.a\(window.o\))X
1 f
('')S
1576(speci\256es)X
1874(the)X
1994(\256le)X
7 f
2118(window.o)X
1 f
2524(in)X
2608(the)X
2728(archive)X
7 f
2987(libdix.a)X
1 f
(.)S
3433(You)X
3593(may)X
3753(also)X
3904(use)X
555 4324(wildcards)N
889(to)X
973(specify)X
1227(the)X
1346(members)X
1661(of)X
1749(the)X
1868(archive.)X
2146(Just)X
2291(remember)X
2638(that)X
2779(most)X
2955(the)X
3074(wildcard)X
3376(characters)X
3724(will)X
3869(only)X
555 4420(\256nd)N
2 f
699(existing)X
1 f
981(\256les.)X
755 4544(A)N
844(\256le)X
977(that)X
1128(is)X
1212(a)X
1279(member)X
1573(of)X
1671(an)X
1778(archive)X
2046(is)X
2130(treated)X
2380(specially.)X
2716(If)X
2801(the)X
2930(\256le)X
3063(doesn't)X
3330(exist,)X
3532(but)X
3664(it)X
3738(is)X
3821(in)X
3913(the)X
555 4640(archive,)N
841(the)X
968(modi\256cation)X
1401(time)X
1572(recorded)X
1883(in)X
1974(the)X
2101(archive)X
2367(is)X
2449(used)X
2625(for)X
2748(the)X
2875(\256le)X
3006(when)X
3209(determining)X
3625(if)X
3702(the)X
3828(\256le)X
3958(is)X
555 4736(out-of-date.)N
953(When)X
1166(\256guring)X
1440(out)X
1563(how)X
1722(to)X
1805(make)X
2000(an)X
2097(archived)X
2394(member)X
2677(target)X
2880(\(not)X
3029(the)X
3147(\256le)X
3269(itself,)X
3469(but)X
3591(the)X
3709(\256le)X
3831(in)X
3913(the)X
555 4832(archive)N
812(\320)X
912(the)X
2 f
1030(archive)X
1 f
1271(\()X
2 f
1298(member)X
1 f
1557(\))X
1604(target\),)X
1854(special)X
2097(care)X
2252(is)X
2325(taken)X
2519(with)X
2681(the)X
2799(transformation)X
3291(rules,)X
3487(as)X
3574(follows:)X
10 f
555 4956(g)N
2 f
635(archive)X
1 f
876(\()X
2 f
903(member)X
1 f
1162(\))X
1209(is)X
1282(made)X
1476(to)X
1558(depend)X
1810(on)X
2 f
1910(member)X
1 f
2169(.)X
10 f
555 5080(g)N
1 f
635(The)X
788(transformation)X
1288(from)X
1472(the)X
2 f
1598(member)X
1 f
1857('s)X
1942(suf\256x)X
2151(to)X
2240(the)X
2 f
2365(archive)X
1 f
2606('s)X
2691(suf\256x)X
2900(is)X
2980(applied)X
3243(to)X
3332(the)X
2 f
3457(archive)X
1 f
3698(\()X
2 f
3725(member)X
1 f
3984(\))X
635 5176(target.)N
10 f
555 5300(g)N
1 f
635(The)X
2 f
783(archive)X
1 f
1024(\()X
2 f
1051(member)X
1 f
1310(\)'s)X
7 f
1418(.TARGET)X
1 f
1777(variable)X
2059(is)X
2135(set)X
2247(to)X
2332(the)X
2452(name)X
2648(of)X
2737(the)X
2 f
2857(member)X
1 f
3138(if)X
2 f
3209(member)X
1 f
3490(is)X
3565(actually)X
3841(a)X
3899(tar-)X
635 5396(get,)N
773(or)X
860(the)X
978(path)X
1136(to)X
1218(the)X
1336(member)X
1619(\256le)X
1741(if)X
2 f
1810(member)X
1 f
2089(is)X
2162(only)X
2324(a)X
2380(source.)X
10 f
555 5520(g)N
1 f
635(The)X
7 f
780(.ARCHIVE)X
1 f
1184(variable)X
1463(for)X
1577(the)X
2 f
1695(archive)X
1 f
1936(\()X
2 f
1963(member)X
1 f
2222(\))X
2269(target)X
2472(is)X
2545(set)X
2654(to)X
2736(the)X
2854(name)X
3048(of)X
3135(the)X
2 f
3253(archive)X
1 f
3494(.)X
10 f
555 5644(g)N
1 f
635(The)X
7 f
787(.MEMBER)X
1 f
1150(variable)X
1436(is)X
1515(set)X
1630(to)X
1718(the)X
1842(actual)X
2060(string)X
2268(inside)X
2485(the)X
2609(parentheses.)X
3030(In)X
3123(most)X
3304(cases,)X
3520(this)X
3661(will)X
3811(be)X
3913(the)X
635 5740(same)N
820(as)X
907(the)X
7 f
1025(.TARGET)X
1 f
1381(variable.)X

26 p
%%Page: 26 27
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(26)X
2343(-)X
10 f
555 672(g)N
1 f
635(The)X
2 f
784(archive)X
1 f
1025(\()X
2 f
1052(member)X
1 f
1311(\)'s)X
1420(place)X
1614(in)X
1700(the)X
1822(local)X
2002(variables)X
2316(of)X
2407(the)X
2529(targets)X
2767(that)X
2911(depend)X
3167(on)X
3271(it)X
3339(is)X
3416(taken)X
3613(by)X
3716(the)X
3837(value)X
635 768(of)N
722(its)X
7 f
817(.TARGET)X
1 f
1173(variable.)X
555 892(Thus,)N
755(a)X
811(program)X
1103(library)X
1337(could)X
1535(be)X
1631(created)X
1884(with)X
2046(the)X
2164(following)X
2495(make\256le:)X
7 f
843 1036(.o.a)N
1611(:)X
1227 1132(...)N
1227 1228(rm)N
1371(-f)X
1515($\(.TARGET:T\))X
843 1324(OBJS)N
1611(=)X
1707(obj1.o)X
2043(obj2.o)X
2379(obj3.o)X
843 1420(libprog.a)N
1611(:)X
1707(libprog.a\($\(OBJS\)\))X
1227 1516(ar)N
1371(cru)X
1563($\(.TARGET\))X
2091($\(.OODATE\))X
1227 1612(ranlib)N
1563($\(.TARGET\))X
1 f
555 1756(This)N
722(will)X
871(cause)X
1075(the)X
1198(three)X
1384(object)X
1605(\256les)X
1763(to)X
1850(be)X
1951(compiled)X
2274(\(if)X
2375(the)X
2498(corresponding)X
2982(source)X
3217(\256les)X
3374(were)X
3555(modi\256ed)X
3863(after)X
555 1852(the)N
699(object)X
941(\256le)X
1089(or,)X
1222(if)X
1317(that)X
1483(doesn't)X
1765(exist,)X
1982(the)X
2126(archived)X
2449(object)X
2691(\256le\),)X
2886(the)X
3030(out-of-date)X
3433(ones)X
3626(archived)X
3949(in)X
7 f
555 1948(libprog.a)N
1 f
(,)S
1027(a)X
1083(table)X
1259(of)X
1346(contents)X
1633(placed)X
1863(in)X
1945(the)X
2063(archive)X
2320(and)X
2456(the)X
2574(newly-archived)X
3094(object)X
3310(\256les)X
3463(to)X
3545(be)X
3641(removed.)X
755 2072(All)N
893(this)X
1044(is)X
1132(used)X
1314(in)X
1411(the)X
7 f
1544(makelib.mk)X
1 f
2059(system)X
2316(make\256le)X
2627(to)X
2724(create)X
2952(a)X
3023(single)X
3249(library)X
3498(with)X
3675(ease.)X
3869(This)X
555 2168(make\256le)N
851(looks)X
1044(like)X
1184(this:)X

27 p
%%Page: 27 28
10 s 10 xH 0 xS 1 f
8 s
7 f
1 f
10 s
2196 384(-)N
2243(27)X
2343(-)X
7 f
8 s
843 720(#)N
843 816(#)N
919(Rules)X
1147(for)X
1299(making)X
1565 -0.4167(libraries.)AX
1983(The)X
2135(object)X
2401(files)X
2629(that)X
2819(make)X
3009(up)X
3123(the)X
3275(library)X
3579(are)X
843 912(#)N
919(removed)X
1223(once)X
1413(they)X
1603(are)X
1755 -0.4219(archived.)AX
843 1008(#)N
843 1104(#)N
919(To)X
1033(make)X
1223(several)X
1527 -0.4167(libararies)AX
1945(in)X
2059 -0.4219(parallel,)AX
2439(you)X
2591(should)X
2857(define)X
3123(the)X
3275(variable)X
843 1200(#)N
919 -0.3984("many_libraries".)AX
1603(This)X
1793(will)X
1983 -0.4219(serialize)AX
2363(the)X
2515 -0.4125(invocations)AX
2971(of)X
3085(ranlib.)X
843 1296(#)N
843 1392(#)N
919(To)X
1033(use,)X
1223(do)X
1337 -0.4219(something)AX
1717(like)X
1907(this:)X
843 1488(#)N
843 1584(#)N
919(OBJECTS)X
1223(=)X
1299(<files)X
1565(in)X
1679(the)X
1831(library>)X
843 1680(#)N
843 1776(#)N
919(fish.a:)X
1223 -0.3971(fish.a\($\(OBJECTS\)\))AX
1945(MAKELIB)X
843 1872(#)N
843 1968(#)N
843 2160(#ifndef)N
1147 -0.4125(_MAKELIB_MK)AX
843 2256 -0.4125(_MAKELIB_MK)AN
1443(=)X
843 2448(#include)N
1243(<po.mk>)X
1043 2640(...)N
1043 2736(rm)N
1157(-f)X
1271 -0.4167($\(.MEMBER\))AX
843 2928(ARFLAGS)N
1443(?=)X
1557(crl)X
843 3120(#)N
843 3216(#)N
919 -0.4167(Re-archive)AX
1337(the)X
1489 -0.4125(out-of-date)AX
1945(members)X
2249(and)X
2401(recreate)X
2743(the)X
2895 -0.4219(library's)AX
3275(table)X
3503(of)X
843 3312(#)N
919(contents)X
1261(using)X
1489(ranlib.)X
1793(If)X
1907 -0.4038(many_libraries)AX
2477(is)X
2591(defined,)X
2933(put)X
3085(the)X
3237(ranlib)X
3503(off)X
843 3408(#)N
919(til)X
1071(the)X
1223(end)X
1375(so)X
1489(many)X
1679 -0.4219(libraries)AX
2059(can)X
2211(be)X
2325(made)X
2515(at)X
2629(once.)X
843 3504(#)N
843 3600(MAKELIB)N
1443(:)X
1519(.USE)X
1709 -0.4219(.PRECIOUS)AX
1043 3696(ar)N
1157 -0.4167($\(ARFLAGS\))AX
1575 -0.4167($\(.TARGET\))AX
1993 -0.4167($\(.OODATE\))AX
843 3792(#ifndef)N
1147 -0.4219(no_ranlib)AX
843 3888(#)N
919(ifdef)X
1147 -0.4038(many_libraries)AX
1043 3984(...)N
843 4080(#)N
919(endif)X
1147 -0.4038(many_libraries)AX
1043 4176(ranlib)N
1309 -0.4167($\(.TARGET\))AX
843 4272(#endif)N
1109 -0.4219(no_ranlib)AX
843 4464(#endif)N
1109 -0.4125(_MAKELIB_MK)AX
3 f
10 s
555 4704(4.3.)N
715(On)X
841(the)X
968(Condition...)X
1 f
755 4828(Like)N
934(the)X
1064(C)X
1149(compiler)X
1465(before)X
1702(it,)X
1797(PMake)X
2055(allows)X
2295(you)X
2446(to)X
2539(con\256gure)X
2873(the)X
3002(make\256le,)X
3329(based)X
3543(on)X
3654(the)X
3783(current)X
555 4924(environment,)N
1000(using)X
1193(conditional)X
1573(statements.)X
1951(A)X
2029(conditional)X
2409(looks)X
2602(like)X
2742(this:)X
7 f
843 5068(#if)N
2 f
1035(boolean)X
1313(expression)X
843 5164(lines)N
7 f
843 5260(#elif)N
2 f
1131(another)X
1400(boolean)X
1678(expression)X
843 5356(more)N
1028(lines)X
7 f
843 5452(#else)N
2 f
843 5548(still)N
982(more)X
1167(lines)X
7 f
843 5644(#endif)N
1 f
555 5788(They)N
742(may)X
902(be)X
1000(nested)X
1226(to)X
1309(a)X
1366(maximum)X
1711(depth)X
1910(of)X
1998(30)X
2099(and)X
2236(may)X
2395(occur)X
2595(anywhere)X
2929(\(except)X
3187(in)X
3270(a)X
3327(comment,)X
3666(of)X
3754(course\).)X

28 p
%%Page: 28 29
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(28)X
2343(-)X
555 672(The)N
700(``)X
7 f
754(#)X
1 f
('')S
876(must)X
1051(the)X
1169(very)X
1332(\256rst)X
1476(character)X
1792(on)X
1892(the)X
2010(line.)X
755 796(Each)N
2 f
944(boolean)X
1230(expression)X
1 f
1614(is)X
1695(made)X
1896(up)X
2003(of)X
2097(terms)X
2302(that)X
2449(look)X
2618(like)X
2765(function)X
3059(calls,)X
3253(the)X
3378(standard)X
3677(C)X
3757(boolean)X
555 892(operators)N
7 f
879(&&)X
1 f
(,)S
7 f
1020(||)X
1 f
(,)S
1161(and)X
7 f
1302(!)X
1 f
(,)S
1395(and)X
1536(the)X
1659(standard)X
1955(relational)X
2282(operators)X
7 f
2605(==)X
1 f
(,)S
7 f
2745(!=)X
1 f
(,)S
7 f
2885(>)X
1 f
(,)S
7 f
2977(>=)X
1 f
(,)S
7 f
3117(<)X
1 f
(,)S
3209(and)X
7 f
3349(<=)X
1 f
(,)S
3489(with)X
7 f
3655(==)X
1 f
3775(and)X
7 f
3915(!=)X
1 f
555 988(being)N
755(overloaded)X
1134(to)X
1218(allow)X
1418(string)X
1622(comparisons)X
2049(as)X
2138(well.)X
7 f
2338(&&)X
1 f
2456(represents)X
2804(logical)X
3044(AND;)X
7 f
3262(||)X
1 f
3380(is)X
3455(logical)X
3694(OR)X
3826(and)X
7 f
3963(!)X
1 f
555 1084(is)N
629(logical)X
868(NOT.)X
1094(The)X
1239(arithmetic)X
1584(and)X
1720(string)X
1922(operators)X
2241(take)X
2395 0.3750(precedence)AX
2778(over)X
2941(all)X
3041(three)X
3222(of)X
3309(these)X
3494(operators,)X
3833(while)X
555 1180(NOT)N
745(takes)X
935 0.3750(precedence)AX
1323(over)X
1491(AND,)X
1710(which)X
1930(takes)X
2119 0.3750(precedence)AX
2506(over)X
2673(OR.)X
2848(This)X
3014 0.3750(precedence)AX
3401(may)X
3563(be)X
3663(overridden)X
555 1276(with)N
722(parentheses,)X
1142(and)X
1283(an)X
1383(expression)X
1750(may)X
1912(be)X
2012(parenthesized)X
2478(to)X
2564(your)X
2735(heart's)X
2978(content.)X
3278(Each)X
3463(term)X
3634(looks)X
3831(like)X
3975(a)X
555 1372(call)N
691(on)X
791(one)X
927(of)X
1014(four)X
1168(functions:)X
555 1496(make)N
836(The)X
985(syntax)X
1218(is)X
7 f
1295(make\()X
2 f
(target)S
7 f
1726(\))X
1 f
1798(where)X
2 f
2019(target)X
1 f
2247(is)X
2324(a)X
2384(target)X
2591(in)X
2676(the)X
2797(make\256le.)X
3116(This)X
3281(is)X
3357(true)X
3505(if)X
3577(the)X
3698(given)X
3899(tar-)X
836 1592(get)N
957(was)X
1105(speci\256ed)X
1413(on)X
1516(the)X
1637(command)X
1976(line,)X
2139(or)X
2229(as)X
2319(the)X
2440(source)X
2673(for)X
2790(a)X
7 f
2849(.MAIN)X
1 f
3112(target)X
3318(\(note)X
3506(that)X
3649(the)X
3770(sources)X
836 1688(for)N
7 f
950(.MAIN)X
1 f
1210(are)X
1329(only)X
1491(used)X
1658(if)X
1727(no)X
1827(targets)X
2061(were)X
2238(given)X
2436(on)X
2536(the)X
2654(command)X
2990(line\).)X
555 1812(de\256ned)N
836(The)X
983(syntax)X
1214(is)X
7 f
1288(defined\()X
2 f
(variable)S
7 f
1939(\))X
1 f
2008(and)X
2145(is)X
2219(true)X
2365(if)X
2 f
2435(variable)X
1 f
2736(is)X
2810(de\256ned.)X
3087(Certain)X
3344(variables)X
3655(are)X
3775(de\256ned)X
836 1908(in)N
918(the)X
1036(system)X
1278(make\256le)X
1574(that)X
1714(identify)X
1983(the)X
2101(system)X
2343(on)X
2443(which)X
2659(PMake)X
2906(is)X
2979(being)X
3177(run.)X
555 2032(exists)N
836(The)X
983(syntax)X
1214(is)X
7 f
1288(exists\()X
2 f
(\256le)S
7 f
1722(\))X
1 f
1791(and)X
1928(is)X
2002(true)X
2148(if)X
2218(the)X
2337(\256le)X
2460(can)X
2593(be)X
2690(found)X
2898(on)X
2999(the)X
3118(global)X
3339(search)X
3566(path)X
3725(\(i.e.)X
3891(that)X
836 2128(de\256ned)N
1092(by)X
7 f
1192(.PATH)X
1 f
1452(targets,)X
1706(not)X
1828(by)X
7 f
1928(.PATH)X
2 f
(suf\256x)S
1 f
2357(targets\).)X
555 2252(empty)N
836(This)X
1001(syntax)X
1233(is)X
1309(much)X
1509(like)X
1651(the)X
1771(others,)X
2009(except)X
2241(the)X
2361(string)X
2565(inside)X
2778(the)X
2898(parentheses)X
3295(is)X
3370(of)X
3459(the)X
3579(same)X
3766(form)X
3944(as)X
836 2348(you)N
993(would)X
1229(put)X
1367(between)X
1671(parentheses)X
2082(when)X
2292(expanding)X
2662(a)X
2734(variable,)X
3049(complete)X
3379(with)X
3557(modi\256ers)X
3895(and)X
836 2444(everything.)N
1221(The)X
1368(function)X
1656(returns)X
1900(true)X
2046(if)X
2116(the)X
2235(resulting)X
2536(string)X
2739(is)X
2813(empty)X
3034(\(NOTE:)X
3318(an)X
3415(unde\256ned)X
3752(variable)X
836 2540(in)N
922(this)X
1061(context)X
1321(will)X
1469(cause)X
1672(at)X
1754(the)X
1876(very)X
2043(least)X
2214(a)X
2274(warning)X
2561(message)X
2857(about)X
3058(a)X
3117(malformed)X
3492(conditional,)X
3895(and)X
836 2636(at)N
916(the)X
1036(worst)X
1236(will)X
1382(cause)X
1583(the)X
1703(process)X
1966(to)X
2050(stop)X
2205(once)X
2379(it)X
2445(has)X
2574(read)X
2735(the)X
2855(make\256le.)X
3173(If)X
3249(you)X
3391(want)X
3568(to)X
3651(check)X
3860(for)X
3975(a)X
836 2732(variable)N
1124(being)X
1331(de\256ned)X
1595(or)X
1690(empty,)X
1938(use)X
2073(the)X
2199(expression)X
2570(``)X
7 f
2624(!defined\()X
2 f
(var)S
7 f
3163(\))X
3267(||)X
3419(empty\()X
2 f
(var)S
7 f
3814(\))X
1 f
('')S
3944(as)X
836 2828(the)N
960(de\256nition)X
1292(of)X
7 f
1385(||)X
1 f
1507(will)X
1657(prevent)X
1924(the)X
7 f
2047(empty\(\))X
1 f
2408(from)X
2589(being)X
2792(evaluated)X
3125(and)X
3266(causing)X
3536(an)X
3637(error,)X
3839(if)X
3913(the)X
836 2924(variable)N
1115(is)X
1188(unde\256ned\).)X
1571(This)X
1733(can)X
1865(be)X
1961(used)X
2128(to)X
2210(see)X
2333(if)X
2402(a)X
2458(variable)X
2737(contains)X
3024(a)X
3080(given)X
3278(word,)X
3483(for)X
3597(example:)X
7 f
1124 3068(#if)N
1316(!empty\()X
2 f
(var)S
7 f
1759(:M)X
2 f
(word)S
7 f
2019(\))X
1 f
755 3240(The)N
901(arithmetic)X
1247(and)X
1383(string)X
1585(operators)X
1904(may)X
2062(only)X
2224(be)X
2320(used)X
2487(to)X
2569(test)X
2700(the)X
2818(value)X
3012(of)X
3099(a)X
3155(variable.)X
3454(The)X
3599(lefthand)X
3882(side)X
555 3336(must)N
735(contain)X
996(the)X
1119(variable)X
1403(expansion,)X
1773(while)X
1976(the)X
2099(righthand)X
2431(side)X
2585(contains)X
2877(either)X
3085(a)X
3146(string,)X
3373(enclosed)X
3679(in)X
3766(double-)X
555 3432(quotes,)N
806(or)X
895(a)X
953(number.)X
1240(The)X
1387(standard)X
1681(C)X
1756(numeric)X
2041(conventions)X
2450(\(except)X
2709(for)X
2825(specifying)X
3181(an)X
3279(octal)X
3457(number\))X
3750(apply)X
3949(to)X
555 3528(both)N
717(sides.)X
917(E.g.)X
7 f
843 3672(#if)N
1035($\(OS\))X
1323(==)X
1467(4.3)X
843 3864(#if)N
1035($\(MACHINE\))X
1563(==)X
1707("sun3")X
843 4056(#if)N
1035($\(LOAD_ADDR\))X
1659(<)X
1755(0xc000)X
1 f
555 4200(are)N
674(all)X
774(valid)X
954(conditionals.)X
1385(In)X
1472(addition,)X
1774(the)X
1892(numeric)X
2175(value)X
2369(of)X
2456(a)X
2512(variable)X
2791(can)X
2923(be)X
3019(tested)X
3226(as)X
3313(a)X
3369(boolean)X
3643(as)X
3730(follows:)X
7 f
843 4344(#if)N
1035($\(LOAD\))X
1 f
555 4488(would)N
775(see)X
898(if)X
7 f
967(LOAD)X
1 f
1179(contains)X
1466(a)X
1522(non-zero)X
1828(value)X
2022(and)X
7 f
843 4632(#if)N
1035(!$\(LOAD\))X
1 f
555 4776(would)N
775(test)X
906(if)X
7 f
975(LOAD)X
1 f
1187(contains)X
1474(a)X
1530(zero)X
1689(value.)X
755 4900(In)N
844(addition)X
1128(to)X
1212(the)X
1332(bare)X
1493(``)X
7 f
1547(#if)X
1 f
(,'')S
1787(there)X
1970(are)X
2091(other)X
2278(forms)X
2487(that)X
2629(apply)X
2829(one)X
2967(of)X
3056(the)X
3175(\256rst)X
3320(two)X
3461(functions)X
3780(to)X
3863(each)X
555 4996(term.)N
742(They)X
927(are)X
1046(as)X
1133(follows:)X
7 f
1043 5140(ifdef)N
1 f
1443(de\256ned)X
7 f
1043 5236(ifndef)N
1 f
1443(!de\256ned)X
7 f
1043 5332(ifmake)N
1 f
1443(make)X
7 f
1043 5428(ifnmake)N
1 f
1443(!make)X
555 5572(There)N
763(are)X
882(also)X
1031(the)X
1149(``else)X
1348(if'')X
1471(forms:)X
7 f
1700(elif)X
1 f
(,)S
7 f
1932(elifdef)X
1 f
(,)S
7 f
2308(elifndef)X
1 f
(,)S
7 f
2732(elifmake)X
1 f
(,)S
3156(and)X
7 f
3292(elifnmake)X
1 f
(.)S
755 5696(For)N
887(instance,)X
1191(if)X
1261(you)X
1402(wish)X
1574(to)X
1657(create)X
1871(two)X
2012(versions)X
2300(of)X
2388(a)X
2445(program,)X
2758(one)X
2895(of)X
2983(which)X
3200(is)X
3274(optimized)X
3615(\(the)X
3761(produc-)X
555 5792(tion)N
700(version\))X
984(and)X
1121(the)X
1240(other)X
1426(of)X
1514(which)X
1731(is)X
1805(for)X
1920(debugging)X
2279(\(has)X
2434(symbols)X
2721(for)X
2836(dbx\),)X
3024(you)X
3164(have)X
3336(two)X
3476(choices:)X
3759(you)X
3899(can)X

29 p
%%Page: 29 30
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(29)X
2343(-)X
555 672(create)N
769(two)X
910(make\256les,)X
1258(one)X
1395(of)X
1483(which)X
1700(uses)X
1859(the)X
7 f
9 f
1978(-)X
7 f
2022(g)X
1 f
2091(\257ag)X
2232(for)X
2347(the)X
2466(compilation,)X
2889(while)X
3088(the)X
3207(other)X
3393(uses)X
3552(the)X
7 f
9 f
3671(-)X
7 f
3715(O)X
1 f
3784(\257ag,)X
3944(or)X
555 768(you)N
699(can)X
835(use)X
966(another)X
1231(target)X
1438(\(call)X
1605(it)X
7 f
1673(debug)X
1 f
(\))S
1964(to)X
2050(create)X
2267(the)X
2389(debug)X
2609(version.)X
2888(The)X
3036(construct)X
3353(below)X
3572(will)X
3719(take)X
3876(care)X
555 864(of)N
650(this)X
793(for)X
915(you.)X
1083(I)X
1138(have)X
1318(also)X
1475(made)X
1677(it)X
1749(so)X
1848(de\256ning)X
2138(the)X
2264(variable)X
7 f
2550(DEBUG)X
1 f
2817(\(say)X
2978(with)X
7 f
3147(pmake)X
3442(-D)X
3593(DEBUG)X
1 f
(\))S
3887(will)X
555 960(also)N
704(cause)X
903(the)X
1021(debug)X
1237(version)X
1493(to)X
1575(be)X
1671(made.)X
7 f
843 1104(#if)N
1035(defined\(DEBUG\))X
1755(||)X
1899(make\(debug\))X
843 1200(CFLAGS)N
1443(+=)X
1587(-g)X
843 1296(#else)N
843 1392(CFLAGS)N
1443(+=)X
1587(-O)X
843 1488(#endif)N
1 f
555 1632(There)N
771(are,)X
918(of)X
1013(course,)X
1271(problems)X
1597(with)X
1766(this)X
1908(approach.)X
2250(The)X
2402(most)X
2584(glaring)X
2838(annoyance)X
3209(is)X
3289(that)X
3436(if)X
3512(you)X
3659(want)X
3842(to)X
3931(go)X
555 1728(from)N
738(making)X
1005(a)X
1068(debug)X
1291(version)X
1554(to)X
1643(making)X
1910(a)X
1973(production)X
2347(version,)X
2630(you)X
2777(have)X
2956(to)X
3045(remove)X
3313(all)X
3419(the)X
3543(object)X
3765(\256les,)X
3944(or)X
555 1824(you)N
705(will)X
859(get)X
987(some)X
1186(optimized)X
1536(and)X
1682(some)X
1881(debug)X
2107(versions)X
2404(in)X
2496(the)X
2624(same)X
2819(program.)X
3141(Another)X
3434(annoyance)X
3808(is)X
3891(you)X
555 1920(have)N
735(to)X
825(be)X
929(careful)X
1181(not)X
1311(to)X
1400(make)X
1601(two)X
1748(targets)X
1989(that)X
2136(``con\257ict'')X
2511(because)X
2793(of)X
2887(some)X
3083(conditionals)X
3501(in)X
3590(the)X
3715(make\256le.)X
555 2016(For)N
686(instance)X
7 f
843 2160(#if)N
1035(make\(print\))X
843 2256(FORMATTER)N
1443(=)X
1539(ditroff)X
1923(-Plaser_printer)X
843 2352(#endif)N
843 2448(#if)N
1035(make\(draft\))X
843 2544(FORMATTER)N
1443(=)X
1539(nroff)X
1827(-Pdot_matrix_printer)X
843 2640(#endif)N
1 f
555 2784(would)N
781(wreak)X
1004(havok)X
1226(if)X
1301(you)X
1447(tried)X
1620(``)X
7 f
1674(pmake)X
1968(draft)X
2262(print)X
1 f
('')S
2582(since)X
2773(you)X
2919(would)X
3145(use)X
3278(the)X
3402(same)X
3593(formatter)X
3917(for)X
555 2880(each)N
723(target.)X
946(As)X
1055(I)X
1102(said,)X
1271(this)X
1406(all)X
1506(gets)X
1655(somewhat)X
2000(complicated.)X
3 f
555 3072(4.4.)N
715(A)X
793(Shell)X
981(is)X
1054(a)X
1114(Shell)X
1302(is)X
1375(a)X
1435(Shell)X
1 f
755 3196(In)N
847(normal)X
1099(operation,)X
1447(the)X
1570(Bourne)X
1831(Shell)X
2020(\(better)X
2255(known)X
2498(as)X
2590(``)X
7 f
2644(sh)X
1 f
(''\))S
2846(is)X
2924(used)X
3096(to)X
3183(execute)X
3454(the)X
3577(commands)X
3949(to)X
555 3292 0.4219(re-create)AN
859(targets.)X
1114(PMake)X
1362(also)X
1512(allows)X
1742(you)X
1883(to)X
1966(specify)X
2219(a)X
2276(different)X
2574(shell)X
2746(for)X
2861(it)X
2926(to)X
3008(use)X
3135(when)X
3329(executing)X
3661(these)X
3846(com-)X
555 3388(mands.)N
819(There)X
1042(are)X
1176(several)X
1439(things)X
1669(PMake)X
1931(must)X
2120(know)X
2332(about)X
2544(the)X
2676(shell)X
2861(you)X
3015(wish)X
3200(to)X
3296(use.)X
3457(These)X
3683(things)X
3912(are)X
555 3484(speci\256ed)N
860(as)X
947(the)X
1065(sources)X
1326(for)X
1440(the)X
7 f
1558(.SHELL)X
1 f
1866(target)X
2069(by)X
2169(keyword,)X
2490(as)X
2577(follows:)X
3 f
555 3608(path=)N
2 f
756(path)X
1 f
755 3704(PMake)N
1010(needs)X
1221(to)X
1311(know)X
1516(where)X
1740(the)X
1865(shell)X
2043(actually)X
2324(resides,)X
2594(so)X
2692(it)X
2763(can)X
2902(execute)X
3175(it.)X
3266(If)X
3347(you)X
3494(specify)X
3753(this)X
3895(and)X
755 3800(nothing)N
1031(else,)X
1208(PMake)X
1467(will)X
1623(use)X
1762(the)X
1892(last)X
2035(component)X
2423(of)X
2522(the)X
2652(path)X
2821(and)X
2968(look)X
3141(in)X
3234(its)X
3340(table)X
3527(of)X
3625(the)X
3754(shells)X
3967(it)X
755 3896(knows)N
987(and)X
1126(use)X
1256(the)X
1377(speci\256cation)X
1805(it)X
1872(\256nds,)X
2070(if)X
2142(any.)X
2301(Use)X
2449(this)X
2587(if)X
2659(you)X
2801(just)X
2938(want)X
3116(to)X
3200(use)X
3329(a)X
3387(different)X
3686(version)X
3944(of)X
755 3992(the)N
873(Bourne)X
1129(or)X
1216(C)X
1289(Shell)X
1473(\(yes,)X
1647(PMake)X
1894(knows)X
2123(how)X
2281(to)X
2363(use)X
2490(the)X
2608(C)X
2681(Shell)X
2865(too\).)X
3 f
555 4116(name=)N
2 f
788(name)X
1 f
755 4212(This)N
920(is)X
996(the)X
1117(name)X
1314(by)X
1417(which)X
1636(the)X
1757(shell)X
1931(is)X
2007(to)X
2092(be)X
2191(known.)X
2452(It)X
2524(is)X
2600(a)X
2659(single)X
2873(word)X
3060(and,)X
3218(if)X
3289(no)X
3391(other)X
3578(keywords)X
3912(are)X
755 4308(speci\256ed)N
1065(\(other)X
1282(than)X
3 f
1445(path)X
1 f
1600(\),)X
1672(it)X
1741(is)X
1818(the)X
1940(name)X
2138(by)X
2242(which)X
2462(PMake)X
2713(attempts)X
3008(to)X
3094(\256nd)X
3242(a)X
3302(speci\256cation)X
3731(for)X
3849(it)X
3917(\(as)X
755 4404(mentioned)N
1120(above\).)X
1386(You)X
1551(can)X
1690(use)X
1824(this)X
1966(if)X
2042(you)X
2189(would)X
2416(just)X
2557(rather)X
2771(use)X
2904(the)X
3028(C)X
3107(Shell)X
3297(than)X
3461(the)X
3585(Bourne)X
3847(Shell)X
755 4500(\(``)N
7 f
836(.SHELL:)X
1220(name=csh)X
1 f
('')S
1678(will)X
1822(do)X
1922(it\).)X
3 f
555 4624(quiet=)N
2 f
(echo-off)S
1057(command)X
1 f
755 4720(As)N
871(mentioned)X
1236(before,)X
1489(PMake)X
1743(actually)X
2024(controls)X
2309(whether)X
2594(commands)X
2967(are)X
3092(printed)X
3345(by)X
3451(introducing)X
3846(com-)X
755 4816(mands)N
990(into)X
1140(the)X
1264(shell's)X
1499(input)X
1689(stream.)X
1949(This)X
2117(keyword,)X
2444(and)X
2586(the)X
2709(next)X
2872(two,)X
3037(control)X
3289(what)X
3470(those)X
3664(commands)X
755 4912(are.)N
902(The)X
3 f
1055(quiet)X
1 f
1256(keyword)X
1565(is)X
1646(the)X
1772(command)X
2115(used)X
2289(to)X
2378(turn)X
2534(echoing)X
2815(off.)X
2956(Once)X
3153(it)X
3224(is)X
3304(turned)X
3536(off,)X
3677(echoing)X
3958(is)X
755 5008(expected)N
1061(to)X
1143(remain)X
1386(off)X
1500(until)X
1666(the)X
1784(echo-on)X
2063(command)X
2399(is)X
2472(given.)X
3 f
555 5132(echo=)N
2 f
757(echo-on)X
1036(command)X
1 f
755 5228(The)N
900(command)X
1236(PMake)X
1483(should)X
1716(give)X
1874(to)X
1956(turn)X
2105(echoing)X
2379(back)X
2551(on)X
2651(again.)X
3 f
555 5352(\256lter=)N
2 f
766(printed)X
1017(echo-off)X
1300(command)X
1 f
755 5448(Many)N
971(shells)X
1182(will)X
1335(echo)X
1516(the)X
1643(echo-off)X
1945(command)X
2289(when)X
2491(it)X
2563(is)X
2644(given.)X
2870(This)X
3040(keyword)X
3349(tells)X
3510(PMake)X
3765(in)X
3855(what)X
755 5544(format)N
990(the)X
1109(shell)X
1281(actually)X
1556(prints)X
1759(the)X
1877(echo-off)X
2170(command.)X
2526(Wherever)X
2864(PMake)X
3111(sees)X
3265(this)X
3400(string)X
3602(in)X
3684(the)X
3802(shell's)X
755 5640(output,)N
1004(it)X
1073(will)X
1222(delete)X
1439(it)X
1508(and)X
1649(any)X
1790(following)X
2126(whitespace,)X
2528(up)X
2633(to)X
2720(and)X
2861(including)X
3188(the)X
3311(next)X
3474(newline.)X
3773(See)X
3913(the)X
755 5736(example)N
1047(at)X
1125(the)X
1243(end)X
1379(of)X
1466(this)X
1601(section)X
1848(for)X
1962(more)X
2147(details.)X

30 p
%%Page: 30 31
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(30)X
2343(-)X
3 f
555 672(echoFlag=)N
2 f
908(\257ag)X
1048(to)X
1130(turn)X
1283(echoing)X
1557(on)X
1 f
755 768(Unless)N
996(a)X
1055(target)X
1261(has)X
1390(been)X
1564(marked)X
7 f
1827(.SILENT)X
1 f
(,)S
2205(PMake)X
2454(wants)X
2663(to)X
2747(start)X
2907(the)X
3027(shell)X
3200(running)X
3471(with)X
3635(echoing)X
3911(on.)X
755 864(To)N
867(do)X
970(this,)X
1128(it)X
1195(passes)X
1423(this)X
1561(\257ag)X
1704(to)X
1789(the)X
1910(shell)X
2084(as)X
2173(one)X
2311(of)X
2400(its)X
2497(arguments.)X
2873(If)X
2949(either)X
3154(this)X
3291(or)X
3380(the)X
3500(next)X
3660(\257ag)X
3802(begins)X
755 960(with)N
918(a)X
975(`)X
9 f
1002(-)X
1 f
1046(',)X
1114(the)X
1233(\257ags)X
1405(will)X
1550(be)X
1647(passed)X
1882(to)X
1965(the)X
2084(shell)X
2255(as)X
2342(separate)X
2626(arguments.)X
3000(Otherwise,)X
3370(the)X
3488(two)X
3628(will)X
3772(be)X
3868(con-)X
755 1056(catenated)N
1079(\(if)X
1175(they)X
1333(are)X
1452(used)X
1619(at)X
1697(the)X
1815(same)X
2000(time,)X
2182(of)X
2269(course\).)X
3 f
555 1180(errFlag=)N
2 f
860(\257ag)X
1000(to)X
1082(turn)X
1235(error)X
1424(checking)X
1730(on)X
1 f
755 1276(Likewise,)N
1092(unless)X
1315(a)X
1374(target)X
1580(is)X
1656(marked)X
7 f
1920(.IGNORE)X
1 f
(,)S
2299(PMake)X
2549(wishes)X
2790 0.2500(error-checking)AX
3286(to)X
3370(be)X
3468(on)X
3570(from)X
3748(the)X
3868(very)X
755 1372(start.)N
939(To)X
1054(this)X
1195(end,)X
1357(it)X
1427(will)X
1577(pass)X
1741(this)X
1882(\257ag)X
2028(to)X
2116(the)X
2240(shell)X
2417(as)X
2510(an)X
2612(argument.)X
2961(The)X
3111(same)X
3301(rules)X
3482(for)X
3601(an)X
3702(initial)X
3913(`)X
9 f
3940(-)X
1 f
3984(')X
755 1468(apply)N
953(as)X
1040(for)X
1154(the)X
3 f
1272(echoFlag)X
1 f
1579(.)X
3 f
555 1592(check=)N
2 f
797(command)X
1129(to)X
1211(turn)X
1364(error)X
1553(checking)X
1859(on)X
1 f
755 1688(Just)N
917(as)X
1022(for)X
1154(echo-control,)X
1618(error-control)X
2067(is)X
2158(achieved)X
2482(by)X
2600(inserting)X
2918(commands)X
3303(into)X
3465(the)X
3601(shell's)X
3847(input)X
755 1784(stream.)N
1013(This)X
1179(is)X
1256(the)X
1377(command)X
1716(to)X
1801(make)X
1998(the)X
2119(shell)X
2293(check)X
2504(for)X
2621(errors.)X
2852(It)X
2924(also)X
3076(serves)X
3300(another)X
3564(purpose)X
3841(if)X
3913(the)X
755 1880(shell)N
936(doesn't)X
1201(have)X
1382(error-control)X
1822(as)X
1918(commands,)X
2314(but)X
2445(I'll)X
2572(get)X
2699(into)X
2852(that)X
3001(in)X
3092(a)X
3157(minute.)X
3428(Again,)X
3673(once)X
3854(error)X
755 1976(checking)N
1065(has)X
1192(been)X
1364(turned)X
1589(on,)X
1709(it)X
1773(is)X
1846(expected)X
2152(to)X
2234(remain)X
2477(on)X
2577(until)X
2743(it)X
2807(is)X
2880(turned)X
3105(off)X
3219(again.)X
3 f
555 2100(ignore=)N
2 f
819(command)X
1151(to)X
1233(turn)X
1386(error)X
1575(checking)X
1881(off)X
1 f
755 2196(This)N
919(is)X
994(the)X
1114(command)X
1452(PMake)X
1701(uses)X
1861(to)X
1945(turn)X
2096(error)X
2275(checking)X
2587(off.)X
2723(It)X
2794(has)X
2923(another)X
3185(use)X
3313(if)X
3383(the)X
3502(shell)X
3674(doesn't)X
3931(do)X
755 2292(error-control,)N
1206(but)X
1328(I'll)X
1446(tell)X
1568(you)X
1708(about)X
1906(that.)X
2059(.)X
2092(.)X
2125(now.)X
3 f
555 2416(hasErrCtl=)N
2 f
948(yes)X
1071(or)X
1162(no)X
1 f
755 2512(This)N
921(takes)X
1110(a)X
1170(value)X
1368(that)X
1512(is)X
1588(either)X
3 f
1794(yes)X
1 f
1924(or)X
3 f
2014(no)X
1 f
2098(.)X
2161(Now)X
2340(you)X
2483(might)X
2692(think)X
2879(that)X
3022(the)X
3143(existence)X
3465(of)X
3555(the)X
3 f
3676(check)X
1 f
3895(and)X
3 f
755 2608(ignore)N
1 f
1007(keywords)X
1353(would)X
1587(be)X
1697(enough)X
1967(to)X
2063(tell)X
2199(PMake)X
2460(if)X
2543(the)X
2674(shell)X
2858(can)X
3003(do)X
3116(error-control,)X
3580(but)X
3715(you'd)X
3935(be)X
755 2704(wrong.)N
1017(If)X
3 f
1108(hasErrCtl)X
1 f
1492(is)X
3 f
1582(yes)X
1 f
1689(,)X
1746(PMake)X
2010(uses)X
2185(the)X
2319(check)X
2543(and)X
2695(ignore)X
2936(commands)X
3319(in)X
3417(a)X
3489(straight-forward)X
755 2800(manner.)N
1060(If)X
1138(this)X
1277(is)X
3 f
1353(no)X
1 f
1437(,)X
1480(however,)X
1800(their)X
1970(use)X
2100(is)X
2176(rather)X
2387(different.)X
2707(In)X
2797(this)X
2935(case,)X
3117(the)X
3238(check)X
3449(command)X
3788(is)X
3864(used)X
755 2896(as)N
843(a)X
900(template,)X
1217(in)X
1300(which)X
1517(the)X
1636(string)X
3 f
1839(%s)X
1 f
1971(is)X
2045(replaced)X
2339(by)X
2440(the)X
2559(command)X
2895(that's)X
3093(about)X
3291(to)X
3373(be)X
3469(executed,)X
3795(to)X
3877(pro-)X
755 2992(duce)N
934(a)X
997(command)X
1340(for)X
1461(the)X
1586(shell)X
1764(that)X
1911(will)X
2062(echo)X
2241(the)X
2366(command)X
2709(to)X
2798(be)X
2901(executed.)X
3234(The)X
3385(ignore)X
3616(command)X
3958(is)X
755 3088(also)N
908(used)X
1079(as)X
1170(a)X
1230(template,)X
1550(again)X
1748(with)X
3 f
1914(%s)X
1 f
2048(replaced)X
2344(by)X
2447(the)X
2568(command)X
2907(to)X
2992(be)X
3091(executed,)X
3420(to)X
3505(produce)X
3787(a)X
3846(com-)X
755 3184(mand)N
964(that)X
1115(will)X
1270(execute)X
1547(the)X
1676(command)X
2023(to)X
2116(be)X
2223(executed)X
2540(and)X
2687(ignore)X
2923(any)X
3070(error)X
3257(it)X
3331(returns.)X
3624(When)X
3846(these)X
755 3280(strings)N
988(are)X
1107(used)X
1274(as)X
1361(templates,)X
1708(you)X
1848(must)X
2023(provide)X
2288(newline\(s\))X
2647(\(``)X
7 f
2728(\\n)X
1 f
(''\))S
2925(in)X
3007(the)X
3125(appropriate)X
3511(place\(s\).)X
755 3404(The)N
906(strings)X
1145(that)X
1291(follow)X
1526(these)X
1717(keywords)X
2055(may)X
2219(be)X
2321(enclosed)X
2628(in)X
2716(single)X
2933(or)X
3026(double)X
3269(quotes)X
3503(\(the)X
3653(quotes)X
3887(will)X
555 3500(be)N
659(stripped)X
945(off\))X
1094(and)X
1238(may)X
1404(contain)X
1668(the)X
1794(usual)X
1991(C)X
2072 0.2237(backslash-characters)AX
2766(\(\\n)X
2883(is)X
2964(newline,)X
3266(\\r)X
3343(is)X
3424(return,)X
3663(\\b)X
3752(is)X
3832(back-)X
555 3596(space,)N
776(\\')X
847(escapes)X
1115(a)X
1173(single-quote)X
1591(inside)X
1804(single-quotes,)X
2273(\\")X
2350(escapes)X
2618(a)X
2676(double-quote)X
3121(inside)X
3333(double-quotes\).)X
3855(Now)X
555 3692(for)N
669(an)X
765(example.)X
755 3816(This)N
934(is)X
1024(actually)X
1315(the)X
1450(contents)X
1754(of)X
1858(the)X
7 f
1993(<shx.mk>)X
1 f
2414(system)X
2673(make\256le,)X
3006(and)X
3159(causes)X
3406(PMake)X
3670(to)X
3769(use)X
3913(the)X
555 3912(Bourne)N
815(Shell)X
1003(in)X
1089(such)X
1260(a)X
1320(way)X
1478(that)X
1622(each)X
1793(command)X
2132(is)X
2208(printed)X
2458(as)X
2548(it)X
2615(is)X
2691(executed.)X
3020(That)X
3190(is,)X
3286(if)X
3358(more)X
3546(than)X
3707(one)X
3846(com-)X
555 4008(mand)N
761(is)X
842(given)X
1048(on)X
1156(a)X
1220(line,)X
1388(each)X
1564(will)X
1716(be)X
1819(printed)X
2073(separately.)X
2466(Similarly,)X
2810(each)X
2985(time)X
3154(the)X
3279(body)X
3466(of)X
3560(a)X
3623(loop)X
3792(is)X
3872(exe-)X
555 4104(cuted,)N
769(the)X
887(commands)X
1254(within)X
1478(that)X
1618(loop)X
1780(will)X
1924(be)X
2020(printed,)X
2287(etc.)X
2421(The)X
2566(speci\256cation)X
2991(runs)X
3149(like)X
3289(this:)X
7 f
843 4248(#)N
843 4344(#)N
939(This)X
1179(is)X
1323(a)X
1419(shell)X
1707(specification)X
2379(to)X
2523(have)X
2763(the)X
2955(bourne)X
3291(shell)X
3579(echo)X
843 4440(#)N
939(the)X
1131(commands)X
1563(just)X
1803(before)X
2139(executing)X
2619(them,)X
2907(rather)X
3243(than)X
3483(when)X
3723(it)X
3867(reads)X
843 4536(#)N
939(them.)X
1227(Useful)X
1563(if)X
1707(you)X
1899(want)X
2139(to)X
2283(see)X
2475(how)X
2667(variables)X
3147(are)X
3339(being)X
3627(expanded,)X
4107(etc.)X
843 4632(#)N
843 4728(.SHELL)N
1243(:)X
1339(path=/bin/sh)X
1963(\\)X
1043 4824(quiet="set)N
1571(-")X
1715(\\)X
1043 4920(echo="set)N
1523(-x")X
1715(\\)X
1043 5016(filter="+)N
1523(set)X
1715(-)X
1811(")X
1907(\\)X
1043 5112(echoFlag=x)N
1571(\\)X
1043 5208(errFlag=e)N
1523(\\)X
1043 5304(hasErrCtl=yes)N
1715(\\)X
1043 5400(check="set)N
1571(-e")X
1763(\\)X
1043 5496(ignore="set)N
1619(+e")X
1 f
555 5668(It)N
624(tells)X
777(PMake)X
1024(the)X
1142(following:)X

31 p
%%Page: 31 32
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(31)X
2343(-)X
10 f
555 672(g)N
1 f
635(The)X
790(shell)X
971(is)X
1054(located)X
1316(in)X
1408(the)X
1536(\256le)X
7 f
1668(/bin/sh)X
1 f
(.)S
2074(It)X
2153(need)X
2335(not)X
2467(tell)X
2599(PMake)X
2855(that)X
3004(the)X
3131(name)X
3334(of)X
3430(the)X
3557(shell)X
3737(is)X
7 f
3819(sh)X
1 f
3944(as)X
635 768(PMake)N
882(can)X
1014(\256gure)X
1221(that)X
1361(out)X
1483(for)X
1597(itself)X
1777(\(it's)X
1926(the)X
2044(last)X
2175(component)X
2551(of)X
2638(the)X
2756(path\).)X
10 f
555 892(g)N
1 f
635(The)X
780(command)X
1116(to)X
1198(stop)X
1351(echoing)X
1625(is)X
7 f
1698(set)X
1890(-)X
1 f
(.)S
10 f
555 1016(g)N
1 f
635(The)X
780(command)X
1116(to)X
1198(start)X
1356(echoing)X
1630(is)X
7 f
1703(set)X
1895(-x)X
1 f
(.)S
10 f
555 1140(g)N
1 f
635(When)X
852(the)X
975(echo)X
1152(off)X
1271(command)X
1612(is)X
1690(executed,)X
2020(the)X
2142(shell)X
2317(will)X
2465(print)X
7 f
2640(+)X
2740(set)X
2936(-)X
1 f
3008(\(The)X
3184(`+')X
3307(comes)X
3536(from)X
3716(using)X
3913(the)X
7 f
9 f
635 1236(-)N
7 f
679(x)X
1 f
753(\257ag)X
899(\(rather)X
1140(than)X
1304(the)X
7 f
9 f
1428(-)X
7 f
1472(v)X
1 f
1546(\257ag)X
1692(PMake)X
1945(usually)X
2202(uses\)\).)X
2440(PMake)X
2693(will)X
2843(remove)X
3109(all)X
3214(occurences)X
3597(of)X
3689(this)X
3829(string)X
635 1332(from)N
811(the)X
929(output,)X
1173(so)X
1264(you)X
1404(don't)X
1593(notice)X
1809(extra)X
1990(commands)X
2357(you)X
2497(didn't)X
2708(put)X
2830(there.)X
10 f
555 1456(g)N
1 f
635(The)X
781(\257ag)X
922(the)X
1041(Bourne)X
1298(Shell)X
1483(will)X
1628(take)X
1783(to)X
1866(start)X
2025(echoing)X
2300(in)X
2383(this)X
2519(way)X
2674(is)X
2748(the)X
7 f
9 f
2867(-)X
7 f
2911(x)X
1 f
2980(\257ag.)X
3140(The)X
3285(Bourne)X
3541(Shell)X
3725(will)X
3869(only)X
635 1552(take)N
798(its)X
902(\257ag)X
1051(arguments)X
1414(concatenated)X
1863(as)X
1959(its)X
2062(\256rst)X
2214(argument,)X
2565(so)X
2664(neither)X
2915(this)X
3058(nor)X
3193(the)X
3 f
3319(errFlag)X
1 f
3606(speci\256cation)X
635 1648(begins)N
864(with)X
1026(a)X
9 f
1082(-)X
1 f
1126(.)X
10 f
555 1772(g)N
1 f
635(The)X
780(\257ag)X
920(to)X
1002(use)X
1129(to)X
1211(turn)X
1360 0.2500(error-checking)AX
1854(on)X
1954(from)X
2130(the)X
2248(start)X
2406(is)X
7 f
9 f
2479(-)X
7 f
2523(e)X
1 f
(.)S
10 f
555 1896(g)N
1 f
635(The)X
788(shell)X
967(can)X
1107(turn)X
1264 0.2500(error-checking)AX
1766(on)X
1874(and)X
2018(off,)X
2160(and)X
2304(the)X
2430(commands)X
2805(to)X
2895(do)X
3003(so)X
3102(are)X
7 f
3229(set)X
3429(+e)X
1 f
3553(and)X
7 f
3696(set)X
3895(-e)X
1 f
(,)S
635 1992(respectively.)N
755 2116(I)N
805(should)X
1041(note)X
1202(that)X
1345(this)X
1483(speci\256cation)X
1911(is)X
1987(for)X
2104(Bourne)X
2363(Shells)X
2581(that)X
2724(are)X
2846(not)X
2971(part)X
3119(of)X
3208(Berkeley)X
9 s
3518(UNIX)X
10 s
(,)S
3740(as)X
3829(shells)X
555 2212(from)N
741(Berkeley)X
1061(don't)X
1260(do)X
1370(error)X
1557(control.)X
1834(You)X
2002(can)X
2144(get)X
2272(a)X
2338(similar)X
2590(effect,)X
2824(however,)X
3151(by)X
3260(changing)X
3583(the)X
3710(last)X
3850(three)X
555 2308(lines)N
726(to)X
808(be:)X
7 f
1043 2452(hasErrCtl=no)N
1667(\\)X
1043 2548(check="echo)N
1619(\\"+)X
1811(%s\\"\\n")X
2195(\\)X
1043 2644(ignore="sh)N
1571(-c)X
1715('%s)X
1907(||)X
2051(exit)X
2291(0\\n")X
1 f
555 2816(This)N
717(will)X
861(cause)X
1060(PMake)X
1307(to)X
1389(execute)X
1655(the)X
1773(two)X
1913(commands)X
7 f
843 2960(echo)N
1083("+)X
2 f
1227(cmd)X
7 f
1361(")X
843 3056(sh)N
987(-c)X
1131(')X
2 f
(cmd)S
7 f
1361(||)X
1505(true')X
1 f
555 3200(for)N
675(each)X
849(command)X
1191(for)X
1311(which)X
1533(errors)X
1747(are)X
1872(to)X
1960(be)X
2062(ignored.)X
2352(\(In)X
2471(case)X
2635(you)X
2780(are)X
2904(wondering,)X
3292(the)X
3415(thing)X
3604(for)X
7 f
3723(ignore)X
1 f
555 3296(tells)N
714(the)X
838(shell)X
1014(to)X
1101(execute)X
1372(another)X
1638(shell)X
1814(without)X
2083(error)X
2265(checking)X
2580(on)X
2685(and)X
2826(always)X
3074(exit)X
3219(0,)X
3304(since)X
3494(the)X
3 f
3617(||)X
1 f
3678(causes)X
3913(the)X
7 f
555 3392(exit)N
802(0)X
1 f
877(to)X
966(be)X
1069(executed)X
1382(only)X
1551(if)X
1627(the)X
1751(\256rst)X
1901(command)X
2243(exited)X
2465(non-zero,)X
2797(and)X
2939(if)X
3014(the)X
3138(\256rst)X
3288(command)X
3630(exited)X
3852(zero,)X
555 3488(the)N
673(shell)X
844(will)X
988(also)X
1137(exit)X
1277(zero,)X
1456(since)X
1641(that's)X
1839(the)X
1957(last)X
2088(command)X
2424(it)X
2488(executed\).)X
3 f
555 3680(4.5.)N
715(Compatibility)X
1 f
755 3804(There)N
965(are)X
1086(three)X
1269(\(well,)X
1476(3)X
1538 MX
(12)130 833 oc
1605(\))X
1654(levels)X
1863(of)X
1952(backwards-compatibility)X
2771(built)X
2939(into)X
3085(PMake.)X
3374(Most)X
3559(make\256les)X
3887(will)X
555 3900(need)N
733(none)X
915(at)X
999(all.)X
1125(Some)X
1333(may)X
1497(need)X
1675(a)X
1737(little)X
1909(bit)X
2019(of)X
2112(work)X
2303(to)X
2391(operate)X
2654(correctly)X
2965(when)X
3164(run)X
3296(in)X
3383(parallel.)X
3669(Each)X
3855(level)X
555 3996(encompasses)N
1010(the)X
1144(previous)X
1456(levels)X
1679(\(e.g.)X
3 f
9 f
1878(-)X
3 f
1922(B)X
1 f
2011(\(one)X
2190(shell)X
2377(per)X
2516(command\))X
2895(implies)X
3 f
9 f
3166(-)X
3 f
3210(V)X
1 f
3268(\))X
3331(The)X
3492(three)X
3689(levels)X
3912(are)X
555 4092(described)N
883(in)X
965(the)X
1083(following)X
1414(three)X
1595(sections.)X
3 f
555 4284(4.5.1.)N
775(DEFCON)X
1133(3)X
1193(\320)X
1293(Variable)X
1611(Expansion)X
1 f
755 4408(As)N
869(noted)X
1072(before,)X
1323(PMake)X
1575(will)X
1724(not)X
1851(expand)X
2107(a)X
2167(variable)X
2450(unless)X
2674(it)X
2742(knows)X
2975(of)X
3066(a)X
3126(value)X
3324(for)X
3442(it.)X
3530(This)X
3696(can)X
3832(cause)X
555 4504(problems)N
885(for)X
1011(make\256les)X
1350(that)X
1502(expect)X
1744(to)X
1838(leave)X
2040(variables)X
2362(unde\256ned)X
2710(except)X
2952(in)X
3046(special)X
3301(circumstances)X
3788(\(e.g.)X
3962(if)X
555 4600(more)N
744(\257ags)X
919(need)X
1095(to)X
1181(be)X
1281(passed)X
1519(to)X
1605(the)X
1727(C)X
1804(compiler)X
2113(or)X
2204(the)X
2325(output)X
2552(from)X
2731(a)X
2790(text)X
2933(processor)X
3264(should)X
3500(be)X
3599(sent)X
3751(to)X
3836(a)X
3895(dif-)X
555 4696(ferent)N
775(printer\).)X
1068(If)X
1154(the)X
1284(variables)X
1606(are)X
1737(enclosed)X
2050(in)X
2144(curly)X
2341(braces)X
2579(\(``)X
7 f
2660(${PRINTER})X
1 f
(''\),)S
3273(the)X
3403(shell)X
3585(will)X
3740(let)X
3851(them)X
555 4792(pass.)N
739(If)X
819(they)X
982(are)X
1106(enclosed)X
1412(in)X
1499(parentheses,)X
1919(however,)X
2241(the)X
2364(shell)X
2540(will)X
2689(declare)X
2947(a)X
3008(syntax)X
3242(error)X
3424(and)X
3565(the)X
3688(make)X
3887(will)X
555 4888(come)N
749(to)X
831(a)X
887(grinding)X
1178(halt.)X
755 5012(You)N
915(have)X
1089(two)X
1231(choices:)X
1516(change)X
1765(the)X
1884(make\256le)X
2181(to)X
2264(de\256ne)X
2481(the)X
2600(variables)X
2911(\(their)X
3106(values)X
3332(can)X
3465(be)X
3562(overridden)X
3931(on)X
555 5108(the)N
676(command)X
1015(line,)X
1178(since)X
1366(that's)X
1567(where)X
1787(they)X
1948(would)X
2171(have)X
2346(been)X
2521(set)X
2633(if)X
2704(you)X
2846(used)X
3015(Make,)X
3240(anyway\))X
3539(or)X
3628(always)X
3873(give)X
555 5204(the)N
3 f
9 f
673(-)X
3 f
717(V)X
1 f
795(\257ag)X
935(\(this)X
1097(can)X
1229(be)X
1325(done)X
1501(with)X
1663(the)X
7 f
1781(.MAKEFLAGS)X
1 f
2281(target,)X
2504(if)X
2573(you)X
2713(want\).)X
3 f
555 5396(4.5.2.)N
775(DEFCON)X
1133(2)X
1193(\320)X
1293(The)X
1446(Number)X
1751(of)X
1838(the)X
1965(Beast)X
1 f
755 5520(Then)N
942(there)X
1125(are)X
1245(the)X
1364(make\256les)X
1692(that)X
1833(expect)X
2064(certain)X
2304(commands,)X
2692(such)X
2860(as)X
2948(changing)X
3263(to)X
3346(a)X
3403(different)X
3701(directory,)X
555 5616(to)N
638(not)X
761(affect)X
966(other)X
1152(commands)X
1520(in)X
1603(a)X
1660(target's)X
1922(creation)X
2202(script.)X
2421(You)X
2580(can)X
2713(solve)X
2903(this)X
3039(is)X
3113(either)X
3316(by)X
3416(going)X
3618(back)X
3790(to)X
3872(exe-)X
555 5712(cuting)N
783(one)X
927(shell)X
1106(per)X
1237(command)X
1581(\(which)X
1832(is)X
1913(what)X
2096(the)X
3 f
9 f
2221(-)X
3 f
2265(B)X
1 f
2345(\257ag)X
2492(forces)X
2716(PMake)X
2970(to)X
3059(do\),)X
3213(which)X
3436(slows)X
3645(the)X
3770(process)X
555 5808(down)N
768(a)X
839(good)X
1034(bit)X
1152(and)X
1302(requires)X
1595(you)X
1749(to)X
1845(use)X
1986(semicolons)X
2380(and)X
2530(escaped)X
2819(newlines)X
3138(for)X
3266(shell)X
3451(constructs,)X
3830(or)X
3931(by)X

32 p
%%Page: 32 33
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(32)X
2343(-)X
555 672(changing)N
887(the)X
1023(make\256le)X
1336(to)X
1435(execute)X
1718(the)X
1853(offending)X
2202(command\(s\))X
2640(in)X
2739(a)X
2812(subshell)X
3111(\(by)X
3255(placing)X
3528(the)X
3663(line)X
3820(inside)X
555 768(parentheses\),)N
997(like)X
1137(so:)X
7 f
843 912(install)N
1227(::)X
1371(.MAKE)X
1043 1008(\(cd)N
1235(src;)X
1475($\(.PMAKE\))X
1955(install\))X
1043 1104(\(cd)N
1235(lib;)X
1475($\(.PMAKE\))X
1955(install\))X
1043 1200(\(cd)N
1235(man;)X
1475($\(.PMAKE\))X
1955(install\))X
1 f
555 1344(This)N
719(will)X
865(always)X
1110(execute)X
1378(the)X
1498(three)X
1681(makes)X
1908(\(even)X
2109(if)X
2180(the)X
3 f
9 f
2299(-)X
3 f
2343(n)X
1 f
2408(\257ag)X
2549(was)X
2695(given\))X
2921(because)X
3197(of)X
3285(the)X
3404(combination)X
3825(of)X
3913(the)X
555 1440(``::'')N
732(operator)X
1025(and)X
1166(the)X
7 f
1288(.MAKE)X
1 f
1552(attribute.)X
1863(Each)X
2048(command)X
2388(will)X
2536(change)X
2788(to)X
2874(the)X
2996(proper)X
3230(directory)X
3544(to)X
3630(perform)X
3913(the)X
555 1536(install,)N
790(leaving)X
1046(the)X
1164(main)X
1344(shell)X
1515(in)X
1597(the)X
1715(directory)X
2025(in)X
2107(which)X
2323(it)X
2387(started.)X
3 f
555 1728(4.5.3.)N
775(DEFCON)X
1133(1)X
1193(\320)X
1293(Imitation)X
1633(is)X
1706(the)X
1833(Not)X
1978(the)X
2105(Highest)X
2387(Form)X
2599(of)X
2686(Flattery)X
1 f
755 1852(The)N
903(\256nal)X
1068(category)X
1368(of)X
1458(make\256le)X
1757(is)X
1833(the)X
1954(one)X
2092(where)X
2311(every)X
2512(command)X
2850(requires)X
3131(input,)X
3337(the)X
3457(dependencies)X
3912(are)X
555 1948(incompletely)N
995(speci\256ed,)X
1322(or)X
1411(you)X
1553(simply)X
1792(cannot)X
2028(create)X
2243(more)X
2430(than)X
2590(one)X
2727(target)X
2931(at)X
3010(a)X
3067(time,)X
3250(as)X
3338(mentioned)X
3697(earlier.)X
3944(In)X
555 2044(addition,)N
861(you)X
1005(may)X
1166(not)X
1291(have)X
1466(the)X
1587(time)X
1752(or)X
1842(desire)X
2057(to)X
2142(upgrade)X
2424(the)X
2545(make\256le)X
2844(to)X
2929(run)X
3059(smoothly)X
3379(with)X
3544(PMake.)X
3814(If)X
3891(you)X
555 2140(are)N
681(the)X
806(conservative)X
1239(sort,)X
1406(this)X
1548(is)X
1628(the)X
1753(compatibility)X
2206(mode)X
2411(for)X
2532(you.)X
2698(It)X
2773(is)X
2852(entered)X
3115(either)X
3324(by)X
3430(giving)X
3660(PMake)X
3913(the)X
3 f
9 f
555 2236(-)N
3 f
599(M)X
1 f
703(\257ag)X
851(\(for)X
1000(Make\),)X
1258(or)X
1353(by)X
1461(executing)X
1801(PMake)X
2055(as)X
2149(``)X
7 f
2203(make)X
1 f
(.'')S
2496(In)X
2590(either)X
2800(case,)X
2986(PMake)X
3240(performs)X
3557(things)X
3779(exactly)X
555 2332(like)N
695(Make)X
898(\(while)X
1123(still)X
1262(supporting)X
1624(most)X
1799(of)X
1886(the)X
2004(nice)X
2158(new)X
2312(features)X
2587(PMake)X
2834(provides\).)X
3177(This)X
3339(includes:)X
10 f
555 2456(g)N
1 f
635(No)X
753(parallel)X
1014(execution.)X
10 f
555 2580(g)N
1 f
635(Targets)X
903(are)X
1029(made)X
1230(in)X
1319(the)X
1444(exact)X
1641(order)X
1838(speci\256ed)X
2150(by)X
2257(the)X
2381(make\256le.)X
2703(The)X
2854(sources)X
3121(for)X
3241(each)X
3415(target)X
3624(are)X
3749(made)X
3949(in)X
635 2676(strict)N
815(left-to-right)X
1209(order,)X
1419(etc.)X
10 f
555 2800(g)N
1 f
635(A)X
715(single)X
928(Bourne)X
1185(shell)X
1357(is)X
1431(used)X
1599(to)X
1682(execute)X
1949(each)X
2118(command,)X
2475(thus)X
2629(the)X
2748(shell's)X
7 f
2978($$)X
1 f
3095(variable)X
3375(is)X
3449(useless,)X
3717(changing)X
635 2896(directories)N
994(doesn't)X
1250(work)X
1435(across)X
1656(command)X
1992(lines,)X
2183(etc.)X
10 f
555 3020(g)N
1 f
635(If)X
716(no)X
823(special)X
1073(characters)X
1427(exist)X
1605(in)X
1693(a)X
1755(command)X
2097(line,)X
2263(PMake)X
2516(will)X
2666(break)X
2871(the)X
2995(command)X
3337(into)X
3487(words)X
3709(itself)X
3895(and)X
635 3116(execute)N
907(the)X
1031(command)X
1373(directly,)X
1664(without)X
1934(executing)X
2272(a)X
2334(shell)X
2511(\256rst.)X
2681(The)X
2832(characters)X
3184(that)X
3329(cause)X
3533(PMake)X
3785(to)X
3872(exe-)X
635 3212(cute)N
790(a)X
847(shell)X
1019(are:)X
7 f
1161(#)X
1 f
(,)S
7 f
1250(=)X
1 f
(,)S
7 f
1339(|)X
1 f
(,)S
7 f
1428(\303)X
1 f
(,)S
7 f
1517(\()X
1 f
(,)S
7 f
1606(\))X
1 f
(,)S
7 f
1695({)X
1 f
(,)S
7 f
1784(})X
1 f
(,)S
7 f
1873(;)X
1 f
(,)S
7 f
1962(&)X
1 f
(,)S
7 f
2051(<)X
1 f
(,)S
7 f
2140(>)X
1 f
(,)S
7 f
2229(*)X
1 f
(,)S
7 f
2318(?)X
1 f
(,)S
7 f
2407([)X
1 f
(,)S
7 f
2496(])X
1 f
(,)S
7 f
2585(:)X
1 f
(,)S
7 f
2674($)X
1 f
(,)S
7 f
2763(`)X
1 f
(,)S
2852(and)X
7 f
2989(\\)X
1 f
(.)S
3098(You)X
3257(should)X
3490(notice)X
3706(that)X
3846(these)X
635 3308(are)N
760(all)X
866(the)X
990(characters)X
1343(that)X
1489(are)X
1614(given)X
1818(special)X
2066(meaning)X
2367(by)X
2472(the)X
2595(shell)X
2771(\(except)X
7 f
3033(')X
1 f
3106(and)X
7 f
3300(,)X
1 f
3373(which)X
3594(PMake)X
3846(deals)X
635 3404(with)N
797(all)X
897(by)X
997(its)X
1092(lonesome\).)X
10 f
555 3528(g)N
1 f
635(The)X
780(use)X
907(of)X
994(the)X
1112(null)X
1256(suf\256x)X
1458(is)X
1531(turned)X
1756(off.)X
3 f
555 3720(4.6.)N
715(The)X
868(Way)X
1048(Things)X
1302(Work)X
1 f
755 3844(When)N
976(PMake)X
1232(reads)X
1431(the)X
1558(make\256le,)X
1883(it)X
1956(parses)X
2186(sources)X
2456(and)X
2600(targets)X
2842(into)X
2994(nodes)X
3209(in)X
3299(a)X
3363(graph.)X
3594(The)X
3747(graph)X
3958(is)X
555 3940(directed)N
839(only)X
1006(in)X
1092(the)X
1214(sense)X
1412(that)X
1556(PMake)X
1807(knows)X
2040(which)X
2260(way)X
2418(is)X
2495(up.)X
2619(Each)X
2804(node)X
2984(contains)X
3275(not)X
3401(only)X
3567(links)X
3746(to)X
3832(all)X
3936(its)X
555 4036(parents)N
813(and)X
955(children)X
1244(\(the)X
1395(nodes)X
1608(that)X
1754(depend)X
2012(on)X
2118(it)X
2188(and)X
2330(those)X
2525(on)X
2631(which)X
2853(it)X
2923(depends,)X
3232(respectively\),)X
3693(but)X
3821(also)X
3975(a)X
555 4132(count)N
753(of)X
840(the)X
958(number)X
1223(of)X
1310(its)X
1405(children)X
1688(that)X
1828(have)X
2000(already)X
2257(been)X
2429(processed.)X
755 4256(The)N
905(most)X
1085(important)X
1421(thing)X
1610(to)X
1697(know)X
1899(about)X
2101(how)X
2263(PMake)X
2514(uses)X
2676(this)X
2815(graph)X
3022(is)X
3099(that)X
3243(the)X
3365(traversal)X
3666(is)X
3743(breadth-)X
555 4352(\256rst)N
699(and)X
835(occurs)X
1065(in)X
1147(two)X
1287(passes.)X
755 4476(After)N
948(PMake)X
1198(has)X
1328(parsed)X
1561(the)X
1682(make\256le,)X
2001(it)X
2068(begins)X
2300(with)X
2465(the)X
2586(nodes)X
2796(the)X
2917(user)X
3074(has)X
3204(told)X
3351(it)X
3418(to)X
3503(make)X
3699(\(either)X
3931(on)X
555 4572(the)N
682(command)X
1027(line,)X
1196(or)X
1292(via)X
1419(a)X
7 f
1484(.MAIN)X
1 f
1752(target,)X
1983(or)X
2078(by)X
2186(the)X
2312(target)X
2523(being)X
2729(the)X
2855(\256rst)X
3007(in)X
3097(the)X
3223(\256le)X
3353(not)X
3483(labeled)X
3743(with)X
3913(the)X
7 f
555 4668(.NOTMAIN)N
1 f
962(attribute\))X
1279(placed)X
1512(in)X
1597(a)X
1656(queue.)X
1891(It)X
1963(continues)X
2293(to)X
2378(take)X
2535(the)X
2656(node)X
2834(off)X
2950(the)X
3070(front)X
3248(of)X
3337(the)X
3457(queue,)X
3691(mark)X
3878(it)X
3944(as)X
555 4764(something)N
922(that)X
1076(needs)X
1293(to)X
1389(be)X
1499(made,)X
1727(pass)X
1899(the)X
2031(node)X
2221(to)X
7 f
2317(Suff_FindDeps)X
1 f
2975(\(mentioned)X
3374(earlier\))X
3641(to)X
3737(\256nd)X
3895(any)X
555 4860(implicit)N
826(sources)X
1090(for)X
1207(the)X
1328(node,)X
1527(and)X
1666(place)X
1859(all)X
1962(the)X
2083(node's)X
2320(children)X
2605(that)X
2747(have)X
2921(yet)X
3041(to)X
3125(be)X
3223(marked)X
3486(at)X
3566(the)X
3686(end)X
3824(of)X
3913(the)X
555 4956(queue.)N
791(If)X
869(any)X
1009(of)X
1100(the)X
1222(children)X
1509(is)X
1586(a)X
7 f
1646(.USE)X
1 f
1862(rule,)X
2031(its)X
2130(attributes)X
2452(are)X
2574(applied)X
2833(to)X
2918(the)X
3039(parent,)X
3283(then)X
3444(its)X
3542(commands)X
3912(are)X
555 5052(appended)N
891(to)X
981(the)X
1107(parent's)X
1394(list)X
1519(of)X
1614(commands)X
1989(and)X
2133(its)X
2236(children)X
2527(are)X
2653(linked)X
2880(to)X
2969(its)X
3071(parent.)X
3319(The)X
3471(parent's)X
3757(unmade)X
555 5148(children)N
847(counter)X
1117(is)X
1199(then)X
1366(decremented)X
1806(\(since)X
2027(the)X
7 f
2153(.USE)X
1 f
2373(node)X
2557(has)X
2692(been)X
2872(processed\).)X
3264(You)X
3430(will)X
3582(note)X
3748(that)X
3896(this)X
555 5244(allows)N
786(a)X
7 f
844(.USE)X
1 f
1058(node)X
1236(to)X
1320(have)X
1494(children)X
1779(that)X
1921(are)X
7 f
2042(.USE)X
1 f
2256(nodes)X
2465(and)X
2603(the)X
2723(rules)X
2900(will)X
3045(be)X
3142(applied)X
3399(in)X
3482(sequence.)X
3838(If)X
3913(the)X
555 5340(node)N
735(has)X
865(no)X
968(children,)X
1274(it)X
1341(is)X
1417(placed)X
1650(at)X
1731(the)X
1852(end)X
1991(of)X
2081(another)X
2345(queue)X
2560(to)X
2645(be)X
2744(examined)X
3079(in)X
3164(the)X
3285(second)X
3531(pass.)X
3712(This)X
3877(pro-)X
555 5436(cess)N
709(continues)X
1036(until)X
1202(the)X
1320(\256rst)X
1464(queue)X
1676(is)X
1749(empty.)X
755 5560(At)N
862(this)X
1004(point,)X
1215(all)X
1322(the)X
1446(leaves)X
1673(of)X
1766(the)X
1890(graph)X
2099(are)X
2224(in)X
2312(the)X
2436(examination)X
2858(queue.)X
3096(PMake)X
3349(removes)X
3647(the)X
3771(node)X
3953(at)X
555 5656(the)N
681(head)X
861(of)X
956(the)X
1082(queue)X
1302(and)X
1446(sees)X
1608(if)X
1685(it)X
1757(is)X
1838(out-of-date.)X
2243(If)X
2325(it)X
2397(is,)X
2498(it)X
2570(is)X
2651(passed)X
2893(to)X
2983(a)X
3047(function)X
3342(that)X
3489(will)X
3640(execute)X
3913(the)X
555 5752(commands)N
925(for)X
1042(the)X
1163(node)X
1342(asynchronously.)X
1888(When)X
2103(the)X
2224(commands)X
2594(have)X
2769(completed,)X
3146(all)X
3249(the)X
3369(node's)X
3605(parents)X
3859(have)X
555 5848(their)N
723(unmade)X
998(children)X
1282(counter)X
1544(decremented)X
1975(and,)X
2131(if)X
2200(the)X
2318(counter)X
2579(is)X
2652(then)X
2810(0,)X
2890(they)X
3048(are)X
3167(placed)X
3397(on)X
3497(the)X
3615(examination)X

33 p
%%Page: 33 34
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(33)X
2343(-)X
555 672(queue.)N
790(Likewise,)X
1127(if)X
1199(the)X
1320(node)X
1499(is)X
1575(up-to-date.)X
1948(Only)X
2131(those)X
2323(parents)X
2578(that)X
2721(were)X
2901(marked)X
3165(on)X
3268(the)X
3389(downward)X
3751(pass)X
3912(are)X
555 768(processed)N
894(in)X
978(this)X
1115(way.)X
1291(Thus)X
1473(PMake)X
1722(traverses)X
2030(the)X
2150(graph)X
2355(back)X
2528(up)X
2629(to)X
2712(the)X
2831(nodes)X
3039(the)X
3158(user)X
3313(instructed)X
3650(it)X
3715(to)X
3798(create.)X
555 864(When)N
767(the)X
885(examination)X
1301(queue)X
1513(is)X
1586(empty)X
1806(and)X
1942(no)X
2042(shells)X
2244(are)X
2363(running)X
2632(to)X
2714(create)X
2927(a)X
2983(target,)X
3206(PMake)X
3453(is)X
3526(\256nished.)X
755 988(Once)N
953(all)X
1060(targets)X
1301(have)X
1480(been)X
1659(processed,)X
2023(PMake)X
2277(executes)X
2581(the)X
2706(commands)X
3080(attached)X
3375(to)X
3464(the)X
7 f
3589(.END)X
1 f
3808(target,)X
555 1084(either)N
764(explicitly)X
1092(or)X
1185(through)X
1460(the)X
1583(use)X
1715(of)X
1807(an)X
1908(ellipsis)X
2159(in)X
2246(a)X
2307(shell)X
2483(script.)X
2706(If)X
2785(there)X
2971(were)X
3153(no)X
3258(errors)X
3471(during)X
3705(the)X
3828(entire)X
555 1180(process)N
818(but)X
942(there)X
1125(are)X
1246(still)X
1387(some)X
1578(targets)X
1814(unmade)X
2090(\(PMake)X
2366(keeps)X
2571(a)X
2629(running)X
2900(count)X
3100(of)X
3189(how)X
3349(many)X
3549(targets)X
3784(are)X
3904(left)X
555 1276(to)N
639(be)X
737(made\),)X
980(there)X
1163(is)X
1238(a)X
1296(cycle)X
1488(in)X
1572(the)X
1692(graph.)X
1917(PMake)X
2166(does)X
2335(a)X
2392(depth-\256rst)X
2742(traversal)X
3040(of)X
3128(the)X
3247(graph)X
3451(to)X
3534(\256nd)X
3679(all)X
3780(the)X
3899(tar-)X
555 1372(gets)N
704(that)X
844(weren't)X
1110(made)X
1304(and)X
1440(prints)X
1642(them)X
1822(out)X
1944(one)X
2080(by)X
2180(one.)X
3 f
555 1564(5.)N
655(Answers)X
969(to)X
1056(Exercises)X
1 f
555 1688(\(3.1\))N
755(This)X
935(is)X
1026(something)X
1397(of)X
1502(a)X
1576(trick)X
1761(question,)X
2090(for)X
2222(which)X
2456(I)X
2521(apologize.)X
2891(The)X
3054(trick)X
3239(comes)X
3482(from)X
3675(the)X
3810(UNIX)X
755 1784(de\256nition)N
1094(of)X
1194(a)X
1263(suf\256x,)X
1498(which)X
1727(PMake)X
1987(doesn't)X
2256(necessarily)X
2646(share.)X
2869(You)X
3040(will)X
3197(have)X
3381(noticed)X
3649(that)X
3801(all)X
3913(the)X
755 1880(suf\256xes)N
1035(used)X
1213(in)X
1306(this)X
1452(tutorial)X
1714(\(and)X
1887(in)X
1979(UNIX)X
2210(in)X
2302(general\))X
2596(begin)X
2804(with)X
2976(a)X
3042(period)X
3277(\()X
7 f
3304(.ms)X
1 f
(,)S
7 f
3498(.c)X
1 f
(,)S
3644(etc.\).)X
3835(Now,)X
755 1976(PMake's)N
1064(idea)X
1222(of)X
1313(a)X
1373(suf\256x)X
1579(is)X
1656(more)X
1844(like)X
1987(English's:)X
2334(it's)X
2459(the)X
2580(characters)X
2930(at)X
3011(the)X
3132(end)X
3271(of)X
3361(a)X
3420(word.)X
3628(With)X
3811(this)X
3949(in)X
755 2072(mind,)N
959(one)X
1095(possible)X
1377(solution)X
1654(to)X
1736(this)X
1871(problem)X
2158(goes)X
2325(as)X
2412(follows:)X
7 f
1043 2216(.SUFFIXES)N
1811(:)X
1907(ec.exe)X
2243(.exe)X
2483(ec.obj)X
2819(.obj)X
3059(.asm)X
1043 2312(ec.objec.exe)N
1667(.obj.exe)X
2099(:)X
1427 2408(link)N
1667(-o)X
1811($\(.TARGET\))X
2339($\(.IMPSRC\))X
1043 2504(.asmec.obj)N
1811(:)X
1427 2600(asm)N
1619(-o)X
1763($\(.TARGET\))X
2291(-DDO_ERROR_CHECKING)X
3251($\(.IMPSRC\))X
1043 2696(.asm.obj)N
1811(:)X
1427 2792(asm)N
1619(-o)X
1763($\(.TARGET\))X
2291($\(.IMPSRC\))X
1 f
555 2964(\(3.2\))N
755(The)X
904(trick)X
1075(to)X
1161(this)X
1300(one)X
1440(lies)X
1575(in)X
1661(the)X
1783(``:='')X
1982(variable-assignment)X
2652(operator)X
2943(and)X
3082(the)X
3203(``:S'')X
3400(variable-expansion)X
755 3060(modi\256er.)N
1067(Basically)X
1386(what)X
1563(you)X
1704(want)X
1881(is)X
1955(to)X
2038(take)X
2193(the)X
2312(pointer)X
2560(variable,)X
2860(so)X
2952(to)X
3035(speak,)X
3259(and)X
3395(transform)X
3727(it)X
3791(into)X
3935(an)X
755 3156(invocation)N
1113(of)X
1200(the)X
1318(variable)X
1597(at)X
1675(which)X
1891(it)X
1955(points.)X
2190(You)X
2348(might)X
2554(try)X
2663(something)X
3016(like)X
7 f
1043 3300($\(PTR:S/\303/\\$\(/:S/$/\)\))N
1 f
755 3444(which)N
973(places)X
1196(``)X
7 f
1250($\()X
1 f
('')S
1422(at)X
1502(the)X
1622(front)X
1799(of)X
1887(the)X
2006(variable)X
2286(name)X
2481(and)X
2618(``)X
7 f
2672(\))X
1 f
('')S
2795(at)X
2874(the)X
2993(end,)X
3150(thus)X
3304(transforming)X
3739(``)X
7 f
3793(VAR)X
1 f
(,'')S
755 3540(for)N
878(example,)X
1199(into)X
1352(``)X
7 f
1406($\(VAR\))X
1 f
(,'')S
1797(which)X
2022(is)X
2104(just)X
2247(what)X
2431(we)X
2553(want.)X
2757(Unfortunately)X
3235(\(as)X
3357(you)X
3505(know)X
3711(if)X
3788(you've)X
755 3636(tried)N
928(it\),)X
1045(since,)X
1256(as)X
1349(it)X
1419(says)X
1583(in)X
1671(the)X
1795(hint,)X
1965(PMake)X
2218(does)X
2391(no)X
2497(further)X
2742(substitution)X
3140(on)X
3246(the)X
3370(result)X
3574(of)X
3666(a)X
3727(modi\256ed)X
755 3732(expansion,)N
1121(that's)X
2 f
1320(all)X
1 f
1425(you)X
1566(get.)X
1705(The)X
1851(solution)X
2129(is)X
2203(to)X
2286(make)X
2481(use)X
2609(of)X
2697(``:='')X
2893(to)X
2976(place)X
3166(that)X
3306(string)X
3508(into)X
3652(yet)X
3770(another)X
755 3828(variable,)N
1054(then)X
1212(invoke)X
1450(the)X
1568(other)X
1753(variable)X
2032(directly:)X
7 f
1043 3972(*PTR)N
1811(:=)X
1955($\(PTR:S/\303/\\$\(/:S/$/\)/\))X
1 f
755 4116(You)N
913(can)X
1045(then)X
1203(use)X
1330(``)X
7 f
1384($\(*PTR\))X
1 f
('')S
1794(to)X
1876(your)X
2043(heart's)X
2282(content.)X
3 f
555 4308(6.)N
655(Glossary)X
977(of)X
1064(Jargon)X
555 4432(attribute:)N
1 f
905(A)X
983(property)X
1275(given)X
1473(to)X
1555(a)X
1611(target)X
1814(that)X
1954(causes)X
2184(PMake)X
2431(to)X
2513(treat)X
2676(it)X
2740(differently.)X
3 f
555 4556(command)N
915(script:)X
1 f
1160(The)X
1307(lines)X
1480(immediately)X
1902(following)X
2234(a)X
2291(dependency)X
2696(line)X
2837(that)X
2978(specify)X
3231(commands)X
3599(to)X
3682(execute)X
3949(to)X
755 4652(create)N
970(each)X
1140(of)X
1229(the)X
1349(targets)X
1585(on)X
1687(the)X
1807(dependency)X
2213(line.)X
2375(Each)X
2558(line)X
2699(in)X
2782(the)X
2901(command)X
3238(script)X
3437(must)X
3613(begin)X
3812(with)X
3975(a)X
755 4748(tab.)N
3 f
555 4872(command-line)N
1071(variable:)X
1 f
1404(A)X
1488(variable)X
1773(de\256ned)X
2035(in)X
2123(an)X
2225(argument)X
2554(when)X
2754(PMake)X
3007(is)X
3086(\256rst)X
3236(executed.)X
3588(Overrides)X
3931(all)X
755 4968(assignments)N
1166(to)X
1248(the)X
1366(same)X
1551(variable)X
1830(name)X
2024(in)X
2106(the)X
2224(make\256le.)X
3 f
555 5092(conditional:)N
1 f
987(A)X
1069(construct)X
1387(much)X
1589(like)X
1733(that)X
1877(used)X
2048(in)X
2134(C)X
2211(that)X
2355(allows)X
2588(a)X
2648(make\256le)X
2947(to)X
3032(be)X
3131(con\256gured)X
3497(on)X
3600(the)X
3721(\257y)X
3828(based)X
755 5188(on)N
855(the)X
973(local)X
1149(environment,)X
1594(or)X
1681(on)X
1781(what)X
1957(is)X
2030(being)X
2228(made)X
2422(by)X
2522(that)X
2662(invocation)X
3020(of)X
3107(PMake.)X
3 f
555 5312(creation)N
856(script:)X
1 f
1099(Commands)X
1483(used)X
1650(to)X
1732(create)X
1945(a)X
2001(target.)X
2224(See)X
2360(``command)X
2750(script.'')X
3 f
555 5436(dependency:)N
1 f
1012(The)X
1163(relationship)X
1567(between)X
1861(a)X
1922(source)X
2157(and)X
2298(a)X
2359(target.)X
2587(This)X
2754(comes)X
2984(in)X
3071(three)X
3257(\257avors,)X
3520(as)X
3612(indicated)X
3931(by)X
755 5532(the)N
878(operator)X
1171(between)X
1464(the)X
1587(target)X
1795(and)X
1936(the)X
2058(source.)X
2312(`:')X
2412(gives)X
2605(a)X
2665(straight)X
2929(time-wise)X
3269(dependency)X
3677(\(if)X
3777(the)X
3899(tar-)X
755 5628(get)N
886(is)X
972(older)X
1170(than)X
1341(the)X
1472(source,)X
1735(the)X
1866(target)X
2082(is)X
2167(out-of-date\),)X
2603(while)X
2813(`!')X
2926(provides)X
3234(simply)X
3483(an)X
3591(ordering)X
3895(and)X
755 5724(always)N
999(considers)X
1322(the)X
1440(target)X
1643(out-of-date.)X
2040(`::')X
2158(is)X
2231(much)X
2429(like)X
2569(`:',)X
2685(save)X
2848(it)X
2912(creates)X
3156(multiple)X
3442(instances)X
3756(of)X
3843(a)X
3899(tar-)X
755 5820(get)N
873(each)X
1041(of)X
1128(which)X
1344(depends)X
1627(on)X
1727(its)X
1822(own)X
1980(list)X
2097(of)X
2184(sources.)X

34 p
%%Page: 34 35
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(34)X
2343(-)X
3 f
555 672(dynamic)N
871(source:)X
1 f
1144(This)X
1309(refers)X
1516(to)X
1601(a)X
1660(source)X
1893(that)X
2036(has)X
2166(a)X
2225(local)X
2404(variable)X
2686(invocation)X
3047(in)X
3132(it.)X
3219(It)X
3290(allows)X
3521(a)X
3579(single)X
3792(depen-)X
755 768(dency)N
967(line)X
1107(to)X
1189(specify)X
1441(a)X
1497(different)X
1794(source)X
2024(for)X
2138(each)X
2306(target)X
2509(on)X
2609(the)X
2727(line.)X
3 f
555 892(global)N
801(variable:)X
1 f
1146(Any)X
1322(variable)X
1619(de\256ned)X
1893(in)X
1993(a)X
2067(make\256le.)X
2401(Takes)X
2631 0.3750(precedence)AX
3032(over)X
3213(variables)X
3541(de\256ned)X
3814(in)X
3913(the)X
755 988(environment,)N
1200(but)X
1322(not)X
1444(over)X
1607(command-line)X
2090(or)X
2177(local)X
2353(variables.)X
3 f
555 1112(input)N
774(graph:)X
1 f
1042(What)X
1253(PMake)X
1517(constructs)X
1879(from)X
2072(a)X
2145(make\256le.)X
2478(Consists)X
2785(of)X
2889(nodes)X
3113(made)X
3324(of)X
3428(the)X
3563(targets)X
3814(in)X
3913(the)X
755 1208(make\256le,)N
1075(and)X
1215(the)X
1337(links)X
1516(between)X
1808(them)X
1992(\(the)X
2141(dependencies\).)X
2645(The)X
2793(links)X
2971(are)X
3093(directed)X
3375(\(from)X
3581(source)X
3814(to)X
3899(tar-)X
755 1304(get\))N
900(and)X
1036(there)X
1217(may)X
1375(not)X
1497(be)X
1593(any)X
1729(cycles)X
1950(\(loops\))X
2197(in)X
2279(the)X
2397(graph.)X
3 f
555 1428(local)N
741(variable:)X
1 f
1074(A)X
1158(variable)X
1443(de\256ned)X
1705(by)X
1811(PMake)X
2064(visible)X
2303(only)X
2471(in)X
2559(a)X
2621(target's)X
2888(shell)X
3065(script.)X
3309(There)X
3523(are)X
3647(seven)X
3855(local)X
755 1524(variables,)N
1086(not)X
1209(all)X
1310(of)X
1398(which)X
1614(are)X
1733(de\256ned)X
1989(for)X
2103(every)X
2302(target:)X
7 f
2527(.TARGET)X
1 f
(,)S
7 f
2903(.ALLSRC)X
1 f
(,)S
7 f
3279(.OODATE)X
1 f
(,)S
7 f
3655(.PREFIX)X
1 f
(,)S
7 f
755 1620(.IMPSRC)N
1 f
(,)S
7 f
1134(.ARCHIVE)X
1 f
(,)S
1561(and)X
7 f
1699(.MEMBER)X
1 f
(.)S
7 f
2097(.TARGET)X
1 f
(,)S
7 f
2475(.PREFIX)X
1 f
(,)S
7 f
2853(.ARCHIVE)X
1 f
(,)S
3279(and)X
7 f
3417(.MEMBER)X
1 f
3775(may)X
3935(be)X
755 1716(used)N
922(on)X
1022(dependency)X
1426(lines)X
1597(to)X
1679(create)X
1892(``dynamic)X
2242(sources.'')X
3 f
555 1840(make\256le:)N
1 f
907(A)X
1001(\256le)X
1139(that)X
1294(describes)X
1628(how)X
1801(a)X
1872(system)X
2129(is)X
2217(built.)X
2418(If)X
2507(you)X
2662(don't)X
2866(know)X
3079(what)X
3270(it)X
3349(is)X
3437(after)X
3620(reading)X
3896(this)X
755 1936(tutorial.)N
1019(.)X
1052(.)X
1085(.)X
3 f
555 2060(modi\256er:)N
1 f
892(A)X
971(letter,)X
1177(following)X
1509(a)X
1566(colon,)X
1785(used)X
1953(to)X
2036(alter)X
2200(how)X
2359(a)X
2416(variable)X
2696(is)X
2770(expanded.)X
3139(It)X
3209(has)X
3337(no)X
3437(effect)X
3641(on)X
3741(the)X
3859(vari-)X
755 2156(able)N
909(itself.)X
3 f
555 2280(operator:)N
1 f
915(What)X
1123(separates)X
1452(a)X
1521(source)X
1764(from)X
1953(a)X
2022(target)X
2238(\(on)X
2378(a)X
2447(dependency)X
2864(line\))X
3044(and)X
3193(speci\256es)X
3502(the)X
3633(relationship)X
755 2376(between)N
1043(the)X
1161(two.)X
1321(There)X
1529(are)X
1648(three:)X
1851(`)X
7 f
1878(:)X
1 f
(',)S
1993(`)X
7 f
2020(::)X
1 f
(',)S
2183(and)X
2319(`)X
7 f
2346(!)X
1 f
('.)S
3 f
555 2500(search)N
802(path:)X
1 f
1008(A)X
1090(list)X
1211(of)X
1302(directories)X
1665(in)X
1751(which)X
1970(a)X
2029(\256le)X
2154(should)X
2390(be)X
2489(sought.)X
2745(PMake's)X
3053(view)X
3232(of)X
3322(the)X
3443(contents)X
3733(of)X
3823(direc-)X
755 2596(tories)N
957(in)X
1043(a)X
1103(search)X
1333(path)X
1495(does)X
1666(not)X
1792(change)X
2043(once)X
2218(the)X
2339(make\256le)X
2638(has)X
2768(been)X
2943(read.)X
3125(A)X
3206(\256le)X
3331(is)X
3407(sought)X
3643(on)X
3746(a)X
3805(search)X
755 2692(path)N
913(only)X
1075(if)X
1144(it)X
1208(is)X
1281(exclusively)X
1666(a)X
1722(source.)X
3 f
555 2816(shell:)N
1 f
757(A)X
835(program)X
1127(to)X
1209(which)X
1425(commands)X
1792(are)X
1911(passed)X
2145(in)X
2227(order)X
2417(to)X
2499(create)X
2712(targets.)X
3 f
555 2940(source:)N
1 f
827(Anything)X
1151(to)X
1235(the)X
1355(right)X
1527(of)X
1615(an)X
1712(operator)X
2001(on)X
2102(a)X
2159(dependency)X
2564(line.)X
2725(Targets)X
2987(on)X
3088(the)X
3207(dependency)X
3612(line)X
3753(are)X
3873(usu-)X
755 3036(ally)N
895(created)X
1148(from)X
1324(the)X
1442(sources.)X
3 f
555 3160(special)N
806(target:)X
1 f
1059(A)X
1137(target)X
1340(that)X
1480(causes)X
1710(PMake)X
1957(to)X
2039(do)X
2139(special)X
2382(things)X
2597(when)X
2791(it's)X
2913(encountered.)X
3 f
555 3284(suf\256x:)N
1 f
788(The)X
933(tail)X
1055(end)X
1191(of)X
1278(a)X
1334(\256le)X
1456(name.)X
1670(Usually)X
1939(begins)X
2168(with)X
2330(a)X
2386(period,)X
7 f
2631(.c)X
1 f
2747(or)X
7 f
2834(.ms)X
1 f
(,)S
3018(e.g.)X
3 f
555 3408(target:)N
1 f
810(A)X
890(word)X
1077(to)X
1161(the)X
1281(left)X
1410(of)X
1499(the)X
1619(operator)X
1909(on)X
2011(a)X
2069(dependency)X
2475(line.)X
2637(More)X
2833(generally,)X
3174(any)X
3312(\256le)X
3436(that)X
3577(PMake)X
3825(might)X
755 3504(create.)N
993(A)X
1076(\256le)X
1203(may)X
1366(be)X
1467(\(and)X
1635(often)X
1825(is\))X
1930(both)X
2097(a)X
2158(target)X
2366(and)X
2507(a)X
2568(source)X
2802(\(what)X
3009(it)X
3077(is)X
3154(depends)X
3441(on)X
3545(how)X
3707(PMake)X
3958(is)X
755 3600(looking)N
1019(at)X
1097(it)X
1161(at)X
1239(the)X
1357(time)X
1519(\320)X
1619(sort)X
1759(of)X
1846(like)X
1986(the)X
2104(wave/particle)X
2557(duality)X
2799(of)X
2886(light,)X
3072(you)X
3212(know\).)X
3 f
555 3724(transformation)N
1097(rule:)X
1 f
1282(A)X
1360(special)X
1603(construct)X
1917(in)X
1999(a)X
2055(make\256le)X
2351(that)X
2491(speci\256es)X
2787(how)X
2945(to)X
3027(create)X
3240(a)X
3296(\256le)X
3418(of)X
3505(one)X
3641(type)X
3799(from)X
3975(a)X
755 3820(\256le)N
877(of)X
964(another,)X
1245(as)X
1332(indicated)X
1646(by)X
1746(their)X
1913(suf\256xes.)X
3 f
555 3944(variable)N
865(expansion:)X
1 f
1263(The)X
1418(process)X
1689(of)X
1786(substituting)X
2188(the)X
2316(value)X
2520(of)X
2617(a)X
2683(variable)X
2971(for)X
3094(a)X
3159 0.4531(reference)AX
3489(to)X
3580(it.)X
3673(Expansion)X
755 4040(may)N
913(be)X
1009(altered)X
1248(by)X
1348(means)X
1573(of)X
1660(modi\256ers.)X
3 f
555 4164(variable:)N
1 f
887(A)X
970(place)X
1165(in)X
1252(which)X
1472(to)X
1558(store)X
1738(text)X
1882(that)X
2026(may)X
2188(be)X
2288(retrieved)X
2598(later.)X
2785(Also)X
2960(used)X
3131(to)X
3217(de\256ne)X
3437(the)X
3559(local)X
3739(environ-)X
755 4260(ment.)N
955(Conditionals)X
1383(exist)X
1554(that)X
1694(test)X
1825(whether)X
2104(a)X
2160(variable)X
2439(is)X
2512(de\256ned)X
2768(or)X
2855(not.)X

1 p
%%Page: 1 36
10 s 10 xH 0 xS 1 f
3 f
12 s
1918 960(Table)N
2177(of)X
2281(Contents)X
1 f
10 s
555 1372(1.)N
635(Introduction)X
1051(..........................................................................................................................................)X
3971(1)X
555 1496(2.)N
635(The)X
780(Basics)X
1009(of)X
1096(PMake)X
1351(...........................................................................................................................)X
3971(1)X
755 1592(2.1.)N
895(Dependency)X
1317(Lines)X
1531(..................................................................................................................)X
3971(2)X
755 1688(2.2.)N
895(Shell)X
1079(Commands)X
1471(.....................................................................................................................)X
3971(3)X
755 1784(2.3.)N
895(Variables)X
1231(.................................................................................................................................)X
3971(4)X
955 1880(2.3.1.)N
1155(Local)X
1358(Variables)X
1691(..........................................................................................................)X
3971(6)X
955 1976(2.3.2.)N
1155(Command-line)X
1655(Variables)X
1991(...........................................................................................)X
3971(6)X
955 2072(2.3.3.)N
1155(Global)X
1393(Variables)X
1731(........................................................................................................)X
3971(6)X
955 2168(2.3.4.)N
1155(Environment)X
1593(Variables)X
1931(..............................................................................................)X
3971(7)X
755 2264(2.4.)N
895(Comments)X
1271(...............................................................................................................................)X
3971(7)X
755 2360(2.5.)N
895(Parallelism)X
1291(..............................................................................................................................)X
3971(7)X
755 2456(2.6.)N
895(Writing)X
1164(and)X
1300(Debugging)X
1676(a)X
1732(Make\256le)X
2051(........................................................................................)X
3971(8)X
755 2552(2.7.)N
895(Invoking)X
1204(PMake)X
1451(......................................................................................................................)X
3931(10)X
755 2648(2.8.)N
895(Summary)X
1231(.................................................................................................................................)X
3931(12)X
755 2744(2.9.)N
895(Exercises)X
1231(.................................................................................................................................)X
3931(13)X
555 2868(3.)N
635(Short-cuts)X
984(and)X
1120(Other)X
1323(Nice)X
1495(Things)X
1751(.......................................................................................................)X
3931(13)X
755 2964(3.1.)N
895(Transformation)X
1414(Rules)X
1631(.............................................................................................................)X
3931(13)X
755 3060(3.2.)N
895(Including)X
1222(Other)X
1425(Make\256les)X
1771(......................................................................................................)X
3931(17)X
755 3156(3.3.)N
895(Saving)X
1137(Commands)X
1531(..................................................................................................................)X
3931(17)X
755 3252(3.4.)N
895(Target)X
1125(Attributes)X
1471(.....................................................................................................................)X
3931(18)X
755 3348(3.5.)N
895(Special)X
1151(Targets)X
1431(.......................................................................................................................)X
3931(21)X
755 3444(3.6.)N
895(Modifying)X
1257(Variable)X
1554(Expansion)X
1931(..............................................................................................)X
3931(22)X
755 3540(3.7.)N
895(More)X
1089(on)X
1189(Debugging)X
1571(................................................................................................................)X
3931(24)X
755 3636(3.8.)N
895(More)X
1089(Exercises)X
1431(.......................................................................................................................)X
3931(24)X
555 3760(4.)N
635(PMake)X
882(for)X
996(Gods)X
1191(...................................................................................................................................)X
3931(24)X
755 3856(4.1.)N
895(Search)X
1134(Paths)X
1331(............................................................................................................................)X
3931(24)X
755 3952(4.2.)N
895(Archives)X
1205(and)X
1341(Libraries)X
1651(............................................................................................................)X
3931(25)X
755 4048(4.3.)N
895(On)X
1013(the)X
1131(Condition...)X
1531(..................................................................................................................)X
3931(27)X
755 4144(4.4.)N
895(A)X
973(Shell)X
1157(is)X
1230(a)X
1286(Shell)X
1470(is)X
1543(a)X
1599(Shell)X
1791(.....................................................................................................)X
3931(29)X
755 4240(4.5.)N
895(Compatibility)X
1371(..........................................................................................................................)X
3931(31)X
955 4336(4.5.1.)N
1155(DEFCON)X
1495(3)X
1555(\320)X
1655(Variable)X
1952(Expansion)X
2311(...........................................................................)X
3931(31)X
955 4432(4.5.2.)N
1155(DEFCON)X
1495(2)X
1555(\320)X
1655(The)X
1800(Number)X
2083(of)X
2170(the)X
2288(Beast)X
2491(..................................................................)X
3931(31)X
955 4528(4.5.3.)N
1155(DEFCON)X
1495(1)X
1555(\320)X
1655(Imitation)X
1968(is)X
2041(the)X
2159(Not)X
2299(the)X
2417(Highest)X
2686(Form)X
2879(of)X
2966(Flattery)X
3251(............................)X
3931(32)X
755 4624(4.6.)N
895(The)X
1040(Way)X
1212(Things)X
1454(Work)X
1671(...........................................................................................................)X
3931(32)X
555 4748(5.)N
635(Answers)X
936(to)X
1018(Exercises)X
1351(...........................................................................................................................)X
3931(33)X
555 4872(6.)N
635(Glossary)X
940(of)X
1027(Jargon)X
1271(...............................................................................................................................)X
3931(33)X
555 4996(Index)N
771(........................................................................................................................................................)X
3931(35)X

36 p
%%Trailer
xt
(psc)xT
576 1 1 xr

35 p
%%Page: 35 37
10 s 10 xH 0 xS 1 f
8 s
10 s
2196 384(-)N
2243(35)X
2343(-)X
3 f
12 s
2129 672(INDEX)N
555 960(!)N
1 f
10 s
699 1104(!)N
3 f
766(2)X
1 f
699 1200(!=)N
3 f
811(5)X
1 f
(;)S
913(13)X
3 f
12 s
555 1392(#)N
1 f
10 s
699 1536(#undef)N
3 f
962(7)X
12 s
555 1728(+)N
1 f
10 s
699 1872(+=)N
3 f
829(4)X
1 f
(;)S
931(6,)X
1011(13)X
3 f
12 s
555 2064(.)N
1 f
10 s
699 2208(...)N
3 f
799(17)X
1 f
(;)S
941(21)X
699 2304(.ALLSRC)N
3 f
1065(6)X
1 f
(;)S
1167(7,)X
1247(20)X
699 2400(.ARCHIVE)N
3 f
1115(25)X
1 f
(;)S
1257(6)X
699 2496(.BEGIN)N
3 f
1004(21)X
1 f
699 2592(.DEFAULT)N
3 f
1124(21)X
1 f
699 2688(.DONTCARE)N
3 f
1195(18)X
1 f
699 2784(.END)N
3 f
924(21)X
1 f
699 2880(.EXEC)N
3 f
968(18)X
1 f
699 2976(.EXPORT)N
3 f
1070(19)X
1 f
699 3072(.EXPORTSAME)N
3 f
1292(19)X
1 f
699 3168(.IGNORE)N
843 3264(attribute)N
3 f
1150(19)X
1 f
(;)S
1292(21)X
843 3360(target)N
3 f
1066(21)X
1 f
699 3456(.IMPSRC)N
3 f
1051(14)X
1 f
(;)S
1193(6,)X
1273(21)X
699 3552(.INCLUDES)N
843 3648(target)N
3 f
1066(21)X
1 f
843 3744(variable)N
3 f
1142(21)X
1 f
699 3840(.INTERRUPT)N
3 f
1199(22)X
1 f
699 3936(.INVISIBLE)N
3 f
1151(19)X
1 f
699 4032(.JOIN)N
3 f
933(19)X
1 f
699 4128(.LIBS)N
843 4224(target)N
3 f
1066(22)X
1 f
843 4320(variable)N
3 f
1142(22)X
1 f
699 4416(.MAIN)N
3 f
973(22)X
1 f
699 4512(.MAKEFLAGS)N
843 4608(target)N
3 f
1066(22)X
1 f
843 4704(variable)N
3 f
1142(7)X
1 f
(;)S
1244(12)X
699 4800(.MAKE)N
3 f
995(20)X
1 f
(;)S
1137(22,)X
1257(32)X
699 4896(.MEMBER)N
3 f
1105(25)X
1 f
(;)S
1247(6)X
699 4992(.NOEXPORT)N
843 5088(attribute)N
3 f
1150(20)X
1 f
699 5184(.NOTMAIN)N
3 f
1138(20)X
1 f
699 5280(.NULL)N
3 f
973(22)X
1 f
(;)S
1115(16)X
699 5376(.OODATE)N
3 f
1089(6)X
1 f
(;)S
1191(20)X
699 5472(.PATH)N
3 f
968(22)X
1 f
(;)S
1110(22)X
699 5568(.PMAKE)N
3 f
1039(6)X
1 f
(;)S
1141(32)X
699 5664(.PRECIOUS)N
843 5760(attribute)N
3 f
1150(20)X
1 f
(;)S
1292(22)X
2687 960(target)N
3 f
2910(22)X
1 f
2543 1056(.PREFIX)N
3 f
2878(6)X
1 f
2543 1152(.RECURSIVE)N
3 f
3047(22)X
1 f
2543 1248(.SHELL)N
3 f
2852(22)X
1 f
(;)S
2994(29)X
2543 1344(.SILENT)N
2687 1440(attribute)N
3 f
2994(20)X
1 f
(;)S
3136(22)X
2687 1536(target)N
3 f
2910(22)X
1 f
2543 1632(.SUFFIXES)N
3 f
2971(22)X
1 f
2543 1728(.TARGET)N
3 f
2919(6)X
1 f
(;)S
3021(7,)X
3101(20)X
2543 1824(.USE)N
3 f
2754(20)X
1 f
(;)S
2896(19)X
3 f
12 s
2399 2016(:)N
1 f
10 s
2543 2160(:)N
3 f
2605(2)X
1 f
(;)S
2707(3)X
2543 2256(::)N
3 f
2627(2)X
1 f
(;)S
2729(3,)X
2809(20)X
2543 2352(:=)N
3 f
2650(5)X
1 f
(;)S
2752(6,)X
2832(13,)X
2952(33)X
2543 2448(:E)N
3 f
2654(23)X
1 f
2543 2544(:H)N
3 f
2663(23)X
1 f
2543 2640(:M)N
3 f
2676(23)X
1 f
2543 2736(:N)N
3 f
2663(23)X
1 f
2543 2832(:R)N
3 f
2658(23)X
1 f
2543 2928(:S)N
3 f
2649(23)X
1 f
(;)S
2791(33)X
2543 3024(:T)N
3 f
2654(23)X
12 s
2399 3216(=)N
1 f
10 s
2543 3360(=)N
2648(13)X
3 f
12 s
2399 3552(?)N
1 f
10 s
2543 3696(?=)N
3 f
2664(4)X
1 f
(;)S
2766(6,)X
2846(13)X
3 f
12 s
2399 3888(A)N
1 f
10 s
2543 4032(arguments)N
2937(10)X
2543 4128(attributes)N
3 f
2881(18)X
1 f
2687 4224(.DONTCARE)N
3 f
3183(18)X
1 f
2687 4320(.EXEC)N
3 f
2956(18)X
1 f
2687 4416(.EXPORT)N
3 f
3058(19)X
1 f
2687 4512(.EXPORTSAME)N
3 f
3280(19)X
1 f
2687 4608(.IGNORE)N
3 f
3050(19)X
1 f
(;)S
3192(21)X
2687 4704(.INVISIBLE)N
3 f
3139(19)X
1 f
2687 4800(.JOIN)N
3 f
2921(19)X
1 f
2687 4896(.MAKE)N
3 f
2983(20)X
1 f
(;)S
3125(22)X
2687 4992(.NOEXPORT)N
3 f
3174(20)X
1 f
2687 5088(.NOTMAIN)N
3 f
3126(20)X
1 f
2687 5184(.PRECIOUS)N
3 f
3133(20)X
1 f
(;)S
3275(22)X
2687 5280(.SILENT)N
3 f
3023(20)X
1 f
(;)S
3165(22)X
2687 5376(.USE)N
3 f
2898(20)X
1 f
(;)S
3040(19)X
2543 5472(attribute)N
2687 5568(.MAKE)N
3003(32)X
3 f
12 s
2399 5760(C)N

36 p
%%Page: 36 38
12 s 12 xH 0 xS 3 f
10 s
1 f
2196 384(-)N
2243(36)X
2343(-)X
699 672(comments)N
3 f
1068(7)X
1 f
699 768(compatibility)N
1185(4-5,)X
1332(7,)X
1412(11-12,)X
1639(31-32)X
699 864(conditional)N
843 960(de\256ned)N
3 f
1119(28)X
1 f
843 1056(empty)N
3 f
1083(28)X
1 f
843 1152(exists)N
3 f
1065(28)X
1 f
843 1248(make)N
3 f
1057(28)X
1 f
699 1344(con\256guration)N
1186(10)X
699 1440(continuation)N
1119(line)X
3 f
1279(1)X
12 s
555 1632(D)N
1 f
10 s
699 1776(debugging)N
1097(9-11)X
699 1872(de\256ned)N
3 f
975(28)X
1 f
699 1968(dependency)N
3 f
1123(2)X
1 f
(;)S
1225(2)X
843 2064(circular)N
3 f
1129(10)X
1 f
699 2160(dependency)N
1103(line)X
3 f
1263(2)X
1 f
699 2256(dynamic)N
995(source)X
3 f
1245(6)X
1 f
(;)S
1347(9,)X
1427(13)X
3 f
12 s
555 2448(E)N
1 f
10 s
699 2592(empty)N
3 f
939(28)X
1 f
699 2688(exists)N
3 f
921(28)X
12 s
555 2880(F)N
1 f
10 s
699 3024(\257ags)N
910(10)X
843 3120(-B)N
3 f
963(11)X
1 f
(;)S
1105(4-5)X
843 3216(-C)N
3 f
963(12)X
1 f
843 3312(-d)N
3 f
950(12)X
1 f
(;)S
1092(9)X
843 3408(-f)N
3 f
937(10)X
1 f
(;)S
1079(1)X
843 3504(-h)N
3 f
950(10)X
1 f
(;)S
1092(17)X
843 3600(-i)N
3 f
932(12)X
1 f
(;)S
1074(21)X
843 3696(-J)N
3 f
941(12)X
1 f
(;)S
1083(7)X
843 3792(-k)N
3 f
950(11)X
1 f
843 3888(-L)N
3 f
959(12)X
1 f
(;)S
1101(7)X
843 3984(-M)N
3 f
981(12)X
1 f
(;)S
1123(5,)X
1203(7)X
843 4080(-n)N
3 f
950(11)X
1 f
(;)S
1092(9)X
843 4176(-p)N
3 f
950(12)X
1 f
(;)S
1092(9,)X
1172(20)X
843 4272(-q)N
3 f
950(11)X
1 f
843 4368(-r)N
3 f
937(11)X
1 f
843 4464(-s)N
3 f
941(11)X
1 f
843 4560(-t)N
3 f
932(11)X
1 f
(;)S
1074(18-20)X
843 4656(-V)N
3 f
968(12)X
1 f
(;)S
1110(5)X
843 4752(-W)N
3 f
986(12)X
1 f
843 4848(-x)N
3 f
950(12)X
12 s
555 5040(I)N
1 f
10 s
699 5184(if)N
843 5280(de\256ned)N
3 f
1119(28)X
1 f
843 5376(empty)N
3 f
1083(28)X
1 f
843 5472(exists)N
3 f
1065(28)X
1 f
843 5568(make)N
3 f
1057(28)X
12 s
555 5760(M)N
1 f
10 s
2543 672(make\256le)N
3 f
2859(1)X
1 f
2687 768(default)N
3 f
2950(1)X
1 f
(;)S
3052(10)X
2687 864(inclusion)N
3 f
3020(17)X
1 f
2687 960(other)N
2912(1,)X
2992(10)X
2687 1056(quick)N
2885(and)X
3021(dirty)X
3232(10)X
2687 1152(system)N
2969(10,)X
3089(21)X
2543 1248(MAKE)N
3 f
2819(28)X
1 f
2543 1344(MFLAGS)N
3 f
2907(7)X
1 f
(;)S
3009(12)X
2543 1440(modi\256er)N
2687 1536(base)N
3 f
2870(23)X
1 f
2687 1632(extension)N
3 f
3034(23)X
1 f
2687 1728(head)N
3 f
2879(23)X
1 f
2687 1824(match)N
3 f
2923(23)X
1 f
2687 1920(nomatch)N
3 f
3003(23)X
1 f
2687 2016(root)N
3 f
2856(23)X
1 f
2687 2112(substitute)N
3 f
3033(23)X
1 f
(;)S
3175(33)X
2687 2208(suf\256x)N
3 f
2909(23)X
1 f
2687 2304(tail)N
3 f
2829(23)X
12 s
2399 2496(N)N
1 f
10 s
2543 2640(null)N
2687(suf\256x)X
3 f
2909(17)X
1 f
(;)S
3051(22,)X
3171(32)X
3 f
12 s
2399 2832(O)N
1 f
10 s
2543 2976(operator)N
3 f
2851(2)X
1 f
(;)S
2953(2,)X
3033(21)X
2687 3072(colon)N
3 f
2905(2)X
1 f
(;)S
3007(3)X
2687 3168(double-colon)N
3 f
3150(2)X
1 f
(;)S
3252(3,)X
3332(20,)X
3452(32)X
2687 3264(force)N
3 f
2893(2)X
1 f
2543 3360(out-of-date)N
3 f
2940(2)X
1 f
(;)S
3042(3-4)X
2543 3456(output)N
2767(control)X
3054(12)X
3 f
12 s
2399 3648(R)N
1 f
10 s
2543 3792(re-creation)N
2952(3)X
3 f
12 s
2399 3984(S)N
1 f
10 s
2543 4128(shell)N
2754(3)X
2543 4224(source)N
3 f
2793(2)X
1 f
(;)S
2895(3,)X
2975(18)X
2687 4320(dynamic)N
3 f
3003(6)X
1 f
(;)S
3105(9,)X
3185(13)X
2543 4416(suf\256x)N
3 f
2765(33)X
1 f
2687 4512(null)N
3 f
2851(17)X
1 f
(;)S
2993(22,)X
3113(32)X
2687 4608(variable)N
2966(modi\256er)X
3297(23)X
3 f
12 s
2399 4800(T)N
1 f
10 s
2543 4944(target)N
3 f
2766(2)X
1 f
(;)S
2868(3,)X
2948(11,)X
3068(18,)X
3188(21)X
2687 5040(.SHELL)N
3016(29)X
3 f
12 s
2399 5232(U)N
1 f
10 s
2543 5376(update)N
2817(3)X
2543 5472(usage)N
2786(10)X
3 f
12 s
2399 5664(V)N
1 f
10 s
2543 5808(variable)N

37 p
%%Page: 37 39
10 s 10 xH 0 xS 1 f
2196 384(-)N
2243(37)X
2343(-)X
843 672(appending)N
3 f
1217(4)X
1 f
843 768(assignment)N
3 f
1243(4)X
1 f
(;)S
1345(13)X
987 864(appended)N
3 f
1335(4)X
1 f
(;)S
1437(6,)X
1517(13)X
987 960(conditional)N
3 f
1387(4)X
1 f
(;)S
1489(6,)X
1569(13)X
987 1056(expanded)N
3 f
1335(5)X
1 f
(;)S
1437(6,)X
1517(13,)X
1637(33)X
987 1152(shell-output)N
3 f
1409(5)X
1 f
(;)S
1511(13)X
843 1248(command-line)N
3 f
1346(6)X
1 f
(;)S
1448(5)X
843 1344(deletion)N
3 f
1141(7)X
1 f
843 1440(environment)N
3 f
1288(7)X
1 f
(;)S
1390(5)X
987 1536(PMAKE)N
3 f
1307(7)X
1 f
(;)S
1409(12)X
843 1632(expansion)N
3 f
1208(5)X
1 f
(;)S
1310(5,)X
1390(22)X
987 1728(modi\256ed)N
3 f
1311(22)X
1 f
(;)S
1453(33)X
843 1824(global)N
3 f
1083(6)X
1 f
(;)S
1185(5)X
987 1920(.INCLUDES)N
3 f
1443(21)X
1 f
(;)S
1585(7)X
987 2016(.LIBS)N
3 f
1220(22)X
1 f
(;)S
1362(7)X
987 2112(.MAKEFLAGS)N
3 f
1536(7)X
1 f
(;)S
1638(12)X
987 2208(.PMAKE)N
3 f
1327(6)X
1 f
(;)S
1429(32)X
987 2304(MAKE)N
3 f
1263(6)X
1 f
987 2400(MFLAGS)N
3 f
1351(7)X
1 f
(;)S
1453(12)X
843 2496(local)N
3 f
1039(6)X
1 f
(;)S
1141(5)X
987 2592(.ALLSRC)N
3 f
1353(6)X
1 f
(;)S
1455(7,)X
1535(18-21)X
987 2688(.ARCHIVE)N
3 f
1403(25)X
1 f
(;)S
1545(6)X
987 2784(.IMPSRC)N
3 f
1339(14)X
1 f
(;)S
1481(6,)X
1561(21)X
987 2880(.MEMBER)N
3 f
1393(25)X
1 f
(;)S
1535(6)X
987 2976(.OODATE)N
3 f
1377(6)X
1 f
(;)S
1479(20)X
987 3072(.PREFIX)N
3 f
1322(6)X
1 f
987 3168(.TARGET)N
3 f
1363(6)X
1 f
(;)S
1465(7,)X
1545(18,)X
1665(20-21)X
843 3264(modi\256ers)N
3 f
1185(22)X
1 f
843 3360(types)N
3 f
1052(5)X

39 p
%%Trailer
xt

xs
