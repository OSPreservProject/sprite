%!PS-Adobe-1.0
%%Creator: promethium:adam (& de Boor,Ext. 238,,5492264)
%%Title: stdin (ditroff)
%%CreationDate: Mon Jul 10 03:43:10 1989
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
1412 984(Pre\256x)N
1682(\320)X
1802(Painless)X
2157(Filesystem)X
2614(Management)X
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
2 f
2094 2088(ABSTRACT)N
1 f
1043 2280(This)N
1217(paper)X
1428(describes)X
1759(the)X
1889(installation,)X
2296(use)X
2435(and)X
2582(theory)X
2818(of)X
2916(operation)X
3250(of)X
7 f
3348(prefix)X
1 f
(,)S
3687(a)X
843 2376(simple)N
1079(daemon)X
1355(to)X
1439(implement)X
1803(a)X
1861(``pre\256x)X
2124(table'')X
2356(for)X
9 s
2470(UNIX)X
10 s
(\262)S
2712(using)X
2907(NFS)X
9 f
3053(\324)X
1 f
3124(.)X
3166(The)X
3313(daemon)X
3589(pro-)X
843 2472(vides)N
1069(for)X
1220(a)X
1313(less)X
1490(painful)X
1774(\256lesystem)X
2155(management)X
2622(scheme)X
2920(than)X
3115(the)X
3270(current)X
3554(static)X
843 2568(con\256guration)N
1296(\256le)X
1424(and)X
1566(mount)X
1796(table)X
1978(scheme,)X
2265(allowing)X
2571(for)X
2690(the)X
2813(dynamic)X
3114(mounting,)X
3465(location)X
843 2664(and)N
983(unmounting)X
1393(of)X
1484(\256lesystems)X
1863(according)X
2204(to)X
2290(user)X
2447(demand.)X
2744(The)X
2892(intent)X
3097(of)X
3187(the)X
3308(daemon)X
3585(is)X
3661(to)X
843 2760(provide)N
1108(a)X
1164(globally)X
1446(consistent)X
1786(\256lesystem)X
2130(for)X
2244(use)X
2371(with)X
2533(PMake)X
2780(and)X
2916(Customs.)X
555 3144(July)N
708(10,)X
828(1989)X
10 f
555 5280(h)N
571(hhhhhhhhhhhhhh)X
8 s
9 f
667 5374(\323)N
1 f
746(Copyright)X
1034(Adam)X
1218(de)X
1306(Boor)X
1461(and)X
1580(Berkeley)X
1837(Softworks,)X
2143(1989.)X
2330(Permission)X
2642(to)X
2719(use,)X
2847(copy,)X
3014(modify,)X
3242(and)X
3361(distribute)X
3630(this)X
555 5454(software)N
791(and)X
900(its)X
978(documentation)X
1375(for)X
1466(any)X
1575(purpose)X
1794(and)X
1903(without)X
2116(fee)X
2210(is)X
2270(hereby)X
2460(granted,)X
2684(provided)X
2928(that)X
3041(the)X
3136(above)X
3305(copyright)X
3567(notice)X
555 5534(appears)N
771(in)X
843(all)X
929(copies.)X
1146(Neither)X
1359(Berkeley)X
1611(Softworks,)X
1912(nor)X
2019(Adam)X
2196(de)X
2277(Boor)X
2426(makes)X
2610(any)X
2723(representations)X
3130(about)X
3293(the)X
3392(suitability)X
3670(of)X
555 5614(this)N
664(software)X
899(for)X
989(any)X
1097(purpose.)X
1347(It)X
1402(is)X
1461(provided)X
1704("as)X
1799(is")X
1884(without)X
2096(express)X
2303(or)X
2372(implied)X
2584(warranty.)X
555 5694(\262)N
7 s
601(UNIX)X
8 s
756(is)X
815(a)X
859(trademark)X
1133(of)X
1202(Bell)X
1325(Laboratories.)X

1 p
%%Page: 1 2
8 s 8 xH 0 xS 1 f
10 s
3 f
12 s
1412 984(Pre\256x)N
1682(\320)X
1802(Painless)X
2157(Filesystem)X
2614(Management)X
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
555 1992(1.)N
655(Introduction)X
1 f
555 2145(The)N
703(``pre\256x'')X
1021(program)X
1316(implements)X
1712(a)X
1771(``pre\256x)X
2035(table'')X
2268(for)X
9 s
2383(UNIX)X
10 s
2586(running)X
2858(NFS.)X
3047(``What's)X
3356(a)X
3414(pre\256x)X
3623(table?'')X
3891(you)X
555 2241(ask.)N
703(A)X
782(pre\256x)X
990(table)X
1167(is)X
1241(a)X
1298(less)X
1439(administration-intensive)X
2238(way)X
2393(of)X
2481(maintaining)X
2884(network)X
3168(\256lesystems,)X
3564(relying)X
3812(on)X
3913(the)X
555 2337(network)N
843(to)X
930(provide)X
1200(the)X
1323(location)X
1606(of)X
1698(a)X
1759(particular)X
2091(\256lesystem,)X
2459(rather)X
2671(than)X
2833(a)X
2893(static)X
3086(con\256guration)X
3537(\256le)X
3663(\(/etc/fstab\))X
555 2433(as)N
647(is)X
725(done)X
906(now.)X
1089(In)X
1181(addition,)X
1488(the)X
1611(set)X
1725(of)X
1817(mounted)X
2122(systems)X
2400(is)X
2478(dynamic,)X
2799(with)X
2966(unneeded)X
3298(systems)X
3575(automatically)X
555 2529(unmounted)N
937(and)X
1075(mounted)X
1377(again)X
1573(when)X
1769(necessary.)X
2124(This)X
2288(dynamic)X
2586(con\256guration)X
3035(helps)X
3226(to)X
3310(reduce)X
3547(the)X
3667(chances)X
3944(of)X
555 2625(a)N
611(client)X
809(hanging)X
1087(because)X
1362(the)X
1480(server)X
1697(of)X
1784(some)X
1973(unneeded)X
2301(\256lesystem)X
2645(has)X
2772(gone)X
2948(down.)X
555 2778(The)N
703(implementation)X
1228(revolves)X
1523(around)X
1769(the)X
1890(notion)X
2117(of)X
2207(a)X
2266(``pre\256x'')X
2584(\320)X
2687(a)X
2746(unique)X
2987(name)X
3183(for)X
3299(a)X
3357(particular)X
3687(\256lesystem)X
555 2874(that)N
706(is)X
790(common)X
1101(across)X
1333(all)X
1444(systems)X
1728(on)X
1839(the)X
1967(local)X
2153(network.)X
2466(The)X
2621(``pre\256x'')X
2946(daemon)X
3230(controls)X
3518(access)X
3754(to)X
3846(these)X
555 2970(pre\256xes.)N
854(When)X
1071(a)X
1132(client)X
1335(wishes)X
1578(to)X
1665(access)X
1896(a)X
1957(\256le)X
2083(in)X
2169(a)X
2229(pre\256xed)X
2516(\256lesystem,)X
2884(the)X
3006(daemon)X
3284(broadcasts)X
3647(to)X
3733(the)X
3855(local)X
555 3066(network)N
855(to)X
954(\256nd)X
1115(out)X
1254(what)X
1447(machine)X
1755(is)X
1844(currently)X
2170(serving)X
2442(the)X
2576(pre\256x,)X
2819(accepting)X
3163(the)X
3297(\256rst)X
3457(responder)X
3810(as)X
3913(the)X
555 3162(machine)N
854(whose)X
1086(proferred)X
1413(\256lesystem)X
1764(it)X
1835(will)X
1986(use.)X
2140(This)X
2309(allows)X
2545(a)X
2608(pre\256x)X
2822(to)X
2911(be)X
3014(served)X
3251(by)X
3358(more)X
3550(than)X
3715(one)X
3858(host,)X
555 3258(with)N
723(the)X
847(one)X
989(most)X
1170(able)X
1329(to)X
1416(serve)X
1611(the)X
1734(pre\256x)X
1946(\(i.e.)X
2096(the)X
2219(one)X
2360(that)X
2505(can)X
2642(respond)X
2921(the)X
3044(fastest\))X
3301(being)X
3504(the)X
3627(one)X
3768(chosen.)X
555 3354(After)N
753(determining)X
1167(the)X
1292(server)X
1516(of)X
1610(the)X
1735(pre\256x,)X
1969(the)X
2094(daemon)X
2375(mounts)X
2637(the)X
2762(appropriate)X
3155(\256lesystem)X
3506(from)X
3689(the)X
3814(server)X
555 3450(and)N
691(the)X
809(client's)X
1065(request)X
1317(passes)X
1542(on)X
1642(to)X
1724(the)X
1842(server)X
2059(to)X
2141(be)X
2237(handled.)X
555 3603(It)N
625(is)X
699(important)X
1030(to)X
1112(note)X
1270(that)X
1410(this)X
1545(daemon)X
1819(does)X
3 f
1986(not)X
1 f
2117(bypass)X
2355(the)X
2473(normal)X
2720(access)X
2946(controls)X
3224(provided)X
3529(by)X
3629(NFS.)X
3815(While)X
555 3699(anyone)N
814(can)X
953(tell)X
1082(the)X
1207(local)X
1390(pre\256x)X
1604(daemon)X
1885(that)X
2032(a)X
2095(particular)X
2430(directory)X
2747(is)X
2827(to)X
2916(be)X
3019(exported)X
3327(under)X
3537(a)X
3600(given)X
3804(pre\256x,)X
555 3795(this)N
690(doesn't)X
946(preclude)X
1243(the)X
1361(local)X
1537(mount)X
1761(daemon)X
2035(from)X
2211(refusing)X
2494(access)X
2720(based)X
2923(on)X
3023(the)X
3141(/etc/exports)X
3535(\256le.)X
555 3948(While)N
772(there)X
954(were)X
1132(several)X
1381(goals)X
1571(considered)X
1940(in)X
2023(the)X
2142(writing)X
2393(of)X
2480(this)X
2615(daemon,)X
2909(the)X
3027(primary)X
3301(one)X
3437(was)X
3582(the)X
3700(providing)X
555 4044(of)N
657(a)X
728(globally)X
1025(consistent)X
1380(\256lesystem)X
1739(across)X
1975(all)X
2090(client)X
2303(machines)X
2641(to)X
2738(allow)X
2951(PMake)X
3213(and)X
3364(Customs)X
3678(to)X
3774(operate)X
555 4140(without)N
830(concern.)X
1135(That)X
1312(it)X
1386(is)X
1469(also)X
1628(helpful)X
1885(for)X
2009(users)X
2204(and)X
2350(administrators)X
2838(in)X
2930(general)X
3197(is)X
3280(a)X
3346(pleasant)X
3639(by-product.)X
555 4236(Because)N
847(of)X
938(this)X
1077(goal,)X
1259(and)X
1399(the)X
1521(fairly)X
1719(trusting)X
1987(atmosphere)X
2381(already)X
2642(assumed)X
2942(by)X
3046(Customs,)X
3369(the)X
3491(service)X
3743(is)X
3819(not)X
3944(as)X
555 4332(elaborate)N
883(as)X
983(it)X
1060(might)X
1279(be,)X
1408(though)X
1663(I)X
1722(doubt)X
1936(not)X
2070(that)X
2222(it)X
2298(will)X
2454(evolve)X
2700(over)X
2875(time)X
3049(as)X
3148(it)X
3224(is)X
3309(used)X
3488(in)X
3582(more)X
3779(diverse)X
555 4428(environments.)N
555 4581(``pre\256x'')N
874(was)X
1023(inspired)X
1305(by)X
1409(the)X
7 f
1531(automount)X
1 f
1987(program)X
2283(that)X
2427(comes)X
2656(with)X
2822(the)X
2943(latest)X
3135(Sun)X
3282(OS)X
3407(4.0)X
3530(release)X
3777(\(4.0.3\).)X
555 4677(It)N
632(operates)X
927(rather)X
1142(differently,)X
1528(however,)X
1852(in)X
1941(that)X
2088(automounted)X
2533(\256lesystems)X
2915(are)X
3041(mounted)X
2 f
3348(in)X
3437(place)X
1 f
3611(,)X
3658(rather)X
3873(than)X
555 4773(under)N
761(some)X
953(temporary)X
1306(mount)X
1533(directory.)X
1866(This)X
2031(is)X
2107(required)X
2397(by)X
2499(PMake's)X
2806(need)X
2980(for)X
3096(a)X
3154(globally)X
3438(consistent)X
3780(\256lesys-)X
555 4869(tem.)N
717(The)X
864(problem)X
1153(lies)X
1286(in)X
1370(the)X
7 f
1490(getwd)X
1 f
1752(library)X
1988(function.)X
2297(If)X
2373(the)X
2493(directory)X
7 f
2805(/n/promethium)X
1 f
3451(were)X
3630(mounted)X
3931(on)X
7 f
555 4965(/tmp_mnt/n/promethium)N
1 f
(,)S
1609(as)X
1702(automount)X
2070(would)X
2296(like,)X
2462(getwd)X
2683(would)X
2908(return)X
3125(a)X
3186(path)X
3349(within)X
3578(the)X
3701(/tmp_mnt)X
555 5061(directory.)N
892(This)X
1061(is)X
1141(ok,)X
1268(as)X
1362(far)X
1479(as)X
1573(local)X
1756(work)X
1948(goes,)X
2142(since)X
2333(/tmp_mnt)X
2669(is)X
2748(in)X
2836(fact)X
2983(where)X
3206(the)X
3330(\256les)X
3489(are)X
3614(to)X
3702(be)X
3804(found.)X
555 5157(Unfortunately,)N
1051(if)X
1126(PMake)X
1379(exports)X
1641(a)X
1703(job)X
1831(somewhere,)X
2243(it)X
2313(will)X
2463(pass)X
2627(along)X
2831(this)X
2971(path)X
3134(to)X
3221(the)X
3344(remote)X
3592(job)X
3719(and)X
3860(Cus-)X
555 5253(toms)N
746(will)X
906(attempt)X
1182(to)X
1280(change)X
1544(to)X
1642(that)X
1798(directory,)X
2143(an)X
2254(attempt)X
2529(that)X
2684(will)X
2843(fail)X
2985(unless)X
3220(someone)X
3540(on)X
3655(the)X
3788(remote)X
555 5349(machine)N
853(has)X
986(also)X
1141(caused)X
1386(the)X
1510(\256le)X
1638(system)X
1886(to)X
1974(be)X
2076(automounted.)X
2540(With)X
2726(``pre\256x,'')X
3067(however,)X
3390(the)X
3513(\256le)X
3640(system)X
3887(will)X
555 5445(always)N
798(be)X
894(in)X
976(the)X
1094(same)X
1279(place,)X
1489(or)X
1576(will)X
1720(be)X
1816(automounted)X
2254(if)X
2323(it)X
2387(is)X
2460(not,)X
2602(and)X
2738(getwd)X
2954(will)X
3098(return)X
3310(the)X
3428(proper)X
3658(value.)X

2 p
%%Page: 2 3
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(2)X
2323(-)X
3 f
555 672(2.)N
655(Operation)X
1 f
555 825(The)N
700(pre\256x)X
907(daemon)X
1181(has)X
1308(two)X
1448(jobs)X
1601(to)X
1683(perform:)X
10 f
755 978(g)N
1 f
835(Export)X
1086(pre\256xes)X
1373(to)X
1468(other)X
1666(machines,)X
2021(providing)X
2364(the)X
2494(local)X
2682(directory)X
3004(name)X
3210(in)X
3304(response)X
3617(to)X
3711(a)X
3779(request)X
835 1074(from)N
1011(a)X
1067(remote)X
1310(pre\256x)X
1517(daemon.)X
10 f
755 1227(g)N
1 f
835(Import)X
1083(pre\256xes)X
1366(from)X
1551(other)X
1745(machines,)X
2097(locating)X
2384(their)X
2560(servers)X
2817(and)X
2962(mounting)X
3297(them)X
3486(for)X
3609(normal)X
3865(NFS)X
835 1323(access)N
1061(on)X
1161(demand,)X
1455(unmounting)X
1861(them)X
2041(when)X
2235(they)X
2393(are)X
2512(no)X
2612(longer)X
2837(required.)X
555 1476(The)N
704(\256rst)X
852(task)X
1005(is)X
1082(simple)X
1319(to)X
1405(perform)X
1688(and)X
1828(quickly)X
2092(explained.)X
2448(Any)X
2610(directory)X
2924(on)X
3027(the)X
3148(system)X
3393(may)X
3554(be)X
3653(declared)X
3949(to)X
555 1572(be)N
658(the)X
783(server)X
1007(for)X
1128(a)X
1191(pre\256x.)X
1445(When)X
1664(a)X
1727(request)X
1985(arrives)X
2230(for)X
2350(the)X
2474(given)X
2678(pre\256x,)X
2911(the)X
3035(daemon)X
3315(will)X
3465(respond)X
3745(with)X
3913(the)X
555 1668(directory.)N
886(To)X
996(ensure)X
1227(against)X
1475(``operator)X
1818(error,'')X
2070(pre\256x)X
2278(will)X
2423(consult)X
7 f
2675(/etc/exports)X
1 f
3272(to)X
3355(make)X
3550(sure)X
3705(the)X
3823(direc-)X
555 1764(tory)N
711(is)X
790(actually)X
1070(exported,)X
1397(warning)X
1686(you)X
1832(if)X
1907(it)X
1977(can't)X
2164(locate)X
2382(the)X
2506(directory.)X
2842(Note)X
3024(that)X
3170(no)X
3276(attention)X
3582(is)X
3661(paid)X
3825(to)X
3913(the)X
555 1860(export)N
784(restrictions)X
1164(that)X
1308(may)X
1470(be)X
1570(placed)X
1804(in)X
1890(the)X
2012(\256le.)X
2157(This)X
2322(is)X
2398(arguably)X
2702(a)X
2761(bug,)X
2924(as)X
3014(it)X
3081(could)X
3282(cause)X
3484(a)X
3543(remote)X
3789(system)X
555 1956(to)N
646(fail)X
782(in)X
873(mounting)X
1208(a)X
1273(pre\256x)X
1489(that)X
1638(it)X
1711(could)X
1918(actually)X
2201(get)X
2327(from)X
2511(somewhere)X
2905(else)X
3058(simply)X
3303(because)X
3586(the)X
3712(restricted)X
555 2052(host)N
708(responded)X
1058(\256rst.)X
555 2205(Imported)N
869(pre\256xes)X
1143(are)X
1262(divided)X
1522(into)X
1666(two)X
1806(categories:)X
2174(``root'')X
2431(and)X
2567(``imported'')X
2984(pre\256xes.)X
555 2358(An)N
686(``imported'')X
1116(pre\256x)X
1336(is)X
1422(a)X
1491(real)X
1645(pre\256x)X
1865(available)X
2188(on)X
2301(the)X
2432(network.)X
2748(As)X
2870(such,)X
3070(pre\256x)X
3290(will)X
3447(broadcast)X
3787(for)X
3913(the)X
555 2454(pre\256x's)N
820(server)X
1037(whenever)X
1370(you)X
1510(try)X
1619(to)X
1701(lookup)X
1943(anything)X
2243(in)X
2325(that)X
2465(directory.)X
555 2607(A)N
634(``root'')X
892(pre\256x,)X
1120(on)X
1221(the)X
1340(other)X
1526(hand,)X
1723(is)X
1797(internal)X
2063(to)X
2146(the)X
2265(machine,)X
2578(being)X
2777(the)X
2895(root)X
3044(of)X
3131(a)X
3187(tree)X
3328(of)X
3415(pre\256xes.)X
3709(Anything)X
555 2703(sought)N
793(by)X
898(the)X
1021(kernel)X
1246(immediately)X
1670(below)X
1890(one)X
2030(is)X
2107(assumed)X
2407(to)X
2493(be)X
2593(an)X
2693(imported)X
3006(pre\256x,)X
3237(to)X
3323(be)X
3423(sought)X
3660(on)X
3764(the)X
3886(net-)X
555 2799(work)N
758(should)X
1009(the)X
1145(kernel)X
1384(attempt)X
1662(to)X
1762(locate)X
1992(anything)X
2310(inside)X
2539(it.)X
2661(For)X
2810(example,)X
3139(if)X
3225(you)X
3382(have)X
3571(a)X
3644(network)X
3944(of)X
555 2895(machines)N
880(with)X
1044(local)X
1222(disks)X
1408(and)X
1546(you)X
1688(want)X
1866(the)X
1985(main)X
2166(partition)X
2458(of)X
2546(each)X
2715(to)X
2798(be)X
2895(accessible)X
3242(as)X
7 f
3330(/n/)X
2 f
(machine)S
1 f
3746(,)X
3787(declar-)X
555 2991(ing)N
7 f
677(/n)X
1 f
793(to)X
875(be)X
971(a)X
1027(root)X
1176(pre\256x)X
1383(will)X
1527(cause)X
1726(the)X
1844(proper)X
2074(thing)X
2258(to)X
2340(happen.)X
555 3144(Why)N
736(this)X
876 0.3375(difference?)AX
1264(It's)X
1396(more)X
1585(\257exible,)X
1869(since)X
2058(pre\256x)X
2269(must)X
2448(be)X
2548(informed)X
2866(of)X
2957(each)X
3129(pre\256x)X
3340(it)X
3408(should)X
3645(import)X
3882(\(not)X
555 3240(being)N
756(party)X
944(to)X
1029(the)X
1150(name)X
1347(lookup)X
1592(performed)X
1950(by)X
2052(the)X
2172(kernel\).)X
2442(It)X
2513(is)X
2588(much)X
2788(easier)X
2998(to)X
3082(have)X
3256(a)X
3314(few,)X
3477(well-known)X
3882(root)X
555 3336(pre\256xes)N
831(under)X
1036(which)X
1253(needed)X
1502(directories)X
1862(can)X
1995(be)X
2092(exported)X
2394(than)X
2553(to)X
2636(have)X
2809(each)X
2978(pre\256x)X
3186(entered)X
3444(in)X
3527(a)X
3584(con\256guration)X
555 3432(\256le)N
691(on)X
805(each)X
987(client)X
1199(system.)X
1475(One)X
1643(of)X
1744(the)X
1876(purposes)X
2195(of)X
2296(this)X
2445(daemon)X
2733(is,)X
2840(after)X
3022(all,)X
3155(to)X
3250(get)X
3381(away)X
3584(from)X
3773(a)X
3842(static)X
555 3528(con\256guration)N
1002(\256le.)X
3 f
555 3720(3.)N
655(Options)X
1 f
555 3873(``pre\256x'')N
870(accepts)X
1127(several)X
1375(command-line)X
1858(options)X
2113(that)X
2253(dictate)X
2487(its)X
2582(actions:)X
9 f
555 4026(-)N
1 f
599(D[D])X
755 4122(Enter)N
958(daemon)X
1241(mode.)X
1468(This)X
1639(includes)X
1935(detaching)X
2276(from)X
2461(its)X
2565(parent.)X
2815(If)X
2898(the)X
3025(second)X
3277(D)X
3364(is)X
3446(given,)X
3672(pre\256x)X
3887(will)X
755 4218(enter)N
936(its)X
1031(debugging)X
1389(mode.)X
9 f
555 4371(-)N
1 f
599(d)X
2 f
659(pre\256x)X
1 f
755 4467(Causes)N
1003(the)X
1122(imported)X
1432(or)X
1520(exported)X
1822(pre\256x)X
2030(to)X
2113(be)X
2210(deleted.)X
2483(If)X
2558(the)X
2677(pre\256x)X
2885(is)X
2959(imported)X
3269(and)X
3406(mounted,)X
3727(it)X
3791(will)X
3935(be)X
755 4563(unmounted)N
1138(\256rst.)X
1305(If)X
1382(the)X
1503(pre\256x)X
1713(is)X
1789(a)X
1848(root)X
2000(pre\256x,)X
2230(all)X
2333(mounted)X
2636(subpre\256xes)X
3024(will)X
3171(be)X
3270(unmounted)X
3653(\256rst.)X
3819(If)X
3895(any)X
755 4659(unmounting)N
1161(fails,)X
1339(the)X
1457(pre\256x)X
1664(will)X
1808(not)X
1930(be)X
2026(deleted.)X
9 f
555 4812(-)N
1 f
599(f)X
2 f
646(\256le)X
1 f
755 4908(Speci\256es)N
1064(a)X
1120(con\256guration)X
1567(\256le)X
1689(to)X
1771(be)X
1867(read.)X
2046(If)X
2120(the)X
2238(\256le)X
2360(is)X
2433(read)X
2592(successfully,)X
3024(this)X
3159(automatically)X
3615(implies)X
9 f
3870(-)X
1 f
3914(D.)X
9 f
555 5061(-)N
1 f
599(i)X
2 f
641(pre\256x)X
1 f
755 5157(Import)N
993(the)X
1111(given)X
1309(pre\256x,)X
1536(using)X
1729(the)X
1847(default)X
2090(mounting)X
2416(options)X
2671(\(rw\).)X
9 f
555 5310(-)N
1 f
599(p)X
755 5406(Print)N
930(the)X
1048(current)X
1296(state)X
1463(of)X
1550(imported)X
1859(and)X
1995(exported)X
2296(pre\256xes.)X
9 f
555 5559(-)N
1 f
599(q)X
755 5655(Used)N
946(only)X
1114(when)X
1314(invoking)X
1624(pre\256x)X
1837(as)X
1930(a)X
1991(daemon,)X
2290(causes)X
2525(standard)X
2822(status)X
3029(messages)X
3357(not)X
3484(to)X
3571(be)X
3672(sent)X
3826(to)X
3913(the)X
755 5751(console.)N
1040(Error)X
1230(messages)X
1553(are)X
1672(still)X
1811(written)X
2058(there,)X
2259(however.)X

3 p
%%Page: 3 4
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(3)X
2323(-)X
9 f
555 672(-)N
1 f
599(r)X
2 f
646(pre\256x)X
1 f
755 768(Set)N
877(the)X
995(given)X
1193(pre\256x)X
1400(as)X
1487(a)X
1543(root)X
1692(pre\256x.)X
9 f
555 921(-)N
1 f
599(x)X
2 f
659(directory)X
1 f
973([)X
2 f
1000(pre\256x)X
1 f
1183(])X
755 1017(Export)N
998(the)X
1121(given)X
1324(directory)X
1639(under)X
1847(the)X
1970(given)X
2172(pre\256x,)X
2403(or)X
2494(as)X
2585(itself)X
2769(if)X
2842(no)X
2946(pre\256x)X
3157(given.)X
3379(If)X
2 f
3457(directory)X
1 f
3775(doesn't)X
755 1113(appear)N
990(in)X
7 f
1072(/etc/exports)X
1 f
(,)S
1688(a)X
1744(warning)X
2027(message)X
2319(is)X
2392(printed)X
2639(to)X
2721(the)X
2839(console.)X
3 f
555 1305(4.)N
655(Installation)X
1 f
555 1458(The)N
705(installation)X
1085(of)X
1177(``pre\256x'')X
1497(is)X
1575(relatively)X
1902(simple,)X
2159(since)X
2348(it)X
2416(is)X
2493(a)X
2553(more-or-less)X
2983(self-contained)X
3462(system.)X
3728(The)X
3877(pro-)X
555 1554(gram)N
748(operates)X
1044(in)X
1134(either)X
1345(daemon)X
1627(mode)X
1833(or)X
1928(as)X
2023(a)X
2087(user's)X
2307(agent)X
2509(when)X
2711(conversing)X
3091(with)X
3260(the)X
3385(local)X
3568(daemon,)X
3869(so)X
3967(it)X
555 1650(should)N
790(probably)X
1097(be)X
1195(installed)X
1488(in)X
1572(/etc)X
1710(with)X
1873(a)X
1930(link)X
2075(to)X
2158(it)X
2223(from)X
2400(a)X
2457(user-accessible)X
2965(directory.)X
2 f
3296(It)X
3366(should)X
3600(not)X
3723(be)X
3820(setuid)X
555 1746(to)N
645(root.)X
1 f
845(As)X
961(it)X
1032(must)X
1214(be)X
1317(started)X
1558(at)X
1643(boot)X
1812(time,)X
2001(when)X
2202(it)X
2273(will)X
2424(be)X
2527(executed)X
2840(by)X
2947(root,)X
3123(there)X
3311(is)X
3391(no)X
3498(need)X
3677(to)X
3766(make)X
3967(it)X
555 1842(setuid.)N
555 1995(Examine)N
861(the)X
980(make\256le)X
1277(in)X
1360(the)X
1479(pre\256x)X
1687(directory)X
1998(to)X
2081(set)X
2191(the)X
2310(installation)X
2686(point)X
2871(and)X
3007(the)X
3125(directory)X
3435(in)X
3517(which)X
3733(the)X
3851(sym-)X
555 2091(bolic)N
742(link)X
893(is)X
973(to)X
1062(be)X
1165(placed,)X
1422(as)X
1516(well)X
1681(as)X
1775(to)X
1864(set)X
1979(the)X
2103(names)X
2334(of)X
2427(certain)X
2672(system)X
2920(\256les,)X
3099(if)X
3174(you've)X
3423(moved)X
3667(them.)X
3873(You)X
555 2187(should)N
788(then)X
946(be)X
1042(able,)X
1216(as)X
1303(root,)X
1472(to)X
1554(type)X
1712(``make)X
1960(install''.)X
555 2340(Once)N
748(you've)X
994(installed)X
1288(the)X
1409(daemon,)X
1706(you)X
1849(must)X
2027(arrange)X
2292(for)X
2409(it)X
2476(to)X
2561(start)X
2722(at)X
2803(boot)X
2968(time)X
3133(and)X
3272(be)X
3370(given)X
3570(the)X
3690(proper)X
3922(set)X
555 2436(of)N
655(initial)X
874(pre\256xes,)X
1181(both)X
1356(imported)X
1678(and)X
1827(exported.)X
2161(The)X
2319(best)X
2481(way)X
2648(to)X
2743(tell)X
2878(it)X
2955(this)X
3103(information)X
3514(is)X
3599(via)X
3729(a)X
3797(\(short\))X
555 2532(con\256guration)N
1012(\256le.)X
1164(I)X
1221(use)X
7 f
1358(/etc/prefix.conf)X
1 f
(.)S
2196(The)X
2351(con\256guration)X
2808(\256le)X
2940(consists)X
3223(of)X
3320(a)X
3386(series)X
3599(of)X
3695(command)X
555 2628(lines,)N
760(with)X
936(optional)X
1232(comments)X
1595(interspersed.)X
2037(There)X
2259(are)X
2392(\256ve)X
2546(commands)X
2927(understood:)X
3338(import,)X
3604(export,)X
3862(root,)X
555 2724(quiet)N
735(and)X
871(debug.)X
555 2877(import)N
2 f
788(pre\256x)X
1 f
991([)X
2 f
1018(mount)X
1238(options)X
1 f
1473(])X
755 2973(Declares)N
2 f
1064(pre\256x)X
1 f
1274(to)X
1363(be)X
1466(an)X
1569(imported)X
1885(pre\256x.)X
2119(When)X
2338(it)X
2409(is)X
2489(mounted,)X
2816(the)X
2941(optional)X
2 f
3230(mount)X
3457(options)X
1 f
3719(are)X
3844(used.)X
755 3069(These)N
967(options)X
1222(are)X
1341(exactly)X
1593(as)X
1680(used)X
1847(in)X
1929(/etc/fstab)X
2243(and)X
2379(default)X
2622(to)X
7 f
2704(rw)X
1 f
2820(if)X
2889(you)X
3029(don't)X
3218(give)X
3376(any.)X
555 3222(export)N
2 f
780(directory)X
1 f
1094([)X
2 f
1121(pre\256x)X
1 f
1304(])X
755 3318(Declares)N
2 f
1057(directory)X
1 f
1371(to)X
1453(be)X
1549(exported)X
1850(under)X
2 f
2053(pre\256x)X
1 f
2236(,)X
2276(if)X
2345(given,)X
2563(or)X
2650(as)X
2737(itself)X
2917(if)X
2986(no)X
3086(pre\256x)X
3293(is)X
3366(speci\256ed.)X
555 3471(root)N
2 f
704(pre\256x)X
1 f
907([)X
2 f
934(mount)X
1154(options)X
1 f
1389(])X
755 3567(Declares)N
2 f
1070(pre\256x)X
1 f
1286(to)X
1381(be)X
1490(a)X
1559(root)X
1721(pre\256x.)X
1961(Any)X
2131(pre\256x)X
2350(mounted)X
2662(under)X
2877(it)X
2953(will)X
3109(be)X
3217(mounted)X
3529(with)X
3703(the)X
3833(given)X
2 f
755 3663(mount)N
975(options)X
1 f
1210(,)X
1250(as)X
1337(described)X
1665(for)X
1779(the)X
7 f
1897(import)X
1 f
2205(command.)X
555 3816(quiet)N
755 3912(Normally,)N
1106(pre\256x)X
1317(will)X
1465(write)X
1654(to)X
1740(/dev/console)X
2169(when)X
2367(it)X
2435(is)X
2512(broadcasting)X
2946(for)X
3063(the)X
3184(server)X
3404(of)X
3494(a)X
3553(pre\256x,)X
3783(as)X
3873(well)X
755 4008(as)N
867(what)X
1068(server/directory)X
1622(pair)X
1792(it)X
1881(is)X
1979(actually)X
2277(mounting)X
2627(and)X
2787(any)X
2947(problems)X
3289(with)X
3475(the)X
3617(mounting)X
3967(it)X
755 4104(encounters.)N
1145(If)X
1221(the)X
7 f
1341(quiet)X
1 f
1603(command)X
1941(is)X
2016(given,)X
2236(the)X
2356(notices)X
2605(about)X
2805(the)X
2924(broadcasting)X
3355(for)X
3470(and)X
3607(the)X
3726(server)X
3944(of)X
755 4200(a)N
811(pre\256x)X
1018(will)X
1162(not)X
1284(be)X
1380(printed.)X
1647(Error)X
1837(messages)X
2160(will)X
2304(still)X
2443(be)X
2539(written)X
2786(to)X
2868(the)X
2986(console,)X
3271(however.)X
555 4353(debug)N
755 4449(Turns)N
970(on)X
1078(debugging)X
1444(for)X
1566(the)X
1692(daemon,)X
1994(including)X
2323(extensive)X
2653(heap)X
2832(checking)X
3149(and)X
3292(very)X
3462(verbose)X
3739(progress)X
755 4545(messages)N
1102(that)X
1266(were)X
1466(needed)X
1737(during)X
1989(development)X
2446(because)X
2744(the)X
2885(thing)X
3092(very)X
3278(frequently)X
3651(caused)X
3913(the)X
755 4641(machine)N
1047(to)X
1129(hang)X
1305(\(thank)X
1530(you)X
1670(``disk)X
1877(wait,'')X
2109(may)X
2267(you)X
2407(rot)X
2516(in)X
2598(Hell\).)X
2803(Not)X
2943(very)X
3106(useful...)X
555 4794(As)N
664(an)X
760(example,)X
1072(here)X
1231(is)X
1304(the)X
1422(con\256guration)X
1869(\256le)X
1991(I've)X
2141(got)X
2263(on)X
2363(my)X
2485(workstation:)X

4 p
%%Page: 4 5
10 s 10 xH 0 xS 1 f
7 f
1 f
2216 384(-)N
2263(4)X
2323(-)X
7 f
843 720(#)N
843 816(#)N
939(Prefix)X
1275(configuration)X
1947(file)X
2187(for)X
2379(promethium)X
843 912(#)N
843 1008(#)N
843 1104(#)N
939(Standard)X
1371(prefix)X
1707(roots)X
843 1200(#)N
843 1296(root)N
1323(/n)X
1995(rw,intr)X
843 1392(root)N
1323(/rn)X
1995(rw,intr)X
843 1488(#)N
843 1584(#)N
939(Standard)X
1371(imports)X
843 1680(#)N
843 1776(import)N
1323(/old_staff)X
1995(rw,intr)X
843 1872(#)N
843 1968(#)N
939(Machine-specific)X
1755(imports/roots)X
843 2064(#)N
843 2160(import)N
1323(/usr/src)X
1995(rw,intr)X
843 2256(#)N
843 2352(#)N
939(Standard)X
1371(exports)X
843 2448(#)N
843 2544(export)N
1323(/)X
1995(/rn/pm)X
843 2640(export)N
1323(/n/pm)X
843 2736(export)N
1323(/n/pm)X
1995(/n/promethium)X
1 f
555 2937(The)N
710(mount)X
944(options)X
1209(are)X
1338(optional,)X
1650(even)X
1832(though)X
2084(I've)X
2244(got)X
2376(them)X
2566(for)X
2690(all)X
2800(imported)X
3118(pre\256xes.)X
3421(Note)X
3606(also)X
3764(that)X
3913(the)X
555 3033(same)N
740(disk)X
893(partition)X
1184(\(/n/pm\))X
1444(is)X
1517(exported)X
1818(under)X
2021(two)X
2161(names.)X
2406(This)X
2568(is)X
2641(perfectly)X
2947(acceptable.)X
555 3186(One)N
719(other)X
914(question)X
1215(arises)X
1427(in)X
1518(as)X
1614(far)X
1733(as)X
1829(in)X
1920(which)X
2145(/etc/rc.foo)X
2502(\256le)X
2633(the)X
2760(daemon)X
3043(should)X
3285(be)X
3390(started.)X
3653(I)X
3709(start)X
3876(it)X
3949(in)X
555 3282(/etc/rc)N
776(itself,)X
976(with)X
1138(the)X
1256(command:)X
7 f
843 3426(if)N
987([)X
1083(-f)X
1227(/etc/prefix)X
1803(-a)X
1947(-f)X
2091(/etc/prefix.conf)X
2907(];)X
3051(then)X
1035 3522(\(echo)N
1323(starting)X
1755(prefix)X
2091(daemon\))X
3043(>/dev/console)X
1035 3618(/etc/prefix)N
1611(/etc/prefix.conf)X
843 3714(fi)N
1 f
555 3858(This)N
738(goes)X
926(immediately)X
1367(before)X
1614(the)X
1753(mounting)X
2100(of)X
2208(the)X
2346(remaining)X
2711(local)X
2907(\(4.2)X
3074(or)X
3181(4.3-type\))X
3513(partitions)X
3855(from)X
555 3954(/etc/fstab)N
872(because)X
1150(I've)X
1303(a)X
1362(partition)X
1656(that)X
1799(wants)X
2009(mounting)X
2338(on)X
7 f
2441(/n/pm)X
1 f
(.)S
2744(Because)X
3034(of)X
3123(the)X
3243(way)X
3399(the)X
3519(daemon)X
3795(works,)X
555 4050(and)N
695(the)X
817(fact)X
961(that)X
1104(/n)X
1189(is)X
1265(a)X
1324(\(root\))X
1530(pre\256x,)X
1760(the)X
1881(daemon)X
2158(must)X
2336(be)X
2435(started)X
2672(before)X
2901(the)X
3022(local)X
3201(partition)X
3495(is)X
3571(mounted,)X
3894(or)X
3984(I)X
555 4146(wouldn't)N
869(be)X
970(able)X
1129(to)X
1216(access)X
1447(the)X
1570(local)X
1751(partition)X
2047(\(dire)X
2223(things)X
2442(would)X
2666(happen,)X
2942(likely\).)X
3195(It)X
3268(is)X
3345(permissible)X
3738(to)X
3824(expli-)X
555 4242(citly)N
718(mount)X
943(a)X
1000(local)X
1177(or)X
1265(remote)X
1509(\256lesystem)X
1854(immediately)X
2275(under)X
2479(a)X
2536(root)X
2686(pre\256x,)X
2914(as)X
3002(the)X
3121(pre\256x)X
3329(is)X
3403(only)X
3565(sought)X
3798(should)X
555 4338(you)N
703(attempt)X
971(to)X
1060(look)X
1229(anything)X
1536(up)X
1643(in)X
1732(it)X
1803(\(mounting)X
2163(a)X
2226(\256lesystem)X
2577(doesn't)X
2840(qualify\),)X
3141(nor)X
3275(will)X
3426(the)X
3551(kernel)X
3779(try)X
3895(and)X
555 4434(look)N
717(anything)X
1017(up)X
1117(there)X
1298(so)X
1389(long)X
1551(as)X
1638(the)X
1756(\256lesystem)X
2100(remains)X
2374(mounted.)X
555 4587(An)N
681(alternative,)X
1067(albeit)X
1272(a)X
1335(strange)X
1594(one,)X
1757(to)X
1846(the)X
1971(con\256guration)X
2425(\256le)X
2554(is)X
2634(to)X
2723(specify)X
2982(all)X
3089(the)X
3214(pre\256xes)X
3495(using)X
3695(command)X
555 4683(line)N
695(options.)X
3 f
555 4875(5.)N
655(Internals)X
1 f
555 5028(This)N
726(section)X
982(is)X
1064(intended)X
1369(to)X
1460(give)X
1627(you)X
1776(a)X
1841(working)X
2137(understanding)X
2620(of)X
2716(how)X
2883(the)X
3010(pre\256x)X
3226(daemon)X
3509(does)X
3685(its)X
3789(job,)X
3940(so)X
555 5124(you'll)N
766(have)X
938(some)X
1127(idea)X
1281(of)X
1368(what's)X
1602(going)X
1804(on)X
1904(should)X
2137(the)X
2255(daemon)X
2529(have)X
2701(a)X
2757(bug)X
2897(in)X
2979(it)X
3043(\(gasp!)X
3264(not)X
2 f
3386(my)X
1 f
3500(software!\).)X
555 5277(The)N
702(main)X
884(aspect)X
1107(of)X
1196(the)X
1316(daemon)X
1591(to)X
1674(know)X
1873(is)X
1947(that)X
2088(it)X
2153(appears)X
2420(to)X
2503(the)X
2622(kernel)X
2844(as)X
2932(just)X
3068(another)X
3330(NFS)X
3497(server.)X
3735(The)X
3881(ker-)X
555 5373(nel)N
678(doesn't)X
939(care)X
1099(that)X
1244(its)X
1344(address)X
1609(is)X
1686(on)X
1790(the)X
1912(local)X
2092(machine,)X
2408(as)X
2499(long)X
2665(as)X
2756(the)X
2878(daemon)X
3156(obeys)X
3367(the)X
3489(protocol.)X
3820(Pre\256x)X
555 5469(sets)N
699(up)X
803(one)X
943(mount)X
1171(point)X
1359(\(with)X
1552(itself)X
1736(as)X
1826(the)X
1947(server\))X
2194(for)X
2311(each)X
2482(imported)X
2794(and)X
2933(root)X
3085(pre\256x,)X
3315(with)X
3480(one)X
3619(other)X
3807(mount)X
555 5565(point)N
740(at)X
7 f
819(/.prefix)X
1 f
1224(whose)X
1450(purpose)X
1725(I'll)X
1844(explain)X
2101(in)X
2184(a)X
2241(bit.)X
2366(These)X
2579(mount)X
2804(points)X
3020(are)X
3140(made)X
2 f
3334(soft)X
1 f
3469(with)X
3631(timeout)X
3895(and)X
555 5661(retransmission)N
1054(parameters)X
1439(tailored)X
1716(to)X
1810(the)X
1940(operation)X
2275(of)X
2374(the)X
2504(daemon.)X
2809(They)X
3005(are)X
3135(soft)X
3286(so)X
3388(operations)X
3753(can)X
3896(just)X
555 5757(timeout)N
819(should)X
1052(the)X
1170(daemon)X
1444(die)X
1562(\(remember,)X
1955(this)X
2090(is)X
2163(fairly)X
2357(young)X
2577(software\).)X

5 p
%%Page: 5 6
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(5)X
2323(-)X
555 672(As)N
669(a)X
729(directory,)X
1063(each)X
1235(mount)X
1463(point)X
1651(should)X
1888(be)X
1988(considered)X
2360(\(and)X
2527(is)X
2604(actually)X
2882(mounted\))X
2 f
3213(read-only)X
1 f
3549(\320)X
3653(you)X
3797(cannot)X
555 768(create)N
770(symbolic)X
1084(links,)X
1280(subdirectories)X
1751(or)X
1839(\256les)X
1993(in)X
2076(one.)X
2253(The)X
2399(only)X
2562(NFS)X
2729(operations)X
3084(supported)X
3421(are)X
3541(the)X
3660(fetching)X
3944(of)X
555 864(attributes,)N
906(the)X
1037(lookup)X
1292(of)X
1392(a)X
1461(\256le,)X
1615(the)X
1745(fetching)X
2040(of)X
2139(\256lesystem)X
2495(attributes)X
2825(and)X
2973(the)X
3103(reading)X
3376(of)X
3475(directory)X
3797(entries)X
555 960(\(always)N
830(just)X
970(.)X
1015(and)X
1156(..,)X
1241(except)X
1476(for)X
1595(a)X
1656(root)X
1810(pre\256x,)X
2042(from)X
2223(which)X
2444(a)X
2504(client)X
2706(will)X
2854(also)X
3007(obtain)X
3231(the)X
3353(names)X
3582(of)X
3673(previously)X
555 1056(encountered)N
968(subpre\256xes\).)X
555 1209(For)N
692(all)X
798(of)X
891(these)X
1082(mount)X
1312(points,)X
1553(save)X
1722(that)X
1868(for)X
1988(/.pre\256x,)X
2263(only)X
2431(the)X
2555(lookup)X
2803(of)X
2896(a)X
2958(\256le)X
3086(will)X
3236(cause)X
3441(the)X
3565(daemon)X
3844(to)X
3931(do)X
555 1305(anything)N
859(special)X
1106(\320)X
1210(fetching)X
1497(the)X
1619(attributes)X
1941(of)X
2032(an)X
2131(unmounted)X
2514(pre\256x)X
2724(will)X
2871(return)X
3086(the)X
3207(attributes)X
3528(of)X
3618(the)X
3739(underly-)X
555 1401(ing)N
679(directory;)X
1013(reading)X
1276(it)X
1342(as)X
1431(a)X
1489(directory)X
1801(will)X
1946(only)X
2109(return)X
2322(precalculated)X
2772(information,)X
3191(potentially)X
3554(making)X
3815(some-)X
555 1497(one)N
701(erroneously)X
1110(think)X
1304(the)X
1432(pre\256x)X
1648(is)X
1730(mounted)X
2039(and)X
2184(empty,)X
2433(or)X
2529(that)X
2678(the)X
2805(daemon)X
3088(has)X
3224(died.)X
3431(The)X
3585(reason)X
3824(pre\256x)X
555 1593(waits)N
748(for)X
866(a)X
926(lookup)X
1172(is)X
1248(to)X
1333(prevent)X
1597(the)X
1718(search)X
1947(for,)X
2084(and)X
2223(mounting)X
2552(of,)X
2662(a)X
2721(pre\256x)X
2931(until)X
3100(it)X
3167(is)X
3243(absolutely)X
3595(necessary)X
3931(\320)X
555 1689(you)N
702(wouldn't)X
1017(want)X
1199(to)X
1287(have)X
1465(the)X
1589(pre\256x)X
1802(mounted)X
2108(if)X
2183(the)X
2307(user)X
2467(just)X
2608(did)X
2736(an)X
2838(ls)X
2917(of)X
3010(its)X
3111(containing)X
3475(directory,)X
3811(would)X
555 1785(you?)N
731(So)X
835(it)X
899(is)X
972(when)X
1166(you)X
1306(lookup)X
1548(something)X
1901(in)X
1983(the)X
2101(pre\256x)X
2308(that)X
2448(the)X
2566(fun)X
2693(begins.)X
3 f
555 1977(5.1.)N
715(Mounting)X
1072(\320)X
1172(The)X
1325(Fun)X
1482(Begins)X
1 f
555 2130(For)N
687(a)X
744(root)X
893(pre\256x,)X
1120(the)X
1238(fun)X
1365(is)X
1438(short-lived:)X
1827(all)X
1927(the)X
2045(daemon)X
2319(does)X
2486(is)X
2559(create)X
2772(a)X
2828(new)X
2982(pre\256x)X
3189(for)X
3303(its)X
3398(own)X
3556(use)X
3683(and)X
3819(return)X
555 2226(a)N
611(handle)X
845(and)X
981(the)X
1099(root's)X
1306(attributes)X
1624(\(though)X
1893(with)X
2055(a)X
2111(different)X
2408(\256le)X
2530(number,)X
2815(so)X
2906(the)X
3024(kernel)X
3245(won't)X
3452(get)X
3570(confused\).)X
555 2379(For)N
691(an)X
791(imported)X
1104(pre\256x,)X
1335(however,)X
1656(the)X
1778(lookup)X
2024(of)X
2115(a)X
2175(\256le)X
2301(is)X
2378(a)X
2438(momentous)X
2835(occasion.)X
3160(The)X
3309(importance,)X
3714(however,)X
555 2475(is)N
631(lost)X
769(on)X
872(the)X
993(daemon,)X
1290(as)X
1380(it)X
1447(will)X
1594(always)X
1840(return)X
2055(the)X
2176(same)X
2364(answer:)X
2637(no)X
2740(matter)X
2968(what)X
3147(is)X
3223(being)X
3423(sought,)X
3678(pre\256x)X
3887(will)X
555 2571(always)N
802(say)X
933(the)X
1055(target)X
1262(is)X
1339(a)X
1399(symbolic)X
1716(link)X
1863(to)X
1948(something)X
2304(in)X
2389(the)X
2510(/.pre\256x)X
2762(directory.)X
3095(This)X
3260(subterfuge)X
3622(is)X
3698(necessary)X
555 2667(to)N
641(cause)X
844(the)X
966(kernel)X
1191(to)X
1277(unlock)X
1519(the)X
1641(data)X
1799(structure)X
2104(by)X
2208(which)X
2428(it)X
2495(found)X
2705(the)X
2826(pre\256x)X
3036(daemon,)X
3333(allowing)X
3636(the)X
3757(daemon)X
555 2763(to)N
638(unmount)X
943(itself)X
1124(and)X
1261(mount)X
1486(the)X
1605(real)X
1747(\256lesystem)X
2092(instead.)X
2380(This)X
2543(bait)X
2684(and)X
2821(switch)X
3050(is)X
3123(performed)X
3478(when)X
3672(the)X
3790(kernel,)X
555 2859(using)N
755(the)X
880(contents)X
1174(of)X
1268(the)X
1393(symbolic)X
1713(link)X
1864(the)X
1989(daemon)X
2270(returned,)X
2585(talks)X
2763(to)X
2852(the)X
2976(daemon)X
3256(again,)X
3476(asking)X
3711(it)X
3781(to)X
3869(look)X
555 2955(something)N
908(up)X
1008(in)X
1090(its)X
1185(/.pre\256x)X
1434(directory.)X
555 3108(The)N
700(format)X
934(of)X
1021(the)X
1139(link)X
1283(contents)X
1570(returned)X
1858(by)X
1958(the)X
2076(previous)X
2372(lookup)X
2614(operation)X
2937(is)X
3010(simply)X
7 f
843 3252(/.prefix/)N
2 f
(pre\256x)S
7 f
1458(/)X
2 f
(component)S
1 f
555 3396(where)N
2 f
776(pre\256x)X
1 f
983(is)X
1060(the)X
1182(address)X
1447(of)X
1538(the)X
1660(daemon's)X
1996(internal)X
2265(pre\256x)X
2476(descriptor)X
2821(\(in)X
2934(ascii\))X
3132(and)X
2 f
3272(component)X
1 f
3648(is)X
3725(the)X
3846(com-)X
555 3492(ponent)N
799(the)X
923(kernel)X
1150(was)X
1301(attempting)X
1669(to)X
1757(\256nd)X
1906(within)X
2135(the)X
2258(pre\256x.)X
2490(The)X
2640(daemon)X
2919(delays)X
3149(its)X
3249(search)X
3480(for)X
3599(the)X
3722(server)X
3944(of)X
555 3588(the)N
682(pre\256x)X
898(until)X
1073(the)X
1200(kernel)X
1429(actually)X
1711(attempts)X
2010(to)X
2100(read)X
2267(the)X
2393(symbolic)X
2714(link)X
2866(whose)X
3099(handle)X
3341(it)X
3413(returned,)X
3729(at)X
3815(which)X
555 3684(point)N
740(it)X
805(assumes)X
1093(the)X
1212(kernel)X
1434(must)X
1610(be)X
1707(serious)X
1955(in)X
2038(its)X
2134(desire)X
2347(for)X
2462(the)X
2581(pre\256x,)X
2808(so)X
2899(it)X
2963(broadcasts)X
3322(to)X
3404(the)X
3522(network)X
3805(to)X
3887(\256nd)X
555 3780(the)N
673(server)X
890(of)X
977(the)X
1095(pre\256x,)X
1322(recording)X
1650(the)X
1768(results)X
1997(until)X
2163(the)X
2281(kernel)X
2502(acts)X
2647(on)X
2747(the)X
2865(link)X
3009(contents.)X
555 3933(When)N
773(the)X
897(daemon)X
1177(gets)X
1332(this)X
1473(second)X
1722(lookup)X
1970(request,)X
2248(it)X
2318(can)X
2456(be)X
2558(fairly)X
2758(certain)X
3002(that)X
3147(the)X
3270(pre\256x's)X
3540(mount)X
3769(point)X
3958(is)X
555 4029(free)N
703(and)X
841(attempts)X
1134(to)X
1218(unmount)X
1524(itself)X
1706(from)X
1884(it.)X
1970(Should)X
2217(this)X
2353(fail,)X
2501(the)X
2620(lookup)X
2863(request)X
3116(will)X
3261(return)X
3474(a)X
3531(general)X
3789(system)X
555 4125(error)N
735(to)X
820(the)X
941(kernel,)X
1185(which)X
1403(the)X
1523(kernel)X
1746(will)X
1892(ignore)X
2119(until)X
2287(it's)X
2411(been)X
2585(told)X
2731(enough)X
2989(times)X
3184(\(this)X
3348(is)X
3423(why)X
3583(the)X
3703(broadcast)X
555 4221(for)N
670(the)X
789(pre\256x's)X
1055(server)X
1273(is)X
1347(done)X
1524(when)X
1719(the)X
1838(link)X
1983(is)X
2057(read,)X
2237(not)X
2360(here,)X
2540(since)X
2726(the)X
2845(link)X
2990(read)X
3150(will)X
3294(never)X
3493(fail\).)X
3667(Should)X
3913(the)X
555 4317(daemon)N
835(not)X
963(be)X
1065(able)X
1225(to)X
1313(disentangle)X
1704(itself)X
1890(from)X
2072(the)X
2196(pre\256x,)X
2429(the)X
2553(user)X
2713(will)X
2863(get)X
2987(back)X
3165(a)X
3227(message)X
3524(about)X
3727(a)X
3788(remote)X
555 4413(system)N
797(error)X
974(from)X
1150(the)X
1268(kernel.)X
1509(This)X
1671(rarely)X
1879(happens,)X
2182(however,)X
2499(so)X
2590(you)X
2730(needn't)X
2991(worry)X
3203(too)X
3325(much)X
3523(about)X
3721(it.)X
555 4566(Once)N
752(the)X
877(daemon)X
1158(has)X
1292(unmounted)X
1679(itself,)X
1886(it)X
1957(goes)X
2131(through)X
2407(the)X
2532(process)X
2800(of)X
2894(mounting)X
3227(the)X
3351(remote)X
3600(system)X
3848(in)X
3936(its)X
555 4662(place.)N
769(Again,)X
1009(if)X
1082(this)X
1220(fails,)X
1401(the)X
1522(user)X
1679(will)X
1826(see)X
1952(a)X
2011(message)X
2306(about)X
2507(a)X
2566(remote)X
2812(system)X
3057(error)X
3237(and)X
3376(the)X
3497(daemon)X
3774(will)X
3921(re-)X
555 4758(mount)N
779(itself)X
959(on)X
1059(the)X
1177(pre\256x)X
1384(so)X
1475(the)X
1593(user)X
1747(can)X
1879(try)X
1988(again)X
2182(once)X
2354(s/he's)X
2561(\(or)X
2675(you've\))X
2945(straightened)X
3357(things)X
3572(out.)X
555 4911(If)N
636(the)X
761(remote)X
1011(mount)X
1242(succeeds,)X
1575(the)X
1700(kernel)X
1928(is)X
2008(told)X
2159(that)X
2306(the)X
2431(component)X
2814(it)X
2885(is)X
2965(seeking)X
3237(\(which)X
3487(will)X
3638(be)X
3740(the)X
3864(ascii)X
555 5007(representation)N
1037(of)X
1131(the)X
1256(pre\256x's)X
1528(address\))X
1823(is)X
1902(also)X
2057(a)X
2119(symbolic)X
2438(link,)X
2608(but)X
2736(this)X
2877(one's)X
3077(back)X
3255(to)X
3343(the)X
3467(pre\256x)X
3680(itself.)X
3886(The)X
555 5103(kernel)N
788(performs)X
1110(the)X
1240(same)X
1437(lookup)X
1691(it)X
1767(did)X
1901(before,)X
2158(since)X
2354(the)X
2483(daemon)X
2768(tacked)X
3009(the)X
3138(component)X
3525(the)X
3654(kernel)X
3886(was)X
555 5199(seeking)N
824(onto)X
990(the)X
1112(end)X
1252(of)X
1343(the)X
1464(link)X
1611(contents)X
1901(it)X
1968(returned,)X
2279(but)X
2404(this)X
2542(time)X
2707(the)X
2828(request)X
3083(goes)X
3253(to)X
3338(the)X
3459(real)X
3603(server)X
3823(of)X
3913(the)X
555 5295(\256lesystem.)N
555 5448(A)N
637(mounted)X
941(pre\256x)X
1152(is)X
1229(displayed)X
1560(in)X
1646(/etc/mtab)X
1968(in)X
2054(a)X
2114(slightly)X
2377(different)X
2678(format)X
2915(than)X
3076(usual.)X
3288(Rather)X
3525(than)X
3686(giving)X
3913(the)X
555 5544(server)N
782(as)X
879(``)X
2 f
933(server)X
1 f
1134(:)X
2 f
1156(directory)X
1 f
1450('',)X
1554(I)X
1611(have)X
1793(chosen)X
2045(to)X
2136(display)X
2396(it)X
2469(as)X
2565(``)X
2 f
2619(server)X
1 f
2820([)X
2 f
2847(directory)X
1 f
3141(]'')X
3251(in)X
3342(an)X
3447(attempt)X
3716(to)X
3807(distin-)X
555 5640(guish)N
748(between)X
1036(permanent)X
1395(systems)X
1668(people)X
1902(are)X
2021(used)X
2188(to)X
2270(and)X
2406(the)X
2524(transient)X
2820(systems)X
3093(that)X
3233(are)X
3352(pre\256xes.)X

6 p
%%Page: 6 7
10 s 10 xH 0 xS 1 f
2216 384(-)N
2263(6)X
2323(-)X
3 f
555 672(5.2.)N
715(Unmounting)X
1 f
555 825(Once)N
749(the)X
871(pre\256x)X
1082(is)X
1159(mounted)X
1463(on)X
1567(a)X
1627(remote)X
1874(\256lesystem,)X
2242(the)X
2364(question)X
2659(arises)X
2866(of)X
2957(when)X
3155(to)X
3241(unmount)X
3548(it.)X
3635(The)X
3783(goal)X
3944(of)X
555 921(reducing)N
863(a)X
926(machine's)X
1283(vulnerability)X
1719(to)X
1808(server)X
2032(crashes)X
2296(would)X
2522(certainly)X
2829(not)X
2957(be)X
3059(met)X
3205(if)X
3280(all)X
3386(pre\256xes)X
3666(ever)X
3831(refer-)X
555 1017(enced)N
769(by)X
875(a)X
937(machine)X
1235(were)X
1418(to)X
1505(remain)X
1753(mounted,)X
2078(nor)X
2210(would)X
2435(administrators)X
2918(be)X
3019(able)X
3178(to)X
3265(switch)X
3499(the)X
3622(serving)X
3883(of)X
3975(a)X
555 1113(pre\256x)N
764(to)X
848(another)X
1111(machine)X
1405(very)X
1570(easily.)X
1799(To)X
1909(avoid)X
2108(this,)X
2264(pre\256x)X
2472(will)X
2617(attempt)X
2878(to)X
2961(unmount)X
3266(a)X
3323(mounted)X
3624(pre\256x)X
3832(every)X
555 1209(ten)N
680(minutes)X
960(or)X
1054(so.)X
1172(If)X
1253(the)X
1378(attempt)X
1645(is)X
1725(successful,)X
2102(the)X
2227(entry)X
2419(is)X
2499(removed)X
2807(from)X
2989(/etc/mtab)X
3313(and)X
3455(the)X
3579(daemon)X
3859(once)X
555 1305(again)N
749(takes)X
934(over)X
1097(the)X
1215(mount)X
1439(point.)X
555 1458(This)N
721(solution)X
1002(isn't)X
1168(ideal)X
1348(of)X
1439(course,)X
1693(as)X
1784(one)X
1924(would)X
2148(much)X
2350(prefer)X
2567(to)X
2653(unmount)X
2961(a)X
3021(pre\256x)X
3232(only)X
3398(ten)X
3520(minutes)X
3796(after)X
3967(it)X
555 1554(was)N
704(last)X
839(accessed,)X
1165(for)X
1283(instance,)X
1590(rather)X
1802(than)X
1964(whenever)X
2301(it's)X
2427(possible)X
2713(at)X
2795(a)X
2854(ten)X
2975(minute)X
3220(interval.)X
3528(Even)X
3716(if)X
3788(you've)X
555 1650(got)N
680(a)X
739(program)X
1034(referencing)X
1424(a)X
1483(pre\256x)X
1693(every)X
1895(\256ve)X
2038(seconds,)X
2335(it)X
2402(could)X
2603(still)X
2745(be)X
2844(unmounted)X
3227(if)X
3299(the)X
3419(program)X
3713(is)X
3788(merely)X
555 1746(stat'ing)N
820(a)X
881(\256le,)X
1028(for)X
1147(example,)X
1464(rather)X
1677(than)X
1840(keeping)X
2119(a)X
2180(\256le)X
2307(open.)X
2508(To)X
2622(allow)X
2825(the)X
2948(daemon)X
3227(access)X
3458(to)X
3545(the)X
3667(last-access)X
555 1842(time)N
719(of)X
808(a)X
866(pre\256x)X
1075(would)X
1297(either)X
1502(require)X
1752(special)X
1997(kernel)X
2220(support)X
2482(\(stat'ing)X
2771(the)X
2891(root)X
3042(of)X
3131(the)X
3251(\256lesystem)X
3596(doesn't)X
3853(help,)X
555 1938(since)N
746(the)X
870(access)X
1102(time)X
1270(of)X
1363(the)X
1487(directory)X
1803(isn't)X
1971(modi\256ed)X
2281(when)X
2480(a)X
2541(directory)X
2856(search)X
3087(is)X
3165(performed;)X
3547(only)X
3714(when)X
3913(the)X
555 2034(directory)N
871(is)X
950(actually)X
1229(read\),)X
1440(or)X
1532(it)X
1601(would)X
1826(force)X
2017(the)X
2140(daemon)X
2419(to)X
2506(act)X
2625(as)X
2717(an)X
2818(intermediary)X
3253(between)X
3546(the)X
3669(kernel)X
3895(and)X
555 2130(the)N
679(remote)X
928(server,)X
1171(an)X
1273(alternative)X
1638(I)X
1691(deemed)X
1967(too)X
2095(costly,)X
2332(given)X
2536(the)X
2660(extra)X
2847(copies)X
3078(required)X
3372(on)X
3478(reads)X
3674(and)X
3815(writes)X
555 2226(\(for)N
697(a)X
754(write,)X
960(the)X
1079(data)X
1234(would)X
1455(\257ow)X
1618(from)X
1795(the)X
1914(client)X
2113(to)X
2196(the)X
2315(kernel)X
2537(to)X
2620(the)X
2739(daemon)X
3014(to)X
3097(the)X
3216(kernel)X
3438(to)X
3521(the)X
3639(remote)X
3882(sys-)X
555 2322(tem,)N
715(causing)X
980(an)X
1076(extra)X
1257(two)X
1397(copies,)X
1642(usually)X
1893(of)X
1980(8K)X
2098(each)X
2266(or)X
2353(more\).)X
3 f
555 2514(5.3.)N
715(Miscellaneous)X
1 f
555 2667(The)N
709(daemon)X
992(actually)X
1275(runs)X
1442(as)X
1538(two)X
1687(processes,)X
2044(one)X
2189(of)X
2285(which)X
2510(services)X
2798(the)X
2924(various)X
3188(mount)X
3420(points)X
3643(and)X
3787(is)X
3868(gen-)X
555 2763(erally)N
764(in)X
852(charge)X
1093(of)X
1186(everything.)X
1575(The)X
1726(other)X
1917(process)X
2184(performs)X
2500(all)X
2606(the)X
2730(mount)X
2960(and)X
3102(unmount)X
3411(system)X
3658(calls)X
3830(at)X
3913(the)X
555 2859(behest)N
792(of)X
891(the)X
1021(\256rst)X
1176(process)X
1448(\(actually,)X
1780(the)X
1909(one)X
2056(with)X
2229(the)X
2358(higher)X
2594(PID)X
2754(is)X
2838(the)X
2967(one)X
3114(that's)X
3323(in)X
3416(charge,)X
3682(but)X
3815(who's)X
555 2955(counting?\).)N
555 3108(The)N
706(daemon)X
986(needs)X
1195(to)X
1283(be)X
1384(in)X
1471(two)X
1616(pieces)X
1842(so)X
1938(the)X
2061(kernel)X
2287(can)X
2424(check)X
2637(out)X
2764(pre\256xes)X
3043(during)X
3277(mounting)X
3608(or)X
3700(unmount-)X
555 3204(ing,)N
698(especially)X
1040(for)X
1155(the)X
1274(children)X
1558(of)X
1646(a)X
1703(root)X
1853(pre\256x)X
2061(\(in)X
2170(order)X
2360(to)X
2442(mount)X
2666(the)X
2784(remote)X
3027(system)X
3269(on)X
3369(a)X
3425(subpre\256x,)X
3763(the)X
3881(ker-)X
555 3300(nel)N
673(must)X
848(request)X
1100(a)X
1156(\256le)X
1278(handle)X
1512(from)X
1688(the)X
1806(root)X
1955(pre\256x.)X
2182(Since)X
2380(the)X
2498(daemon)X
2772(can't)X
2953(run)X
3080(to)X
3162(\256eld)X
3324(this)X
3459(request)X
3711(while)X
3909(it's)X
555 3396(in)N
637(the)X
755(kernel)X
976(performing)X
1357(the)X
1475(mount,)X
1719(having)X
1957(only)X
2119(a)X
2175(single)X
2386(process)X
2647(would)X
2867(cause)X
3066(instant,)X
3319(unending)X
3637(deadlock\).)X
3 f
555 3588(6.)N
655(Quirks)X
1 f
555 3741(There)N
767(are)X
890(a)X
950(few)X
1095(non-obvious)X
1519(pieces)X
1744(of)X
1835(behaviour)X
2180(exhibited)X
2502(by)X
2606(a)X
2665(system)X
2910(running)X
3182(this)X
3320(daemon.)X
3617(One)X
3774(already)X
555 3837(mentioned)N
914(is)X
988(the)X
1107(tendency)X
1418(to)X
1501(generate)X
1795(strange)X
2048(error)X
2226(messages)X
2550(should)X
2784(a)X
2841(pre\256x)X
3049(be)X
3146(unmountable.)X
3605(As)X
3714(an)X
3810(exam-)X
555 3933(ple,)N
693(the)X
811(command)X
7 f
843 4077(cd)N
987(/n/fishnet/biscuit)X
1 f
555 4221(will)N
699(yield)X
879(the)X
997(following)X
1328(two)X
1468(enlightening)X
1888(messages:)X
7 f
843 4365(NFS)N
1035(readlink)X
1467(failed)X
1803(for)X
1995(server)X
2331(prefix:)X
2715(RPC:)X
2955(Remote)X
3291(system)X
3627(error)X
843 4461(/n/fishnet/biscuit:)N
1803(Unknown)X
2187(error)X
1 f
555 4662(Another)N
844(quirk)X
1038(is)X
1116(exhibited)X
1439(by)X
1544(doing)X
1751(and)X
1892(``ls)X
2024(-l'')X
2152(of)X
2244(anything)X
2549(under)X
2757(an)X
2858(as-yet)X
3075(unmounted)X
3460(pre\256x.)X
3692(While)X
3913(the)X
555 4758(pre\256x)N
762(will)X
906(indeed)X
1140(be)X
1236(mounted)X
1536(by)X
1636(this)X
1771(operation,)X
2114(the)X
2232(nature)X
2453(of)X
2540(ls)X
2613(-l)X
2682(will)X
2826(cause)X
3025(output)X
3249(like)X
3389(this)X
7 f
555 4902(lrwxrwxrwx)N
1131(1)X
1227(root)X
1515(23)X
1659(Dec)X
1851(31)X
2043(1969)X
2283(/usr/src/public)X
3051(->)X
3195(/.prefix/164480/public)X
1 f
555 5046(to)N
637(be)X
733(displayed,)X
1080(to)X
1162(the)X
1280(edi\256cation)X
1638(of)X
1725(no)X
1825(one.)X
3 f
555 5238(7.)N
655(Extensions)X
1 f
555 5391(This)N
718(service)X
967(is)X
1041(by)X
1142(no)X
1243(means)X
1469(complete.)X
1804(The)X
1950(understanding)X
2425(of)X
2513(the)X
2632(access)X
2859(controls)X
3138(in)X
3221(/etc/exports)X
3616(for)X
3730(exported)X
555 5487(pre\256xes)N
832(is)X
908(an)X
1007(example)X
1302(of)X
1392(a)X
1451(worthwhile)X
1839(extension,)X
2189(as)X
2279(would)X
2502(an)X
2601(additional)X
2944(call)X
3083(to)X
3168(force)X
3357(all)X
3459(active)X
3673(pre\256xes)X
3949(to)X
555 5583(unmount)N
868(a)X
933(pre\256x,)X
1169(if)X
1247(possible,)X
1558(to)X
1649(allow)X
1855(an)X
1959(administrator)X
2414(to)X
2504(switch)X
2741(the)X
2867(server)X
3092(of)X
3187(a)X
3251(pre\256x.)X
3486(Unfortunately,)X
3984(I)X
555 5679(have)N
737(little)X
913(time)X
1085(to)X
1177(implement)X
1549(such)X
1726(extensions)X
2094(\(nor)X
2258(do)X
2368(I)X
2425(need)X
2606(them)X
2795(at)X
2882(the)X
3009(moment\).)X
3367(Ideas,)X
3586(however,)X
3912(are)X
555 5775(always)N
798(welcome,)X
1128(especially)X
1469(if)X
1538(accompanied)X
1982(by)X
2082(code.)X

1 p
%%Page: 1 8
10 s 10 xH 0 xS 1 f
3 f
12 s
1918 960(Table)N
2177(of)X
2281(Contents)X
1 f
10 s
555 1401(1.)N
635(Introduction)X
1051(..........................................................................................................................................)X
3971(1)X
555 1554(2.)N
635(Operation)X
991(.............................................................................................................................................)X
3971(2)X
555 1707(3.)N
635(Options)X
911(.................................................................................................................................................)X
3971(2)X
555 1860(4.)N
635(Installation)X
1031(...........................................................................................................................................)X
3971(3)X
555 2013(5.)N
635(Internals)X
951(...............................................................................................................................................)X
3971(4)X
755 2109(5.1.)N
895(Mounting)X
1230(\320)X
1330(The)X
1475(Fun)X
1619(Begins)X
1871(.................................................................................................)X
3971(5)X
755 2205(5.2.)N
895(Unmounting)X
1331(............................................................................................................................)X
3971(6)X
755 2301(5.3.)N
895(Miscellaneous)X
1391(.........................................................................................................................)X
3971(6)X
555 2454(6.)N
635(Quirks)X
891(..................................................................................................................................................)X
3971(6)X
555 2607(7.)N
635(Extensions)X
1011(............................................................................................................................................)X
3971(6)X

8 p
%%Trailer
xt

xs
