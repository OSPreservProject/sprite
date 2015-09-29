%!PS-Adobe-1.0
%%Creator: sniff:adam (Adam de Boor,Ext. 238,,5492264)
%%Title: stdin (ditroff)
%%CreationDate: Sat Jan 21 18:28:56 1989
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

%%Page: 1 1
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
3 f
10 s
460 784(NAME)N
1 f
11 s
748 888(pmake)N
9 f
1005(-)X
1 f
1075(create)X
1307(programs)X
1661(in)X
1752(parallel)X
3 f
10 s
460 1040(SYNOPSIS)N
11 s
748 1144(pmake)N
1 f
1029([)X
3 f
9 f
1058(-)X
3 f
1106(d)X
2 f
1182(what)X
1 f
1354(])X
1410([)X
3 f
9 f
1439(-)X
3 f
1487(f)X
2 f
1543(make\256le)X
1 f
1836(])X
1892([)X
3 f
9 f
1921(-)X
3 f
1969(h)X
1 f
2018(])X
2074([)X
3 f
9 f
2103(-)X
3 f
2151(i)X
1 f
2176(])X
2232([)X
3 f
9 f
2261(-)X
3 f
2309(k)X
1 f
2358(])X
2414([)X
3 f
9 f
2443(-)X
3 f
2491(l)X
1 f
2516(])X
2572([)X
3 f
9 f
2601(-)X
3 f
2649(n)X
1 f
2698(])X
2754([)X
3 f
9 f
2783(-)X
3 f
2831(p)X
2 f
2907(#)X
1 f
(])S
3007([)X
3 f
9 f
3036(-)X
3 f
3084(q)X
1 f
3133(])X
3189([)X
3 f
9 f
3218(-)X
3 f
3266(r)X
1 f
3305(])X
3360([)X
3 f
9 f
3389(-)X
3 f
3437(s)X
1 f
3471(])X
3526([)X
3 f
9 f
3555(-)X
3 f
3603(t)X
1 f
3632(])X
3687([)X
3 f
9 f
3716(-)X
3 f
3764(x)X
1 f
(])S
3863([)X
3 f
9 f
3892(-)X
3 f
3940(v)X
1 f
(])S
4039([)X
3 f
9 f
4068(-)X
3 f
4116(B)X
1 f
4175(])X
1036 1248([)N
3 f
9 f
1065(-)X
3 f
1113(C)X
1 f
1176(])X
1266([)X
3 f
9 f
1295(-)X
3 f
1343(D)X
2 f
1467(variable)X
1 f
1761(])X
1851([)X
3 f
9 f
1880(-)X
3 f
1928(I)X
2 f
2023(directory)X
1 f
2346(])X
2436([)X
3 f
9 f
2465(-)X
3 f
2513(J)X
2 f
2618(#)X
1 f
(])S
2752([)X
3 f
9 f
2781(-)X
3 f
2829(L)X
2 f
2949(#)X
1 f
(])S
3083([)X
3 f
9 f
3112(-)X
3 f
3160(M)X
1 f
3243(])X
3332([)X
3 f
9 f
3361(-)X
3 f
3409(P)X
1 f
3463(])X
3552([)X
3 f
9 f
3581(-)X
3 f
3629(V)X
1 f
3692(])X
3781([)X
3 f
9 f
3810(-)X
3 f
3858(W)X
1 f
(])S
4035([)X
3 f
9 f
4064(-)X
3 f
4112(X)X
1 f
4175(])X
1036 1352([)N
2 f
1065(VAR1)X
3 f
1278(=)X
2 f
1328(value1)X
1 f
1563(])X
1614([)X
2 f
1643(VAR2)X
3 f
1856(=)X
2 f
1906(value2)X
3 f
2148(...)X
1 f
(])S
2265([)X
2 f
2294(targ1)X
1 f
2485(])X
2536([)X
2 f
2565(targ2)X
2778(...)X
1 f
(])S
10 f
394 1456(c)N
1420(c)Y
1332(c)Y
1244(c)Y
1156(c)Y
1068(c)Y
980(c)Y
892(c)Y
804(c)Y
716(c)Y
628(i)Y
398(iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii)X
716(c)Y
752(c)Y
840(c)Y
928(c)Y
1016(c)Y
1104(c)Y
1192(c)Y
1280(c)Y
1368(c)Y
1456(c)Y
394(i)X
398(iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii)X
3 f
10 s
460 1608(ARGUMENTS)N
1 f
11 s
9 f
748 1712(-)N
1 f
796(d)X
2 f
1036(what)X
1 f
1760(Specify)X
2052(what)X
2246(modules)X
2567(should)X
2824(print)X
3013(debugging)X
3407(information.)X
2 f
3890(what)X
1 f
4084(is)X
4165(a)X
1760 1816(string)N
1983(of)X
2078(letters)X
2316(from)X
2509(the)X
2639(following)X
3004(set:)X
3149(a,)X
3232(c,)X
3315(d,)X
3403(j,)X
3472(m,)X
3585(p,)X
3673(r,)X
3746(s,)X
3824(t,)X
3893(v.)X
9 f
748 1920(-)N
1 f
796(f)X
2 f
1036(make\256le)X
1 f
1760(Specify)X
2054(a)X
2118(different)X
2446(make\256le)X
2775(to)X
2869(read)X
3045(than)X
3222(the)X
3355(standard)X
3678(``Make\256le'')X
4131(or)X
1760 2024(``make\256le''.)N
2246(If)X
2 f
2326(make\256le)X
1 f
2641(is)X
2722("-",)X
2867(reads)X
3074(from)X
3267(standard)X
3587(input.)X
9 f
748 2128(-)N
1 f
796(h)X
1760(Prints)X
1988(out)X
2123(help)X
2297(information.)X
9 f
748 2232(-)N
1 f
796(i)X
1760(``Ignore)X
2069(errors'')X
2353(--)X
2433(ignore)X
2680(non-zero)X
3014(exit)X
3169(statuses)X
3465(of)X
3560(commands.)X
9 f
748 2336(-)N
1 f
796(k)X
1760(``Keepgoing'')X
2296(--)X
2388(if)X
2476(an)X
2592(error)X
2795(is)X
2887(encountered,)X
3372(keep)X
3571(working)X
3897(on)X
4018(those)X
1760 2440(parts)N
1953(of)X
2048(the)X
2178(input)X
2382(graph)X
2604(that)X
2759(are)X
2888(not)X
3023(affected)X
3328(by)X
3438(the)X
3568(error.)X
9 f
748 2544(-)N
1 f
796(l)X
1760(PMake)X
2041(has)X
2190(the)X
2330(ability)X
2589(to)X
2690(lock)X
2874(a)X
2945(directory)X
3294(against)X
3575(other)X
3787(people)X
4053(exe-)X
1760 2648(cuting)N
2047(it)X
2163(in)X
2298(the)X
2472(same)X
2718(directory)X
3101(\(by)X
3283(means)X
3573(of)X
3711(a)X
3815(\256le)X
3993(called)X
1760 2752(``LOCK.make'')N
2356(that)X
2517(it)X
2595(creates)X
2867(and)X
3022(checks)X
3289(for)X
3419(in)X
3516(the)X
3651(directory\).)X
4047(This)X
1760 2856(is)N
1857(a)X
1934(Good)X
2166(Thing)X
2414(because)X
2729(two)X
2898(people)X
3170(doing)X
3408(the)X
3553(same)X
3771(thing)X
3990(in)X
4096(the)X
1760 2960(same)N
1972(place)X
2189(can)X
2342(be)X
2456(disastrous)X
2839(for)X
2972(the)X
3111(\256nal)X
3299(product)X
3599(\(too)X
3772(many)X
3999(cooks)X
1760 3064(and)N
1910(all)X
2022(that\).)X
2251(Whether)X
2577(this)X
2728(locking)X
3016(is)X
3097(the)X
3227(default)X
3494(is)X
3575(up)X
3685(to)X
3776(your)X
3959(system)X
1760 3168(administrator.)N
2291(If)X
2387(locking)X
2690(is)X
2787(on,)X
3 f
9 f
2935(-)X
3 f
2983(l)X
1 f
3046(will)X
3222(turn)X
3401(it)X
3488(off,)X
3649(and)X
3813(vice)X
3997(versa.)X
1760 3272(Note)N
1969(that)X
2139(this)X
2304(locking)X
2606(will)X
2781(not)X
2931(prevent)X
2 f
3232(you)X
1 f
3396(from)X
3604(invoking)X
3955(PMake)X
1760 3376(twice)N
1977(in)X
2072(the)X
2206(same)X
2413(place)X
2625(--)X
2709(if)X
2789(you)X
2946(own)X
3122(the)X
3255(lock)X
3432(\256le,)X
3592(PMake)X
3866(will)X
4029(warn)X
1760 3480(you)N
1914(about)X
2132(it)X
2204(but)X
2339(continue)X
2665(to)X
2756(execute.)X
9 f
748 3584(-)N
1 f
796(n)X
1760(``No)X
1952(execute'')X
2306(--)X
2391(do)X
2506(not)X
2646(execute)X
2942(commands.)X
3394(Just)X
3557(print)X
3750(the)X
3884(ones)X
4071(that)X
1760 3688(would)N
2002(be)X
2107(executed.)X
9 f
748 3792(-)N
1 f
796(p)X
2 f
1036(#)X
1 f
1760(Tell)X
2 f
1929(PMake)X
1 f
2204(if)X
2284(and)X
2437(when)X
2653(to)X
2748(print)X
2941(the)X
3075(input)X
3283(graph.)X
3553(The)X
3716(number)X
4011(is)X
4096(the)X
1760 3896(bitwise)N
2038(OR)X
2183(of)X
2279(the)X
2410(numbers)X
2736(1)X
2803(and)X
2953(2)X
3020(with)X
3200(1)X
3267(meaning)X
3594(to)X
3685(print)X
3874(the)X
4004(graph)X
1760 4000(before)N
2010(making)X
2301(anything)X
2636(and)X
2789(2)X
2859(meaning)X
3189(to)X
3284(print)X
3477(the)X
3611(graph)X
3837(after)X
4023(mak-)X
1760 4104(ing)N
1895(everything.)X
2338(If)X
2418(no)X
2528(number)X
2819(is)X
2900(given,)X
3140(it)X
3212(defaults)X
3513(to)X
3604(3.)X
9 f
748 4208(-)N
1 f
796(q)X
1760(``Query'')X
2125(--)X
2213(do)X
2331(not)X
2474(execute)X
2773(any)X
2930(commands.)X
3386(Just)X
3553(exit)X
3715(0)X
3788(if)X
3871(the)X
4008(given)X
1760 4312(target\(s\))N
2075(is)X
2156(\(are\))X
2343(up)X
2453(to)X
2544(date)X
2713(and)X
2862(exit)X
3017(non-zero)X
3351(otherwise.)X
9 f
748 4416(-)N
1 f
796(r)X
1760(``Remove)X
2145(built-in)X
2438(rules'')X
2699(--)X
2789(do)X
2909(not)X
3054(parse)X
3271(the)X
3411(built-in)X
3704(rules)X
3907(given)X
4135(in)X
1760 4520(the)N
1890(system)X
2157(make\256le.)X
9 f
748 4624(-)N
1 f
796(s)X
1760(``Silence'')X
2158(--)X
2238(do)X
2348(not)X
2483(echo)X
2671(commands)X
3075(as)X
3170(they)X
3344(are)X
3473(executed.)X
9 f
748 4728(-)N
1 f
796(t)X
1760(``Touch)X
2074(targets'')X
2398(--)X
2486(rather)X
2721(than)X
2903(executing)X
3276(the)X
3414(commands)X
3826(to)X
3925(create)X
4165(a)X
1760 4832(target,)N
2025(just)X
2195(change)X
2486(its)X
2612(modi\256cation)X
3101(time)X
3301(so)X
3420(it)X
3511(appears)X
3820(up-to-date.)X
1760 4936(This)N
1939(is)X
2020(dangerous.)X
9 f
748 5040(-)N
1 f
796(x)X
1760(``Export'')X
2169(--)X
2280(causes)X
2562(commands)X
2996(to)X
3117(be)X
3252(exported)X
3612(when)X
3854(in)X
3975(Make-)X
1760 5144 0.3177(compatibility)AN
2272(mode.)X
2529(Since)X
2763(exporting)X
3139(commands)X
3559(in)X
3666(this)X
3832(mode)X
4066(will)X
1760 5248(often)N
1972(take)X
2150(longer)X
2406(than)X
2589(running)X
2894(them)X
3102(on)X
3220(the)X
3358(local)X
3560(machine,)X
3911(exporta-)X
1760 5352(tion)N
1920(is)X
2001(off)X
2125(by)X
2235(default)X
2502(and)X
2651(must)X
2845(be)X
2950(turned)X
3197(on)X
3307(using)X
3520(this)X
3670(\257ag.)X
9 f
748 5456(-)N
1 f
796(v)X
1760(``System)X
2113(V'')X
2269(--)X
2362(invokes)X
2671 0.3177(compatibility)AX
3179(functions)X
3542(suitable)X
3852(for)X
3988(acting)X
1760 5560(like)N
1925(the)X
2065(System)X
2357(V)X
2452(version)X
2743(of)X
2848(Make.)X
3102(This)X
3291(implies)X
3 f
9 f
3584(-)X
3 f
3632(B)X
3723(,)X
1 f
3777(and)X
3 f
9 f
3935(-)X
3 f
3983(V)X
1 f
4077(and)X
1760 5664(turns)N
1961(off)X
2088(directory)X
2431(locking.)X
2743(Locking)X
3062(may)X
3239(be)X
3347(turned)X
3597(back)X
3788(on)X
3901(again)X
4116(by)X
1760 5768(giving)N
2008(the)X
3 f
9 f
2138(-)X
3 f
2186(l)X
1 f
2233(\257ag)X
2387(after)X
3 f
9 f
2570(-)X
3 f
2618(v)X
2684(.)X
1 f
10 s
460 6160(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(1)X

2 p
%%Page: 2 2
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
9 f
748 784(-)N
1 f
796(B)X
1760(``Backwards-compatible'')X
2711(--)X
2791(performs)X
3130(as)X
3225(much)X
3443(like)X
3598(Make)X
3820(as)X
3915(possible)X
1760 888(\(including)N
2146(executing)X
2512(a)X
2574(single)X
2807(shell)X
2996(per)X
3130(command)X
3500(and)X
3649(expanding)X
4038(vari-)X
1760 992(ables)N
1963(as)X
2058(Make)X
2280(did\))X
2444(while)X
2662(still)X
2818(performing)X
3236(in)X
3327(parallel.)X
9 f
748 1096(-)N
1 f
796(C)X
1760(``Non-compatible'')X
2480(--)X
2569(turns)X
2776(off)X
2909(all)X
3029 0.3177(compatibility)AX
3533(speci\256ed)X
3877(up)X
3996(to)X
4096(the)X
1760 1200(point)N
1964(at)X
2050(which)X
3 f
9 f
2287(-)X
3 f
2335(C)X
1 f
2420(is)X
2501(encountered.)X
9 f
748 1304(-)N
1 f
796(D)X
2 f
1036(variable)X
1 f
1760(De\256nes)X
2050(the)X
2180(given)X
2398(variable)X
2704(to)X
2795(be)X
3 f
2900(1)X
1 f
2966(in)X
3057(the)X
3187(global)X
3430(context.)X
9 f
748 1408(-)N
1 f
796(I)X
2 f
1036(directory)X
1 f
1760(Specify)X
2057(another)X
2348(directory)X
2693(in)X
2789(which)X
3031(to)X
3127(look)X
3311(for)X
3440(#include'd)X
3844(make\256les.)X
1760 1512(This)N
1939(\257ag)X
2093(may)X
2267(be)X
2372(repeated)X
2692(as)X
2787(many)X
3005(times)X
3219(as)X
3314(necessary.)X
9 f
748 1616(-)N
1 f
796(J)X
2 f
1036(#)X
1 f
1760(Specify)X
2051(the)X
2181(maximum)X
2562(number)X
2853(of)X
2948(jobs)X
3117(to)X
3208(run)X
3347(at)X
3433(once)X
3621(on)X
3731(all)X
3842(machines.)X
9 f
748 1720(-)N
1 f
796(L)X
2 f
1036(#)X
1 f
1760(Specify)X
2051(the)X
2181(maximum)X
2562(number)X
2853(of)X
2948(jobs)X
3117(to)X
3208(run)X
3347(locally.)X
9 f
748 1824(-)N
1 f
796(M)X
1760(Be)X
1888(as)X
1991(much)X
2217(like)X
2380(Make)X
2610(as)X
2712(possible.)X
3052(No)X
3188(parallel)X
3482(execution.)X
3876(Old-style)X
1760 1928(variable)N
2066(expansion.)X
2467(One)X
2635(shell)X
2824(per)X
2958(command.)X
3350(Etc.)X
9 f
748 2032(-)N
1 f
796(P)X
1760(``Don't)X
2045(use)X
2184(Pipes'')X
2455(--)X
2535(see)X
2669(the)X
2799(section)X
3071(on)X
3 f
3181(OUTPUT)X
1 f
3548(.)X
9 f
748 2136(-)N
1 f
796(V)X
1760(``Do)X
1954(old-style)X
2292(variable)X
2605(expansion'')X
3049(--)X
3136(expands)X
3453(an)X
3565(unknown)X
3920(variable)X
1760 2240(to)N
1851(the)X
1981(empty)X
2224(string.)X
9 f
748 2344(-)N
1 f
796(W)X
1760(Don't)X
1987(print)X
2176(warning)X
2486(messages.)X
9 f
748 2448(-)N
1 f
796(X)X
1760(``No)X
1949(Export'')X
2271(--)X
2353(prohibits)X
2691(exportation.)X
9 f
3139(-)X
1 f
3187(x)X
3255(and)X
9 f
3406(-)X
1 f
3454(X)X
3541(should)X
3800(not)X
3937(be)X
4043(used)X
1760 2552(in)N
1851(the)X
1981(same)X
2184(command.)X
748 2656(VAR=value)N
1760(Set)X
1906(the)X
2047(value)X
2271(of)X
2377(the)X
2518(variable)X
3 f
2835(VAR)X
1 f
3057(to)X
3159(the)X
3300(given)X
3529(value.)X
3796(This)X
3985(super-)X
1760 2760(cedes)N
1979(any)X
2130(value)X
2345(assigned)X
2672(to)X
2765(the)X
2897(variable)X
3205(in)X
3298(the)X
3430(make\256le.)X
3802(See)X
3 f
3952(VARI-)X
1760 2864(ABLES)N
1 f
2049(.)X
3 f
10 s
460 3016(DESCRIPTION)N
2 f
11 s
748 3120(PMake)N
1 f
1023(is)X
1108(a)X
1173(program)X
1497(designed)X
1836(to)X
1931(make)X
2148(the)X
2282(maintenance)X
2753(of)X
2851(other)X
3057(programs)X
3414(much)X
3635(easier.)X
3909(Its)X
4022(input)X
748 3224(is)N
834(a)X
900(``make\256le'')X
1347(that)X
1507(speci\256es)X
1837(which)X
2079(\256les)X
2253(depend)X
2534(on)X
2649(which)X
2891(other)X
3099(\256les)X
3273(and)X
3427(what)X
3625(to)X
3721(do)X
3835(about)X
4057(\256les)X
748 3328(that)N
913(are)X
1052(``out-of-date.'')X
1613(If)X
1703(you)X
1867(don't)X
2085(specify)X
2371(a)X
2442(make\256le)X
2778(to)X
2879(read,)X
3 f
3083(Make\256le)X
1 f
3442(and)X
3 f
3600(make\256le)X
1 f
3918(,)X
3971(in)X
4071(that)X
748 3432(order,)N
977(are)X
1106(looked)X
1368(for)X
1492(and)X
1641(read)X
1814(if)X
1890(they)X
2064(exist.)X
748 3584(This)N
944(manual)X
1243(page)X
1448(is)X
1545(meant)X
1799(to)X
1906(be)X
2027(a)X
2104(reference)X
2468(page)X
2672(only.)X
2889(For)X
3049(a)X
3126(more)X
3345(thorough)X
3701(description)X
4131(of)X
2 f
748 3688(PMake)N
1 f
1004(,)X
1048(please)X
1290(refer)X
1477(to)X
2 f
1568(PMake)X
1839(--)X
1919(A)X
1995(Tutorial)X
1 f
2285(\(available)X
2655(in)X
2746(this)X
2896(distribution\).)X
748 3840(There)N
975(are)X
1104(four)X
1272(basic)X
1475(types)X
1683(of)X
1778(lines)X
1967(in)X
2058(a)X
2119(make\256le:)X
1036 3992(1\))N
1324(File)X
1484(dependency)X
1926(speci\256cations)X
1036 4144(2\))N
1324(Creation)X
1650(commands)X
1036 4296(3\))N
1324(Variable)X
1649(assignments)X
1036 4448(4\))N
1324(Comments,)X
1751(include)X
2033(statements)X
2428(and)X
2577(conditional)X
2997(directives)X
748 4600(Any)N
923(line)X
1080(may)X
1256(be)X
1363(continued)X
1735(over)X
1915(multiple)X
2235(lines)X
2426(by)X
2538(ending)X
2802(it)X
2876(with)X
3057(a)X
3120(backslash.)X
3530(The)X
3690(backslash,)X
4077(fol-)X
748 4704(lowing)N
1029(newline)X
1344(and)X
1507(any)X
1670(initial)X
1914(whitespace)X
2341(on)X
2465(the)X
2608(following)X
2986(line)X
3154(are)X
3296(compressed)X
3746(into)X
3919(a)X
3993(single)X
748 4808(space.)N
3 f
10 s
460 4960(DEPENDENCY)N
1049(LINES)X
1 f
11 s
748 5064(On)N
885(a)X
954(dependency)X
1404(line,)X
1589(there)X
1795(are)X
1932(targets,)X
2219(sources)X
2512(and)X
2669(an)X
2782(operator.)X
3149(The)X
3316(targets)X
3580(``depend'')X
3979(on)X
4096(the)X
748 5168(sources)N
1038(and)X
1192(are)X
1325(usually)X
1606(created)X
1886(from)X
2083(them.)X
2330(Any)X
2507(number)X
2802(of)X
2901(targets)X
3162(and)X
3315(sources)X
3604(may)X
3782(be)X
3891(speci\256ed)X
748 5272(on)N
860(a)X
923(dependency)X
1367(line.)X
1546(All)X
1683(the)X
1815(targets)X
2074(in)X
2167(the)X
2298(line)X
2454(are)X
2584(made)X
2798(to)X
2890(depend)X
3167(on)X
3278(all)X
3390(the)X
3521(sources.)X
3851(If)X
3932(you)X
4087(run)X
748 5376(out)N
883(of)X
978(room,)X
1208(use)X
1347(a)X
1408(backslash)X
1772(at)X
1858(the)X
1988(end)X
2137(of)X
2232(the)X
2362(line)X
2517(to)X
2608(continue)X
2934(onto)X
3113(the)X
3243(next)X
3417(one.)X
748 5528(Any)N
923(\256le)X
1060(may)X
1236(be)X
1343(a)X
1406(target)X
1631(and)X
1782(any)X
1933(\256le)X
2070(may)X
2246(be)X
2353(a)X
2416(source,)X
2691(but)X
2828(the)X
2960(relationship)X
3401(between)X
3717(them)X
3917(is)X
3999(deter-)X
748 5632(mined)N
991(by)X
1101(the)X
1231(``operator'')X
1662(that)X
1817(separates)X
2161(them.)X
2382(Three)X
2609(operators)X
2958(are)X
3087(de\256ned:)X
1036 5784(:)N
1324(A)X
1420(target)X
1654(on)X
1775(the)X
1916(line)X
2082(is)X
2174(considered)X
2588(``out-of-date'')X
3128(if)X
3214(any)X
3373(of)X
3478(its)X
3594(sources)X
3889(has)X
4038(been)X
10 s
460 6176(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(2)X

3 p
%%Page: 3 3
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
1324 784(modi\256ed)N
1661(more)X
1865(recently)X
2172(than)X
2347(the)X
2478(target.)X
2724(Sources)X
3025(for)X
3150(a)X
3212(target)X
3435(accumulate)X
3859(over)X
4037(lines)X
1324 888(when)N
1536(this)X
1686(operator)X
2001(is)X
2082(used.)X
1036 1040(!)N
1324(Targets)X
1614(will)X
1778(always)X
2048(be)X
2157(re-created,)X
2556(but)X
2695(this)X
2849(will)X
3013(not)X
3152(happen)X
3432(until)X
3620(all)X
3734(of)X
3832(its)X
3941(sources)X
1324 1144(have)N
1517(been)X
1710(examined)X
2080(and)X
2233(re-created,)X
2632(if)X
2712(necessary.)X
3123(Sources)X
3427(accumulate)X
3855(over)X
4037(lines)X
1324 1248(as)N
1419(for)X
1543(the)X
1673(colon.)X
1036 1400(::)N
1324(Much)X
1562(like)X
1728(the)X
1869(colon,)X
2119(but)X
2264(acts)X
2433(like)X
2598(the)X
2738(!)X
2799(operator)X
3124(if)X
3210(no)X
3330(sources)X
3625(are)X
3764(speci\256ed.)X
4131(In)X
1324 1504(addition)N
1641(sources)X
1931(do)X
2046(not)X
2186(accumulate)X
2615(over)X
2798(lines.)X
3014(Rather,)X
3298(the)X
3433(commands)X
3842(associated)X
1324 1608(with)N
1524(the)X
1675(line)X
1851(\(see)X
2035(below\))X
2321(are)X
2470(executed)X
2825(only)X
3024(if)X
3120(the)X
3270(target)X
3513(is)X
3614(out-of-date)X
4047(with)X
1324 1712(respect)N
1617(to)X
1730(the)X
1882(sources)X
2189(on)X
2321(that)X
2498(line)X
2674(only.)X
2918(In)X
3034(addition,)X
3389(the)X
3540(target)X
3784(will)X
3965(not)X
4121(be)X
1324 1816(removed)N
1654(if)X
2 f
1730(PMake)X
1 f
2001(is)X
2082(interrupted,)X
2513(unlike)X
2756(for)X
2880(the)X
3010(other)X
3213(two)X
3367(operators.)X
748 1968(For)N
892(example:)X
1036 2124(a)N
1324(:)X
1371(a.o)X
1498(b.o)X
1630(c.o)X
1036 2228(b)N
1324(!)X
1375(d.o)X
1507(e.o)X
1036 2332(c)N
1324(::)X
1396(f.o)X
1324 2436(command1)N
1036 2540(a)N
1324(:)X
1371(g.o)X
1036 2644(b)N
1324(!)X
1375(h.o)X
1036 2748(c)N
1324(::)X
1324 2852(command2)N
748 3008(speci\256es)N
1089(that)X
1260(a)X
1337(depends)X
1663(on)X
1789(a.o,)X
1954(b.o,)X
2124(c.o)X
2267(and)X
2432(g.o)X
2579(and)X
2743(will)X
2918(be)X
3038(remade)X
3334(only)X
3528(if)X
3619(out-of-date)X
4047(with)X
748 3112(respect)N
1021(to)X
1114(these)X
1319(four)X
1489(\256les.)X
1682(b)X
1750(depends)X
2062(on)X
2173(d.o,)X
2328(e.o)X
2456(and)X
2606(h.o)X
2739(and)X
2889(will)X
3050(always)X
3317(be)X
3423(remade,)X
3727(but)X
3863(only)X
4043(after)X
748 3216(these)N
964(three)X
1175(\256les)X
1357(have)X
1558(been)X
1759(remade.)X
2075(c)X
2149(will)X
2322(be)X
2440(remade)X
2734(with)X
2926(command1)X
3353(if)X
3442(it)X
3527(is)X
3621(out-of-date)X
4047(with)X
748 3320(respect)N
1019(to)X
1110(f.o,)X
1249(as)X
1344(for)X
1468(the)X
1598(colon)X
1816(operator,)X
2153(while)X
2371(command2)X
2785(will)X
2945(always)X
3211(be)X
3316(executed.)X
748 3472(Targets)N
1035(and)X
1185(sources)X
1471(may)X
1646(also)X
1811(contain)X
2094(standard)X
2415(shell)X
2605(wildcard)X
2936(characters)X
3315(\(?,)X
9 f
3428(*)X
1 f
(,)S
3517([)X
3569(and)X
3719({}\),)X
3877(but)X
4013(the)X
4143(?,)X
9 f
748 3576(*)N
1 f
(,)S
838([)X
891(and)X
1042(])X
1095(characters)X
1475(may)X
1651(only)X
1832(be)X
1939(used)X
2124(in)X
2217(the)X
2348(\256nal)X
2528(component)X
2943(of)X
3039(the)X
3170(target)X
3394(or)X
3490(source.)X
3764(If)X
3845(a)X
3907(target)X
4131(or)X
748 3680(source)N
1003(contains)X
1323(only)X
1506(curly)X
1713(braces)X
1963(and)X
2116(no)X
2230(other)X
2437(wildcard)X
2771(characters,)X
3175(it)X
3251(need)X
3443(not)X
3582(describe)X
3900(an)X
4008(exist-)X
748 3784(ing)N
883(\256le.)X
1040(Otherwise,)X
1445(only)X
1624(existing)X
1926(\256les)X
2095(will)X
2255(be)X
2360(used.)X
2565(E.g.)X
2729(the)X
2859(pattern)X
1036 3940({a,b,c}.o)N
748 4096(will)N
908(expand)X
1184(to)X
1036 4252(a.o)N
1163(b.o)X
1295(c.o)X
748 4408(regardless)N
1126(of)X
1221(whether)X
1526(these)X
1729(three)X
1927(\256les)X
2096(exist,)X
2307(while)X
1036 4564([abc].o)N
748 4720(will)N
915(only)X
1101(expand)X
1384(to)X
1482(this)X
1639(if)X
1722(all)X
1840(three)X
2045(\256les)X
2221(exist.)X
2439(The)X
2604(resulting)X
2941(expansion)X
3326(is)X
3413(in)X
3510(directory)X
3856(order,)X
4091(not)X
748 4824(alphabetically)N
1266(sorted)X
1503(as)X
1598(in)X
1689(the)X
1819(shell.)X
3 f
10 s
460 4976(COMMANDS)N
1 f
11 s
748 5080(Each)N
955(target)X
1187(has)X
1335(associated)X
1728(with)X
1916(it)X
1997(a)X
2067(sort)X
2230(of)X
2334(shell)X
2532(script)X
2759(made)X
2980(up)X
3098(of)X
3201(a)X
3270(series)X
3500(of)X
3603(shell)X
3800(commands.)X
748 5184(The)N
919(creation)X
1237(script)X
1467(for)X
1603(a)X
1676(target)X
1911(should)X
2180 0.3187(immediately)AX
2657(follow)X
2920(the)X
3061(dependency)X
3514(line)X
3680(for)X
3815(that)X
3981(target.)X
748 5288(Each)N
946(of)X
1041(the)X
1171(commands)X
1575(in)X
1666(this)X
1816(script)X
2 f
2034(must)X
1 f
2222(be)X
2327(preceeded)X
2705(by)X
2815(a)X
2876(tab)X
3006(character.)X
748 5440(While)N
991(any)X
1145(given)X
1368(target)X
1596(may)X
1775(appear)X
2036(on)X
2151(more)X
2359(than)X
2538(one)X
2692(dependency)X
3139(line,)X
3321(only)X
3505(one)X
3659(of)X
3758(these)X
3965(depen-)X
748 5544(dency)N
980(lines)X
1169(may)X
1343(be)X
1448(followed)X
1783(by)X
1893(a)X
1954(creation)X
2260(script,)X
2500(unless)X
2742(the)X
2872("::")X
3016(operator)X
3331(is)X
3412(used.)X
10 s
460 6152(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(3)X

4 p
%%Page: 4 4
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
748 784(One)N
917(helpful)X
1190(feature)X
1457(of)X
2 f
1553(PMake)X
1 f
1825(is)X
1907(the)X
2038(ability)X
2288(to)X
2380(squirrel)X
2672(away)X
2880(commands)X
3285(for)X
3410(a)X
3472(target)X
3695(to)X
3786(be)X
3891(executed)X
748 888(when)N
963(everything)X
1365(else)X
1527(has)X
1669(been)X
1860(done.)X
2077(To)X
2199(do)X
2311(this,)X
2485(make)X
2700(one)X
2851(of)X
2948(the)X
3080(commands)X
3486(for)X
3612(the)X
3744(target)X
3969(be)X
4076(just)X
748 992(``...'')N
953(\(an)X
1088(ellipsis\))X
1391(on)X
1502(a)X
1564(line)X
1720(by)X
1830(itself.)X
2051(The)X
2210(ellipsis)X
2483(itself)X
2682(won't)X
2909(be)X
3014(executed,)X
3371(of)X
3466(course,)X
3739(but)X
3874(any)X
4023(com-)X
748 1096(mands)N
1005(in)X
1101(the)X
1236(target's)X
1527(script)X
1750(that)X
1910(follow)X
2167(the)X
2302(ellipsis)X
2580(will)X
2745(be)X
2855(saved)X
3082(until)X
2 f
3271(PMake)X
1 f
3546(is)X
3631(done)X
3828(processing)X
748 1200(everything)N
1147(it)X
1219(needs)X
1441(to)X
1532(process.)X
1861(If)X
1941(you)X
2095(were)X
2287(to)X
2378(say,)X
1036 1356(a.o)N
1427(:)X
1474(a.c)X
1212 1460(cc)N
1312(-c)X
1402(a.c)X
1212 1564(...)N
1212 1668(@echo)N
1481("All)X
1652(done")X
748 1824(Then)N
961(the)X
1101(command)X
1481(``echo)X
1737("All)X
1918(done"'')X
2215(would)X
2467(execute)X
2768(once)X
2966(everything)X
3375(else)X
3543(had)X
3701(\256nished.)X
4033(Note)X
748 1928(that)N
903(this)X
1053(will)X
1213(only)X
1392(happen)X
1668(if)X
1744(``a.o'')X
1987(is)X
2068(found)X
2295(to)X
2386(be)X
2491(out-of-date.)X
748 2080(There)N
981(is)X
1068(another)X
1360(way)X
1534(in)X
1631(which)X
1874(make\256le)X
2206(shell)X
2401(commands)X
2810(differ)X
3032(from)X
3230(regular)X
3506(shell)X
3700(commands,)X
4131(as)X
748 2184(illustrated)N
1126(in)X
1219(the)X
1351(above)X
1584(make\256le)X
1911(scrap.)X
2163(The)X
2323(\256rst)X
2483(two)X
2638(characters)X
3017(after)X
3201(the)X
3332(initial)X
3563(tab)X
3694(\(and)X
3873(any)X
4023(other)X
748 2288(whitespace\))N
1203(are)X
1345(treated)X
1620(specially.)X
1991(If)X
2084(they)X
2271(are)X
2413(any)X
2575(combination)X
3052(of)X
3159(`@')X
3332(and)X
3493(`)X
9 f
3522(-)X
1 f
3570(',)X
3655(\(``@'',)X
3937(``@)X
9 f
4076(-)X
1 f
4124('',)X
748 2392(``)N
9 f
806(-)X
1 f
854(@'')X
1015(or)X
1110(``)X
9 f
1168(-)X
1 f
1216(''\),)X
1347(they)X
1521(cause)X
2 f
1738(PMake)X
1 f
2009(to)X
2100(do)X
2210(different)X
2535(things.)X
748 2544(In)N
848(most)X
1047(cases,)X
1281(shell)X
1474(commands)X
1882(are)X
2015(printed)X
2291(to)X
2386(the)X
2520(screen)X
2770(before)X
3020(they're)X
3295(actually)X
3601(executed.)X
3962(This)X
4145(is)X
748 2648(to)N
841(keep)X
1031(you)X
1187(informed)X
1534(of)X
1631(what's)X
1889(going)X
2114(on.)X
2248(If)X
2330(an)X
2437(`@')X
2600(appears,)X
2914(however,)X
3262(this)X
3413(echoing)X
3715(is)X
3797(suppressed.)X
748 2752(In)N
843(the)X
973(case)X
1146(of)X
1241(the)X
1371(echo)X
1559(command,)X
1951(above,)X
2205(this)X
2355(makes)X
2602(sense.)X
2836(It)X
2912(would)X
3154(look)X
3333(silly)X
3508(to)X
3599(see)X
1036 2908(echo)N
1224("All)X
1395(done")X
1036 3012(All)N
1171(done)X
748 3168(so)N
2 f
862(PMake)X
1 f
1147(allows)X
1413(you)X
1581(to)X
1686(avoid)X
1918(that)X
2087(\(this)X
2280(sort)X
2448(of)X
2557(echo)X
2759(control)X
3045(is)X
3140(only)X
3333(available)X
3688(if)X
3777(you)X
3944(use)X
4096(the)X
748 3272(Bourne)N
1035(or)X
1136(C)X
1223(shells)X
1452(to)X
1548(execute)X
1844(your)X
2032(commands,)X
2463(since)X
2671(the)X
2806(commands)X
3215(are)X
3349(echoed)X
3625(by)X
3740(the)X
3875(shell,)X
4091(not)X
748 3376(by)N
2 f
858(PMake)X
1 f
1114(\).)X
748 3528(The)N
916(other)X
1128(special)X
1404(character)X
1757(is)X
1847(the)X
1986(`)X
9 f
2015(-)X
1 f
2063('.)X
2167(Shell)X
2380(commands)X
2793(exit)X
2957(with)X
3145(a)X
3215(certain)X
3485(``exit)X
3706(status.'')X
4039(Nor-)X
748 3632(mally)N
976(this)X
1130(status)X
1357(will)X
1521(be)X
1630(0)X
1700(if)X
1780(everything)X
2183(went)X
2380(ok)X
2494(and)X
2647(non-zero)X
2985(if)X
3065(something)X
3459(went)X
3656(wrong.)X
3928(For)X
4076(this)X
748 3736(reason,)N
2 f
1021(PMake)X
1 f
1292(will)X
1452(consider)X
1772(an)X
1877(error)X
2069(to)X
2160(have)X
2348(occurred)X
2677(if)X
2753(one)X
2902(of)X
2997(the)X
3127(commands)X
3531(it)X
3603(invokes)X
3899(returns)X
4165(a)X
748 3840(non-zero)N
1088(status.)X
1339(When)X
1576(it)X
1653(detects)X
1925(an)X
2035(error,)X
2254(its)X
2365(usual)X
2578(action)X
2821(is)X
2907(to)X
3003(stop)X
3177(working,)X
3519(wait)X
3698(for)X
3827(everything)X
748 3944(in)N
842(process)X
1130(to)X
1224(\256nish,)X
1467(and)X
1619(exit)X
1777(with)X
1959(a)X
2022(non-zero)X
2358(status)X
2583(itself.)X
2828(This)X
3009(behavior)X
3341(can)X
3487(be)X
3594(altered,)X
3880(however,)X
748 4048(by)N
860(means)X
1109(of)X
3 f
9 f
1206(-)X
3 f
1254(i)X
1 f
1303(or)X
3 f
9 f
1400(-)X
3 f
1448(k)X
1 f
1521(arguments,)X
1933(or)X
2029(by)X
2140(placing)X
2423(a)X
2485(`)X
9 f
2514(-)X
1 f
2562(')X
2614(at)X
2701(the)X
2832(front)X
3026(of)X
3122(the)X
3253(command.)X
3668(\(Another)X
4008(quick)X
748 4152(note:)N
949(the)X
1081(decision)X
1398(of)X
1494(whether)X
1800(to)X
1892(abort)X
2096(a)X
2158(target)X
2382(when)X
2595(one)X
2745(of)X
2841(its)X
2948(shell)X
3138(commands)X
3543(returns)X
3810(non-zero)X
4145(is)X
748 4256(left)N
896(to)X
994(the)X
1131(shell)X
1327(that)X
1489(is)X
1577(executing)X
1949(the)X
2086(commands.)X
2519(Some)X
2749(shells)X
2979(allow)X
3204(this)X
3361(``error-checking'')X
4023(to)X
4121(be)X
748 4360(switched)N
1083(on)X
1193(and)X
1342(off)X
1466(at)X
1552(will)X
1712(while)X
1930(others)X
2167(do)X
2277(not.\))X
3 f
10 s
460 4512(VARIABLES)N
2 f
11 s
748 4616(PMake)N
1 f
1021(has)X
1162(the)X
1294(ability)X
1545(to)X
1638(save)X
1818(text)X
1975(in)X
2068(variables)X
2409(to)X
2501(be)X
2607(recalled)X
2909(later)X
3089(at)X
3176(your)X
3360(convenience.)X
3867(Variables)X
748 4720(in)N
2 f
841(PMake)X
1 f
1114(are)X
1245(used)X
1430(much)X
1650(like)X
1807(variables)X
2149(in)X
2 f
2242(sh)X
1 f
2327(\(1\))X
2453(and,)X
2626(by)X
2738(tradition,)X
3084(consist)X
3353(of)X
3450(all)X
3563(upper-case)X
3966(letters.)X
748 4824(They)N
951(are)X
1080(assigned-)X
1434(and)X
1583(appended-to)X
2040(using)X
2253(lines)X
2442(of)X
2537(the)X
2667(form)X
2 f
1036 4980(VARIABLE)N
3 f
1460(=)X
2 f
1532(value)X
1036 5084(VARIABLE)N
3 f
1460(+=)X
2 f
1582(value)X
1 f
748 5240(respectively,)N
1226(while)X
1451(being)X
1676(conditionally)X
2172(assigned-to)X
2602(\(if)X
2714(not)X
2856(already)X
3144(de\256ned\))X
3461(and)X
3617(assigned-to)X
4047(with)X
748 5344(expansion)N
1127(by)X
1237(lines)X
1426(of)X
1521(the)X
1651(form)X
2 f
1036 5500(VARIABLE)N
3 f
1460(?=)X
2 f
1576(value)X
1036 5604(VARIABLE)N
3 f
1460(:=)X
2 f
1561(value)X
1 f
748 5760(Finally,)N
10 s
460 6152(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(4)X

5 p
%%Page: 5 5
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
2 f
1036 784(VARIABLE)N
3 f
1460(!=)X
2 f
1561(command)X
1 f
748 940(will)N
909(execute)X
2 f
1201(command)X
1 f
1565(using)X
1778(the)X
1908(Bourne)X
2189(shell)X
2378(and)X
2527(place)X
2735(the)X
2865(result)X
3083(in)X
3174(the)X
3304(given)X
3522(variable.)X
3872(Newlines)X
748 1044(are)N
880(converted)X
1252(to)X
1346(spaces)X
1600(before)X
1849(the)X
1982(assignment)X
2404(is)X
2488(made.)X
2726(This)X
2908(is)X
2992(not)X
3129(intended)X
3457(to)X
3550(be)X
3657(used)X
3842(with)X
4023(com-)X
748 1148(mands)N
1000(that)X
1155(produce)X
1460(a)X
1521(large)X
1719(amount)X
2006(of)X
2101(output.)X
2371(If)X
2451(you)X
2605(use)X
2744(it)X
2816(this)X
2966(way,)X
3156(it)X
3228(will)X
3388(probably)X
3723(deadlock.)X
748 1300(Variables)N
1113(are)X
1248(expanded)X
1613(by)X
1728(enclosing)X
2093(the)X
2228(variable)X
2539(name)X
2757(in)X
2853(either)X
3081(parentheses)X
3518(or)X
3618(curly)X
3826(braces)X
4077(and)X
748 1404(preceeding)N
1168(the)X
1310(whole)X
1559(thing)X
1775(with)X
1966(a)X
2039(dollar)X
2279(sign.)X
2504(E.g.)X
2679(to)X
2781(set)X
2912(the)X
3053(variable)X
3 f
3370(CFLAGS)X
1 f
3760(to)X
3862(the)X
4003(string)X
748 1508(``)N
9 f
806(-)X
1 f
854 0.2000(I/sprite/src/lib/libc)AX
9 f
1530(-)X
1 f
1578(O'')X
1721(you)X
1875(would)X
2117(place)X
2325(a)X
2386(line)X
1036 1664(CFLAGS)N
1395(=)X
9 f
1467(-)X
1 f
1515 0.2000(I/sprite/src/lib/libc)AX
9 f
2191(-)X
1 f
2239(O)X
748 1820(in)N
884(the)X
1059(make\256le)X
1430(and)X
1624(use)X
1808(the)X
1983(word)X
3 f
2229($\(CFLAGS\))X
1 f
2754(wherever)X
3146(you)X
3344(would)X
3630(like)X
3829(the)X
4003(string)X
748 1924(``)N
9 f
806(-)X
1 f
854 0.2000(I/sprite/src/lib/libc)AX
9 f
1541(-)X
1 f
1589(O'')X
1743(to)X
1845(appear.)X
2156(To)X
2287(pass)X
2471(a)X
2543(string)X
2777(of)X
2882(the)X
3022(form)X
3225(``$\()X
2 f
3356(name)X
1 f
3546(\)'')X
3665(or)X
3770(``${)X
2 f
3914(name)X
1 f
4104(}'')X
748 2028(through)N
1047(to)X
1141(the)X
1274(shell)X
1466(\(e.g.)X
1647(to)X
1741(tell)X
1879(it)X
1953(to)X
2046(substitute)X
2409(one)X
2560(of)X
2657(its)X
2765(variables\),)X
3158(you)X
3314(can)X
3460(use)X
3601(``$$\()X
2 f
3776(name)X
1 f
3966(\)'')X
4077(and)X
748 2132(``$${)N
2 f
936(name)X
1 f
1126(}'',)X
1278(respectively,)X
1756(or,)X
1881(as)X
1984(long)X
2171(as)X
2274(the)X
2 f
2412(name)X
1 f
2632(is)X
2721(not)X
2864(a)X
2 f
2933(PMake)X
1 f
3212(variable,)X
3548(you)X
3710(can)X
3861(just)X
4018(place)X
748 2236(the)N
882(string)X
1109(in)X
1204(directly,)X
1522(as)X
2 f
1621(PMake)X
1 f
1896(will)X
2060(not)X
2199(expand)X
2478(a)X
2542(variable)X
2851(it)X
2926(doesn't)X
3210(know,)X
3452(unless)X
3697(it)X
3772(is)X
3856(given)X
4077(one)X
748 2340(of)N
843(the)X
973(three)X
1171 0.3177(compatibility)AX
1666(\257ags)X
3 f
9 f
1854(-)X
3 f
1902(V)X
1 f
1965(,)X
3 f
9 f
2009(-)X
3 f
2057(B)X
1 f
2116(,)X
2160(or)X
3 f
9 f
2255(-)X
3 f
2303(M)X
1 f
2386(.)X
748 2492(There)N
984(are)X
1122(two)X
1285(distinct)X
1577(times)X
1800(at)X
1895(which)X
2141(variable)X
2456(substitution)X
2900(occurs:)X
3185(When)X
3426(parsing)X
3715(a)X
3784(dependency)X
748 2596(line,)N
943(such)X
1144(substitution)X
1597(occurs)X
1866 0.3187(immediately)AX
2349(upon)X
2565(reading)X
2869(the)X
3017(line.)X
3234(Thus)X
3450(all)X
3578(variables)X
3935(used)X
4135(in)X
748 2700(dependency)N
1194(lines)X
1387(must)X
1585(be)X
1694(de\256ned)X
1979(before)X
2229(they)X
2407(appear)X
2667(on)X
2781(any)X
2934(dependency)X
3379(line.)X
3581(For)X
3728(variables)X
4071(that)X
748 2804(appear)N
1022(in)X
1131(shell)X
1338(commands,)X
1782(variable)X
2106(substitution)X
2559(occurs)X
2828(when)X
3057(the)X
3204(command)X
3591(is)X
3689(processed,)X
4096(i.e.)X
748 2908(when)N
967(it)X
1046(is)X
1134(prepared)X
1470(to)X
1568(be)X
1680(passed)X
1943(to)X
2041(the)X
2178(shell)X
2374(or)X
2476(before)X
2729(being)X
2953(squirreled)X
3333(away)X
3546(for)X
3676(later)X
3861(execution)X
748 3012(\(qv.)N
3 f
909(COMMANDS)X
1 f
1445(,)X
1489(above\).)X
748 3164(There)N
981(are)X
1116(four)X
1290(different)X
1621(types)X
1835(of)X
1936(variables)X
2282(at)X
2374(which)X
2 f
2617(PMake)X
1 f
2894(will)X
3060(look)X
3245(when)X
3462(trying)X
3700(to)X
3796(expand)X
4077(any)X
748 3268(given)N
986(variable.)X
1356(They)X
1579(are)X
1728(\(in)X
1868(order)X
2095(of)X
2210(decreasing)X
2628(precedence\):)X
3119(\(1\))X
3263(variables)X
3623(that)X
3797(are)X
3945(de\256ned)X
748 3372(speci\256c)N
1043(to)X
1138(a)X
1203(certain)X
1469(target.)X
1718(These)X
1954(are)X
2087(the)X
2221(so-called)X
2565(``local'')X
2879(variables)X
3223(and)X
3375(will)X
3538(only)X
3720(be)X
3828(used)X
4014(when)X
748 3476(performing)N
1167(variable)X
1474(substitution)X
1910(on)X
2021(the)X
2152(target's)X
2439(shell)X
2629(script)X
2848(and)X
2998(in)X
3090(dynamic)X
3417(sources)X
3702(\(see)X
3865(below)X
4102(for)X
748 3580(more)N
960(details\),)X
1273(\(2\))X
1406(variables)X
1755(that)X
1919(were)X
2120(de\256ned)X
2409(on)X
2527(the)X
2665(command)X
3043(line,)X
3228(\(3\))X
3360(variables)X
3708(de\256ned)X
3997(in)X
4096(the)X
748 3684(make\256le)N
1093(and)X
1261(\(4\))X
1404(those)X
1631(de\256ned)X
1931(in)X
2 f
2041(PMake)X
1 f
2297('s)X
2400(environment,)X
2908(as)X
3021(passed)X
3295(by)X
3423(your)X
3624(login)X
3846(shell.)X
4097(An)X
748 3788(important)N
1122(side)X
1294(effect)X
1524(of)X
1627(this)X
1785(searching)X
2152(order)X
2367(is)X
2456(that)X
2619(once)X
2815(you)X
2977(de\256ne)X
3221(a)X
3289(variable)X
3602(on)X
3719(the)X
3856(command)X
748 3892(line,)N
925(nothing)X
1217(in)X
1308(the)X
1438(make\256le)X
1764(can)X
1908(change)X
2179(it.)X
2 f
2273(Nothing.)X
1 f
748 4044(As)N
870(mentioned)X
1268(above,)X
1525(each)X
1711(target)X
1937(has)X
2079(associated)X
2466(with)X
2648(it)X
2723(as)X
2821(many)X
3041(as)X
3138(seven)X
3362(``local'')X
3674(variables.)X
4038(Four)X
748 4148(of)N
844(these)X
1048(variables)X
1389(are)X
1519(always)X
1786(set)X
1907(for)X
2032(every)X
2250(target)X
2474(that)X
2630(must)X
2825(be)X
2931(re-created.)X
3327(Each)X
3526(local)X
3720(variable)X
4026(has)X
4165(a)X
748 4252(long,)N
955(meaningful)X
1385(name)X
1604(and)X
1759(a)X
1826(short,)X
2051(one-character)X
2556(name)X
2774(that)X
2934(exists)X
3162(for)X
3291(backwards-compatability.)X
748 4356(They)N
951(are:)X
1036 4508(.TARGET)N
1427(\(@\))X
1324 4612(The)N
1483(name)X
1696(of)X
1791(the)X
1921(target.)X
1036 4764(.OODATE)N
1440(\(?\))X
1324 4868(The)N
1483(list)X
1614(of)X
1709(sources)X
1994(for)X
2118(this)X
2268(target)X
2491(that)X
2646(were)X
2838(deemed)X
3134(out-of-date.)X
1036 5020(.ALLSRC)N
1418(\(>\))X
1324 5124(The)N
1483(list)X
1614(of)X
1709(all)X
1820(sources)X
2105(for)X
2229(this)X
2379(target.)X
1036 5276(.PREFIX)N
1383(\()X
9 f
1412(*)X
1 f
(\))S
1324 5380(The)N
1488(\256le)X
1628(pre\256x)X
1860(of)X
1960(the)X
2095(\256le.)X
2257(This)X
2441(contains)X
2762(only)X
2946(the)X
3081(\256le)X
3221(portion)X
3503(--)X
3588(no)X
3703(suf\256x)X
3929(or)X
4028(lead-)X
1324 5484(ing)N
1459(directory)X
1799(components.)X
748 5636(Three)N
980(other)X
1188(``local'')X
1503(variables)X
1848(are)X
1982(set)X
2107(only)X
2291(for)X
2420(certain)X
2687(targets)X
2949(under)X
3176(special)X
3447(circumstances.)X
3994(These)X
748 5740(are)N
885(the)X
1023(``.IMPSRC'',)X
1536(``.ARCHIVE'')X
2093(and)X
2249(``.MEMBER'')X
2798(variables.)X
3167(When)X
3406(they)X
3587(are)X
3723(set,)X
3872(how)X
4052(they)X
748 5844(are)N
877(used,)X
1082(and)X
1231(what)X
1424(their)X
1608(short)X
1806(forms)X
2033(are)X
2162(are)X
2291(detailed)X
2593(in)X
2684(later)X
2863(sections.)X
10 s
460 6236(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(5)X

6 p
%%Page: 6 6
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
748 784(In)N
850(addition,)X
1190(for)X
1320(you)X
1480(System)X
1768(V)X
1859(fans,)X
2055(the)X
2191(six)X
2322(variables)X
2668(``@F'',)X
2964(``@D'',)X
3274(``<F'',)X
3539(``<D'',)X
3818(``)X
9 f
3876(*)X
1 f
(F'',)S
4077(and)X
748 888(``)N
9 f
806(*)X
1 f
(D'')S
998(are)X
1132(de\256ned)X
1418(to)X
1514(be)X
1624(the)X
1758(same)X
1965(as)X
2064(for)X
2192(the)X
2326(System)X
2612(V)X
2701(version)X
2986(of)X
3085(Make.)X
3333(If)X
3417(you)X
3575(don't)X
3787(know)X
4008(about)X
748 992(these)N
951(things,)X
1211(be)X
1316(glad.)X
748 1144(Four)N
942(of)X
1043(these)X
1252(local)X
1452(variables)X
1798(may)X
1978(be)X
2089(used)X
2277(in)X
2373(sources)X
2663(on)X
2778(dependency)X
3225(lines.)X
3441(The)X
3605(variables)X
3950(expand)X
748 1248(to)N
854(the)X
999(proper)X
1265(value)X
1493(for)X
1632(each)X
1830(target)X
2068(on)X
2193(the)X
2337(line.)X
2528(The)X
2701(variables)X
3055(are)X
3198(``.TARGET'',)X
3741(``.PREFIX'',)X
748 1352(``.ARCHIVE'',)N
1320(and)X
1469(``.MEMBER''.)X
748 1504(In)N
849(addition,)X
1189(certain)X
1457(variables)X
1803(are)X
1938(set)X
2064(by)X
2180(or)X
2281(have)X
2475(special)X
2748(meaning)X
3080(to)X
2 f
3177(PMake)X
1 f
3433(.)X
3505(The)X
3 f
3670(.PMAKE)X
1 f
4048(\(and)X
3 f
748 1608(MAKE)N
1 f
1022(\))X
1081(variable)X
1395(is)X
1484(set)X
1612(to)X
1711(the)X
1848(name)X
2068(by)X
2185(which)X
2 f
2429(PMake)X
1 f
2707(was)X
2872(invoked,)X
3207(to)X
3305(allow)X
3530(recursive)X
3881(makes)X
4135(to)X
748 1712(use)N
892(the)X
1027(same)X
1235(version,)X
1543(whatever)X
1892(it)X
1969(may)X
2148(be.)X
2302(All)X
2441(command-line)X
2977(\257ags)X
3169(given)X
3391(to)X
2 f
3486(PMake)X
1 f
3761(are)X
3894(stored)X
4135(in)X
748 1816(the)N
3 f
897(.MAKEFLAGS)X
1 f
1528(\(and)X
3 f
1725(MFLAGS)X
1 f
2102(\))X
2172(variable)X
2497(just)X
2666(as)X
2780(they)X
2973(were)X
3184(given.)X
3442(This)X
3639(variable)X
3963(is)X
4062(also)X
748 1920(exported)N
1078(to)X
1169(subshells)X
1514(as)X
1609(the)X
3 f
1739(PMAKE)X
1 f
2089(environment)X
2557(variable.)X
748 2072(Variable)N
1080(expansion)X
1466(may)X
1647(be)X
1759(modi\256ed)X
2102(as)X
2204(for)X
2335(the)X
2472(C)X
2559(shell.)X
2776(A)X
2867(general)X
3154(expansion)X
3539(speci\256cation)X
4013(looks)X
748 2176(like:)N
3 f
1036 2332($\()N
2 f
1109(variable)X
1 f
1403([)X
3 f
1432(:)X
2 f
1461(modi\256er)X
1 f
1754([)X
3 f
1783(:)X
1 f
1812(...]])X
3 f
1936(\))X
1 f
748 2488(Each)N
946(modi\256er)X
1267(begins)X
1519(with)X
1698(a)X
1759(single)X
1992(character,)X
2358(thus:)X
1036 2640(M)N
2 f
1114(pattern)X
1 f
1324 2744(This)N
1510(is)X
1598(used)X
1788(to)X
1886(select)X
2116(only)X
2302(those)X
2517(words)X
2760(\(a)X
2857(word)X
3066(is)X
3154(a)X
3222(series)X
3451(of)X
3552(characters)X
3936(that)X
4097(are)X
1324 2848(neither)N
1598(spaces)X
1856(nor)X
2002(tabs\))X
2202(that)X
2364(match)X
2609(the)X
2746(given)X
2 f
2971(pattern)X
3254(.)X
1 f
3304(The)X
3469(pattern)X
3742(is)X
3829(a)X
3896(wildcard)X
1324 2952(pattern)N
1597(like)X
1758(that)X
1919(used)X
2108(by)X
2224(the)X
2360(shell,)X
2577(where)X
2819(")X
9 f
2855(*)X
1 f
(")S
2963(means)X
3215(0)X
3286(or)X
3386(more)X
3594(characters)X
3977(of)X
4077(any)X
1324 3056(sort;)N
1517("?")X
1664(is)X
1759(any)X
1922(single)X
2169(character;)X
2552("[abcd]")X
2884(matches)X
3209(any)X
3372(single)X
3619(character)X
3977(that)X
4145(is)X
1324 3160(either)N
1561(`a',)X
1716(`b',)X
1876(`c')X
2009(or)X
2118(`d')X
2256(\(there)X
2497(may)X
2685(be)X
2803(any)X
2965(number)X
3269(of)X
3377(characters)X
3768(between)X
4096(the)X
1324 3264(brackets\);)N
3 f
1697([0-9])X
1 f
1898(matches)X
2213(any)X
2366(single)X
2602(character)X
2949(that)X
3107(is)X
3191(between)X
3509(`0')X
3636(and)X
3788(`9')X
3915(\(i.e.)X
4077(any)X
1324 3368(digit.)N
1537(This)X
1722(form)X
1920(may)X
2099(be)X
2209(freely)X
2441(mixed)X
2689(with)X
2873(the)X
3008(other)X
3216(bracket)X
3502(form\),)X
3751(and)X
3905(\\)X
3957(is)X
4043(used)X
1324 3472(to)N
1416(escape)X
1673(any)X
1822(of)X
1917(the)X
2047(characters)X
2425(")X
9 f
2461(*)X
1 f
(",)S
2585("?",)X
2740("[")X
2863(or)X
2958(":",)X
3099(leaving)X
3381(them)X
3580(as)X
3675(regular)X
3946(charac-)X
1324 3576(ters)N
1473(to)X
1564(match)X
1802(themselves)X
2216(in)X
2307(a)X
2368(word.)X
1036 3728(N)N
2 f
1099(pattern)X
1 f
1324 3832(This)N
1504(is)X
1586(identical)X
1914(to)X
2006(":M")X
2204(except)X
2457(it)X
2530(substitutes)X
2926(all)X
3038(words)X
3275(that)X
3431(don't)X
3640(match)X
3878(the)X
4008(given)X
1324 3936(pattern.)N
1036 4088(S/)N
2 f
1110(search-string)X
1 f
1579(/)X
2 f
1604(replacement-string)X
1 f
2274(/[g])X
1324 4192(Causes)N
1612(the)X
1759(\256rst)X
1935(occurrence)X
2359(of)X
2 f
2470(search-string)X
1 f
2977(in)X
3084(the)X
3230(variable)X
3552(to)X
3659(be)X
3780(replaced)X
4116(by)X
2 f
1324 4296(replacement-string)N
2031(,)X
1 f
2090(unless)X
2347(the)X
2492("g")X
2645(\257ag)X
2814(is)X
2910(given)X
3143(at)X
3243(the)X
3387(end,)X
3572(in)X
3677(which)X
3928(case)X
4115(all)X
1324 4400(occurences)N
1738(of)X
1834(the)X
1965(string)X
2189(are)X
2319(replaced.)X
2662(The)X
2822(substitution)X
3258(is)X
3340(performed)X
3729(on)X
3840(each)X
4024(word)X
1324 4504(in)N
1423(the)X
1561(variable)X
1875(in)X
1974(turn.)X
2168(If)X
2 f
2256(search-string)X
1 f
2755(begins)X
3014(with)X
3200(a)X
3268("\303",)X
3420(the)X
3557(string)X
3787(must)X
3988(match)X
1324 4608(starting)N
1616(at)X
1707(the)X
1842(beginning)X
2222(of)X
2322(the)X
2457(word.)X
2686(If)X
2 f
2771(search-string)X
1 f
3267(ends)X
3455(with)X
3639(a)X
3705("$",)X
3869(the)X
4003(string)X
1324 4712(must)N
1520(match)X
1760(to)X
1853(the)X
1985(end)X
2136(of)X
2233(the)X
2365(word)X
2569(\(these)X
2803(two)X
2959(may)X
3135(be)X
3242(combined)X
3614(to)X
3707(force)X
3911(an)X
4018(exact)X
1324 4816(match\).)N
1627(If)X
1721(a)X
1796(backslash)X
2174(preceeds)X
2517(these)X
2734(two)X
2902(characters,)X
3316(however,)X
3676(they)X
3864(lose)X
4042(their)X
1324 4920(special)N
1608(meaning.)X
1972(Variable)X
2313(expansion)X
2708(also)X
2888(occurs)X
3155(in)X
3262(the)X
3408(normal)X
3696(fashion)X
3993(inside)X
1324 5024(both)N
1518(the)X
2 f
1663(search-string)X
1 f
2169(and)X
2333(the)X
2 f
2478(replacement-string)X
3185(,)X
3 f
3244(except)X
1 f
3520(that)X
3690(a)X
3766(backslash)X
4145(is)X
1324 5128(used)N
1511(to)X
1606(prevent)X
1896(the)X
2030(expansion)X
2413(of)X
2512(a)X
2577("$",)X
2741(not)X
2880(another)X
3170(dollar)X
3402(sign,)X
3596(as)X
3694(is)X
3778(usual.)X
4033(Note)X
1324 5232(that)N
2 f
1494(search-string)X
1 f
2000(is)X
2096(just)X
2261(a)X
2337(string,)X
2597(not)X
2747(a)X
2823(pattern,)X
3127(so)X
3242(none)X
3450(of)X
3560(the)X
3704(usual)X
3926(regular-)X
1324 5336(expression/wildcard)N
2060(characters)X
2443(has)X
2587(any)X
2741(special)X
3013(meaning)X
3344(save)X
3527("\303")X
3655(and)X
3809("$".)X
3996(In)X
4096(the)X
1324 5440(replacement)N
1781(string,)X
2030(the)X
2164("&")X
2331(character)X
2678(is)X
2762(replaced)X
3085(by)X
3198(the)X
2 f
3331(search-string)X
1 f
3825(unless)X
4070(it)X
4145(is)X
1324 5544(preceeded)N
1707(by)X
1822(a)X
1887(backslash.)X
2299(You)X
2476(are)X
2609(allowed)X
2914(to)X
3009(use)X
3152(any)X
3305(character)X
3653(except)X
3909(colon)X
4131(or)X
1324 5648(exclamation)N
1786(point)X
1998(to)X
2097(separate)X
2415(the)X
2553(two)X
2714(strings.)X
3000(This)X
3186(so-called)X
3533(delimiter)X
3882(character)X
1324 5752(may)N
1498(be)X
1603(placed)X
1855(in)X
1946(either)X
2169(string)X
2392(by)X
2502(preceeding)X
2910(it)X
2982(with)X
3161(a)X
3222(backslash.)X
10 s
460 6152(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(6)X

7 p
%%Page: 7 7
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
1036 784(T)N
1324(Replaces)X
1664(each)X
1847(word)X
2049(in)X
2140(the)X
2270(variable)X
2576(expansion)X
2955(by)X
3065(its)X
3171(last)X
3316(component)X
3730(\(its)X
3865(``tail''\).)X
1036 936(H)N
1324(This)X
1510(is)X
1598(similar)X
1873(to)X
1970(":T",)X
2171(except)X
2429(that)X
2590(every)X
2813(word)X
3021(is)X
3108(replaced)X
3434(by)X
3550(everything)X
3955(but)X
4096(the)X
1324 1040(tail)N
1460(\(the)X
1619(``head''\).)X
1036 1192(E)N
1324(":E")X
1497(replaces)X
1807(each)X
1990(word)X
2192(by)X
2302(its)X
2408(suf\256x)X
2630(\(``extension''\).)X
1036 1344(R)N
1324(This)X
1503(replaces)X
1813(each)X
1996(word)X
2198(by)X
2308(everything)X
2707(but)X
2842(the)X
2972(suf\256x)X
3194(\(the)X
3353(``root'')X
3633(of)X
3728(the)X
3858(word\).)X
748 1496(In)N
843(addition,)X
1177(PMake)X
1448(supports)X
1768(the)X
1898(System)X
2180(V)X
2265(form)X
2458(of)X
2553(substitution)X
2988(\()X
2 f
3017(string1)X
1 f
3267(=)X
2 f
3317(string2)X
1 f
3567(\).)X
3 f
10 s
460 1648(COMMENTS,)N
993(INCLUSION)X
1479(AND)X
1686(CONDITIONALS)X
1 f
11 s
748 1752(Make\256le)N
1104(inclusion)X
1471(and)X
1641(conditional)X
2082(structures)X
2467(reminiscent)X
2922(of)X
3038(the)X
3189(C)X
3290(compiler)X
3646(have)X
3854(also)X
4038(been)X
748 1856(included)N
1074(in)X
2 f
1165(PMake)X
1 f
1421(.)X
748 2008(Comments)N
1155(begin)X
1375(with)X
1556(a)X
1619(`#')X
1745(anywhere)X
2110(but)X
2247(in)X
2340(a)X
2403(shell)X
2594(command)X
2966(and)X
3117(continue)X
3445(to)X
3538(the)X
3670(end)X
3821(of)X
3918(the)X
4049(line.)X
748 2112(If)N
829(the)X
960(`#')X
1085(comes)X
1333(at)X
1420(the)X
1551(beginning)X
1927(of)X
2023(the)X
2154(line,)X
2332(however,)X
2679(the)X
2810(following)X
3176(keywords)X
3540(are)X
3669(recognized)X
4077(and)X
748 2216(acted)N
956(on:)X
3 f
10 s
748 2368(include)N
1029('')X
2 f
1083(make\256le)X
3 f
1351('')X
748 2520(include)N
1029(<)X
2 f
1075(system)X
1322(make\256le)X
3 f
1590(>)X
1 f
11 s
748 2672(This)N
928(is)X
1010(very)X
1189(similar)X
1457(to)X
1548(the)X
1678(C)X
1759(compiler's)X
2158(\256le-inclusion)X
2646(facility,)X
2941(right)X
3130(down)X
3347(to)X
3438(the)X
3568(syntax.)X
3864(What)X
4077(fol-)X
748 2776(lows)N
943(the)X
3 f
1080(#include)X
1 f
1428(must)X
1629(be)X
1741(a)X
1809(\256lename)X
2142(enclosed)X
2479(either)X
2709(in)X
2807(double-quotes)X
3334(or)X
3435(angle)X
3654(brackets.)X
4019(Vari-)X
748 2880(ables)N
957(will)X
1123(be)X
1233(expanded)X
1597(between)X
1917(the)X
2052(double-quotes)X
2578(or)X
2678(angle-brackets.)X
3262(If)X
3347(angle-brackets)X
3887(are)X
4021(used,)X
748 2984(the)N
879(system)X
1147(make\256le)X
1474(directory)X
1815(is)X
1897(searched.)X
2271(If)X
2352(the)X
2482(name)X
2695(is)X
2776(enclosed)X
3106(in)X
3197(double-quotes,)X
3740(the)X
3870(including)X
748 3088(make\256le's)N
1143(directory,)X
1511(followed)X
1852(by)X
1968(all)X
2085(directories)X
2485(given)X
2709(via)X
3 f
9 f
2844(-)X
3 f
2892(I)X
1 f
2953(arguments,)X
3369(followed)X
3709(by)X
3824(the)X
3959(system)X
748 3192(directory,)N
1110(is)X
1191(searched)X
1520(for)X
1644(a)X
1705(\256le)X
1840(of)X
1935(the)X
2065(given)X
2283(name.)X
748 3344(If)N
843(the)X
988(\256le)X
1138(is)X
1234(found,)X
2 f
1498(PMake)X
1 f
1784(starts)X
2007(taking)X
2265(input)X
2484(from)X
2692(that)X
2862(\256le)X
3012(as)X
3121(if)X
3211(it)X
3297(were)X
3503(part)X
3676(of)X
3785(the)X
3929(original)X
748 3448(make\256le.)N
748 3600(When)N
991(the)X
1132(end)X
1292(of)X
1398(the)X
1539(\256le)X
1685(is)X
1777(reached,)X
2 f
2105(PMake)X
1 f
2387(goes)X
2581(back)X
2780(to)X
2882(the)X
3023(previous)X
3359(\256le)X
3504(and)X
3663(continues)X
4033(from)X
748 3704(where)N
988(it)X
1064(left)X
1208(off.)X
1380(This)X
1563(facility)X
1840(is)X
1925(recursive)X
2272(up)X
2385(to)X
2479(a)X
2543(depth)X
2764(limited)X
3041(only)X
3223(by)X
3336(the)X
3469(number)X
3763(of)X
3861(open)X
4057(\256les)X
748 3808(allowed)N
1049(to)X
1140(any)X
1289(process)X
1574(at)X
1660(one)X
1809(time.)X
3 f
10 s
748 3960(if)N
817([!])X
2 f
918(expr)X
3 f
1081([)X
2 f
1128(op)X
1228(expr)X
3 f
1391(...)X
1471(])X
748 4112(ifdef)N
937([!])X
2 f
1051(variable)X
3 f
1351([)X
2 f
1378(op)X
3 f
2 f
1491(variable)X
3 f
1758(...])X
748 4264(ifndef)N
981([!])X
2 f
1095(variable)X
3 f
1395([)X
2 f
1422(op)X
3 f
2 f
1535(variable)X
3 f
1802(...])X
748 4416(ifmake)N
1017([!])X
2 f
1131(target)X
3 f
1355([)X
2 f
1382(op)X
3 f
2 f
1495(target)X
3 f
1686(...])X
748 4568(ifnmake)N
1061([!])X
2 f
1175(target)X
3 f
1399([)X
2 f
1426(op)X
3 f
2 f
1539(target)X
3 f
1730(...])X
1 f
11 s
748 4720(These)N
984(are)X
1117(all)X
1232(the)X
1366(beginnings)X
1779(of)X
1878(conditional)X
2302(constructs)X
2685(in)X
2780(the)X
2914(spirit)X
3122(of)X
3221(the)X
3354(C)X
3438(compiler.)X
3821(Condition-)X
748 4824(als)N
868(may)X
1042(be)X
1147(nested)X
1394(to)X
1485(a)X
1546(depth)X
1764(of)X
1859(thirty.)X
748 4976(In)N
847(the)X
981(expressions)X
1417(given)X
1639(above,)X
2 f
1897(op)X
1 f
2011(may)X
2189(be)X
2298(either)X
3 f
2525(||)X
1 f
2589(\(logical)X
9 s
2881(OR)X
11 s
(\))S
3036(or)X
3 f
3135(&&)X
1 f
3307(\(logical)X
9 s
3598(AND)X
11 s
(\).)S
3 f
3852(&&)X
1 f
4023(has)X
4165(a)X
748 5080(higher)N
997(precedence)X
1416(than)X
3 f
1592(||)X
1 f
1630(.)X
1698(As)X
1819(in)X
1912(C,)X
2 f
2017(PMake)X
1 f
2290(will)X
2452(evaluate)X
2770(an)X
2877(expression)X
3277(only)X
3458(as)X
3555(far)X
3675(as)X
3771(necessary)X
4135(to)X
748 5184(determine)N
1124(its)X
1231(value.)X
1467(I.e.)X
1602(if)X
1679(the)X
1810(left)X
1951(side)X
2116(of)X
2212(an)X
3 f
2318(&&)X
1 f
2487(is)X
2569(false,)X
2780(the)X
2910(expression)X
3308(is)X
3389(false)X
3577(and)X
3726(vice)X
3895(versa)X
4102(for)X
3 f
748 5288(||)N
1 f
786(.)X
852(Parentheses)X
1289(may)X
1463(be)X
1568(used)X
1751(as)X
1846(usual)X
2054(to)X
2145(change)X
2416(the)X
2546(order)X
2753(of)X
2848(evaluation.)X
748 5440(One)N
928(other)X
1143(boolean)X
1456(operator)X
1783(is)X
1876(provided:)X
3 f
2248(!)X
1 f
2311(\(logical)X
2615(negation\).)X
3004(It)X
3092(is)X
3185(of)X
3292(a)X
3365(higher)X
3624(precedence)X
4052(than)X
748 5544(either)N
980(the)X
9 s
1115(AND)X
11 s
1302(or)X
9 s
1402(OR)X
11 s
1533(operators,)X
1913(and)X
2071(may)X
2253(be)X
2366(applied)X
2656(in)X
2755(any)X
2912(of)X
3015(the)X
3153(``if'')X
3353(constructs,)X
3762(negating)X
4096(the)X
748 5648(given)N
966(function)X
1282(for)X
1406(``#if'')X
1642(or)X
1737(the)X
1867 0.4732(implicit)AX
2166(function)X
2482(for)X
2606(the)X
2736(other)X
2939(four.)X
10 s
460 6152(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(7)X

8 p
%%Page: 8 8
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
2 f
748 784(Expr)N
1 f
950(can)X
1103(be)X
1217(one)X
1375(of)X
1479(several)X
1758(things.)X
2026(Four)X
2222(functions)X
2580(are)X
2717(provided,)X
3082(each)X
3273(of)X
3376(which)X
3621(takes)X
3832(a)X
3901(different)X
748 888(sort)N
902(of)X
997(argument.)X
748 1040(The)N
914(function)X
3 f
1237(de\256ned)X
1 f
1540(is)X
1628(used)X
1818(to)X
1915(test)X
2066(for)X
2196(the)X
2332(existence)X
2688(of)X
2789(a)X
2856(variable.)X
3212(Its)X
3328(argument)X
3689(is,)X
3798(therefore,)X
4165(a)X
748 1144(variable)N
1055(name.)X
1312(Certain)X
1594(lower-case)X
1996(variable)X
2302(names)X
2549(\(e.g.)X
2727(``sun'',)X
3009(``unix'')X
3304(and)X
3453(``sprite''\))X
3816(are)X
3945(de\256ned)X
748 1248(in)N
846(the)X
983(system)X
1257(make\256le)X
1590(\(qv.)X
3 f
1757(FILES)X
1 f
2012(\))X
2069(to)X
2166(specify)X
2448(the)X
2584(sort)X
2744(of)X
2845(system)X
3118(on)X
3234(which)X
2 f
3477(PMake)X
1 f
3754(is)X
3841(being)X
4065(run.)X
748 1352(These)N
984(are)X
1117(intended)X
1447(to)X
1541(make)X
1757(make\256les)X
2120(more)X
2326(portable.)X
2684(Any)X
2860(variable)X
3169(may)X
3346(be)X
3454(used)X
3640(as)X
3738(the)X
3871(argument)X
748 1456(of)N
843(the)X
3 f
973(de\256ned)X
1 f
1269(function.)X
748 1608(The)N
3 f
909(make)X
1 f
1138(function)X
1456(is)X
1539(given)X
1759(the)X
1891(name)X
2106(of)X
2203(a)X
2266(target)X
2491(in)X
2584(the)X
2715(make\256le)X
3042(and)X
3192(evaluates)X
3543(to)X
3635(true)X
3795(if)X
3872(the)X
4003(target)X
748 1712(was)N
907(given)X
1126(on)X
2 f
1237(PMake)X
1 f
1493('s)X
1579(command-line)X
2112(or)X
2208(as)X
2304(a)X
2366(source)X
2618(for)X
2743(the)X
3 f
2874(.MAIN)X
1 f
3161(target)X
3384(before)X
3630(the)X
3760(line)X
3915(contain-)X
748 1816(ing)N
883(the)X
1013(conditional.)X
748 1968(The)N
3 f
918(exists)X
1 f
1156(function)X
1483(takes)X
1697(a)X
1769(\256le)X
1915(name,)X
2161(which)X
2409(\256le)X
2555(is)X
2647(searched)X
2987(for)X
3122(on)X
3243(the)X
3384(system)X
3662(search)X
3918(path)X
4102(\(as)X
748 2072(de\256ned)N
1029(by)X
3 f
1139(.PATH)X
1 f
1428(targets)X
1685(\(see)X
1848(below\)\).)X
2165(It)X
2241(evaluates)X
2591(true)X
2750(if)X
2826(the)X
2956(\256le)X
3091(is)X
3172(found.)X
3 f
748 2224(empty)N
1 f
1018(takes)X
1235(a)X
1310(variable)X
1630(expansion)X
2023(speci\256cation)X
2505(\(minus)X
2786(the)X
2930(dollar)X
3172(sign\))X
3384(as)X
3493(its)X
3613(argument.)X
4003(If)X
4096(the)X
748 2328(resulting)N
1079(expansion)X
1458(is)X
1539(empty,)X
1804(this)X
1954(evaluates)X
2304(true.)X
2 f
748 2480(Expr)N
1 f
958(can)X
1119(also)X
1300(be)X
1422(an)X
1543(arithmetic)X
1940(or)X
2051(string)X
2290(comparison,)X
2761(with)X
2956(the)X
3102(lefthand)X
3429(side)X
3609(being)X
3843(a)X
3920(variable)X
748 2584(expansion.)N
1159(The)X
1328(standard)X
1658(C)X
1748(relational)X
2113(operators)X
2471(are)X
2609(allowed,)X
2941(and)X
3099(the)X
3238(usual)X
3455(number/base)X
3936(conver-)X
748 2688(sion)N
923(is)X
1010(performed,)X
1426(with)X
1611(the)X
1747(exception)X
2118(that)X
2278(octal)X
2477(numbers)X
2807(are)X
2941(not)X
3081(supported.)X
3477(If)X
3562(the)X
3697(righthand)X
4062(side)X
748 2792(of)N
846(a)X
910("==")X
1106(or)X
1203("!=")X
1378(operator)X
1695(begins)X
1949(with)X
2130(a)X
2193(quotation)X
2551(mark,)X
2778(a)X
2841(string)X
3066(comparison)X
3501(is)X
3584(done)X
3779(between)X
4096(the)X
748 2896(expanded)N
1110(variable)X
1419(and)X
1571(the)X
1704(text)X
1862(between)X
2180(the)X
2313(quotation)X
2672(marks.)X
2956(If)X
3039(no)X
3152(relational)X
3511(operator)X
3829(is)X
3912(given,)X
4154(it)X
748 3000(is)N
844(assumed)X
1184(that)X
1354(the)X
1499(expanded)X
1873(variable)X
2194(is)X
2290(to)X
2396(be)X
2516(compared)X
2900(against)X
3187(0,)X
3290(i.e.)X
3457(it)X
3543(is)X
3638(interpreted)X
4056(as)X
4165(a)X
748 3104(boolean,)N
1071(with)X
1250(a)X
1311(0)X
1377(value)X
1590(being)X
1808(false)X
1996(and)X
2145(a)X
2206(non-zero)X
2540(value)X
2753(being)X
2971(true.)X
748 3256(When,)N
1007(in)X
1103(the)X
1238(course)X
1494(of)X
1594(evaluating)X
1989(one)X
2143(of)X
2243(these)X
2451(conditional)X
2876(expressions,)X
2 f
3335(PMake)X
1 f
3611(encounters)X
4018(some)X
748 3360(word)N
951(it)X
1024(does)X
1208(not)X
1344(recognize,)X
1731(it)X
1804(applies)X
2077(one)X
2227(of)X
2323(either)X
2 f
2546(make)X
1 f
2753(or)X
2 f
2848(de\256ned)X
1 f
3124(to)X
3215(it,)X
3309(depending)X
3698(on)X
3808(the)X
3938(form)X
4131(of)X
748 3464(``if'')N
943(used.)X
1151(E.g.)X
1318(``#ifdef'')X
1669(will)X
1832(apply)X
2052(the)X
2 f
2184(de\256ned)X
1 f
2462(function,)X
2802(while)X
3022(``#ifnmake'')X
3495(will)X
3657(apply)X
3877(the)X
4009(nega-)X
748 3568(tion)N
908(of)X
1003(the)X
2 f
1133(make)X
1 f
1340(function.)X
748 3720(If)N
833(the)X
967(expression)X
1369(following)X
1738(one)X
1891(of)X
1990(these)X
2197(forms)X
2428(evaluates)X
2782(true,)X
2967(the)X
3101(reading)X
3391(of)X
3490(the)X
3624(make\256le)X
3954(contin-)X
748 3824(ues)N
895(as)X
998(before.)X
1274(If)X
1362(it)X
1442(evaluates)X
1800(false,)X
2018(the)X
2156(following)X
2528(lines)X
2724(are)X
2860(skipped.)X
3185(In)X
3287(both)X
3473(cases,)X
3709(this)X
3866(continues)X
748 3928(until)N
933(either)X
1156(an)X
3 f
1261(#else)X
1 f
1464(or)X
1559(an)X
3 f
1664(#endif)X
1 f
1921(line)X
2076(is)X
2157(encountered.)X
3 f
10 s
748 4080(else)N
1 f
11 s
748 4232(The)N
917(#else,)X
1152(as)X
1257(in)X
1358(the)X
1498(C)X
1589(compiler,)X
1957(causes)X
2218(the)X
2357(sense)X
2578(of)X
2682(the)X
2821(last)X
2975(conditional)X
3404(to)X
3504(be)X
3618(inverted)X
3938(and)X
4096(the)X
748 4336(reading)N
1042(of)X
1145(the)X
1283(make\256le)X
1616(to)X
1714(be)X
1826(based)X
2055(on)X
2172(this)X
2329(new)X
2504(value.)X
2768(I.e.)X
2909(if)X
2992(the)X
3129(previous)X
3461(expression)X
3866(evaluated)X
748 4440(true,)N
933(the)X
1067(parsing)X
1352(of)X
1451(the)X
1585(make\256le)X
1915(is)X
2000(suspended)X
2392(until)X
2581(an)X
2690(#endif)X
2940(line)X
3098(is)X
3182(read.)X
3402(If)X
3485(the)X
3618(previous)X
3946(expres-)X
748 4544(sion)N
917(evaluated)X
1277(false,)X
1487(the)X
1617(parsing)X
1898(of)X
1993(the)X
2123(make\256le)X
2449(is)X
2530(resumed.)X
3 f
10 s
748 4696(elif)N
875([!])X
2 f
976(expr)X
3 f
1139([)X
2 f
1186(op)X
1286(expr)X
3 f
1449(...)X
1529(])X
748 4848(elifdef)N
995([!])X
2 f
1109(variable)X
3 f
1409([)X
2 f
1436(op)X
3 f
2 f
1549(variable)X
3 f
1816(...])X
748 5000(elifndef)N
1039([!])X
2 f
1153(variable)X
3 f
1453([)X
2 f
1480(op)X
3 f
2 f
1593(variable)X
3 f
1860(...])X
748 5152(elifmake)N
1075([!])X
2 f
1189(target)X
3 f
1413([)X
2 f
1440(op)X
3 f
2 f
1553(target)X
3 f
1744(...])X
748 5304(elifnmake)N
1119([!])X
2 f
1233(target)X
3 f
1457([)X
2 f
1484(op)X
3 f
2 f
1597(target)X
3 f
1788(...])X
1 f
11 s
748 5456(The)N
911(``elif'')X
1171(constructs)X
1554(are)X
1687(a)X
1752(combination)X
2220(of)X
2319(``else'')X
2598(and)X
2751(``if,'')X
2969(as)X
3068(the)X
3201(name)X
3417(implies.)X
3725(If)X
3808(the)X
3941(preced-)X
748 5560(ing)N
885(``if'')X
1079(evaluated)X
1441(false,)X
1653(the)X
1785(expression)X
2185(following)X
2552(the)X
2684(``elif'')X
2942(is)X
3025(evaluated)X
3387(and)X
3538(the)X
3670(lines)X
3861(following)X
748 5664(it)N
826(are)X
961(read)X
1140(or)X
1241(ignored)X
1538(the)X
1674(same)X
1883(as)X
1984(for)X
2114(a)X
2181(regular)X
2458(``if.'')X
2678(If)X
2764(the)X
2900(preceding)X
3275(``if'')X
3473(evaluated)X
3838(true,)X
4024(how-)X
748 5768(ever,)N
943(the)X
1073(``elif'')X
1329(is)X
1410(ignored)X
1701(and)X
1850(all)X
1961(following)X
2326(lines)X
2515(until)X
2700(the)X
2830(``endif'')X
3149(\(see)X
3312(below\))X
3578(are)X
3707(ignored.)X
10 s
460 6160(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(8)X

9 p
%%Page: 9 9
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
3 f
10 s
748 784(endif)N
11 s
748 936(#endif)N
1 f
1024(is)X
1124(used)X
1326(to)X
1436(end)X
1604(a)X
1684(conditional)X
2123(section.)X
2436(If)X
2535(lines)X
2743(were)X
2954(being)X
3191(skipped,)X
3528(the)X
3677(reading)X
3982(of)X
4096(the)X
748 1040(make\256le)N
1081(resumes.)X
1420(Otherwise,)X
1832(it)X
1911(has)X
2057(no)X
2174(effect)X
2403(\(the)X
2569(make\256le)X
2902(continues)X
3268(to)X
3365(be)X
3476(parsed)X
3733(as)X
3834(it)X
3912(was)X
4076(just)X
748 1144(before)N
994(the)X
3 f
1124(#endif)X
1 f
1381(was)X
1539(encountered\).)X
3 f
10 s
748 1296(undef)N
1 f
11 s
748 1448(Takes)N
985(the)X
1120(next)X
1299(word)X
1506(on)X
1621(the)X
1756(line)X
1916(as)X
2016(a)X
2082(global)X
2329(variable)X
2639(to)X
2734(be)X
2843(unde\256ned)X
3216(\(only)X
3428(unde\256nes)X
3791(global)X
4038(vari-)X
748 1552(ables,)N
973(not)X
1108(command-line)X
1640(variables\).)X
2031(If)X
2111(the)X
2241(variable)X
2547(is)X
2628(already)X
2909(unde\256ned,)X
3300(no)X
3410(message)X
3730(is)X
3811(generated.)X
3 f
10 s
460 1704(TARGET)N
830(ATTRIBUTES)X
1 f
11 s
748 1808(In)N
2 f
844(PMake)X
1 f
1100(,)X
1145(\256les)X
1315(can)X
1460(have)X
1649(certain)X
1912(``attributes.'')X
2402(These)X
2635(attributes)X
2987(cause)X
2 f
3205(PMake)X
1 f
3477(to)X
3569(treat)X
3748(the)X
3878(targets)X
4135(in)X
748 1912(special)N
1023(ways.)X
1255(An)X
1392(attribute)X
1717(is)X
1806(a)X
1875(special)X
2150(word)X
2360(given)X
2586(as)X
2689(a)X
2758(source)X
3017(to)X
3116(a)X
3185(target)X
3415(on)X
3532(a)X
3600(dependency)X
4049(line.)X
748 2016(The)N
907(words)X
1143(and)X
1292(their)X
1476(functions)X
1826(are)X
1955(given)X
2173(below:)X
748 2168(.DONTCARE)N
1400(If)X
1495(a)X
1571(target)X
1809(is)X
1905(marked)X
2206(with)X
2400(this)X
2565(attribute)X
2897(and)X
3061(PMake)X
3346(can't)X
3558(\256gure)X
3799(out)X
3948(how)X
4135(to)X
1400 2272(create)N
1636(it,)X
1734(it)X
1810(will)X
1974(ignore)X
2225(this)X
2379(fact)X
2537(and)X
2690(assume)X
2975(the)X
3109(\256le)X
3248(isn't)X
3430(really)X
3656(needed)X
3930(or)X
4028(actu-)X
1400 2376(ally)N
1555(exists)X
1778(and)X
1927(PMake)X
2198(just)X
2348(can't)X
2546(\256nd)X
2705(it.)X
748 2528(.EXEC)N
1400(This)X
1587(causes)X
1846(the)X
1984(marked)X
2278(target's)X
2572(shell)X
2768(script)X
2993(to)X
3091(always)X
3364(be)X
3476(executed)X
3818(\(unless)X
4096(the)X
3 f
9 f
1400 2632(-)N
3 f
1448(n)X
1 f
1519(or)X
3 f
9 f
1614(-)X
3 f
1662(t)X
1 f
1713(\257ag)X
1867(is)X
1948(given\),)X
2217(but)X
2352(appear)X
2608(invisible)X
2935(to)X
3026(any)X
3175(targets)X
3432(that)X
3587(depend)X
3863(on)X
3973(it.)X
748 2784(.EXPORT)N
1400(This)X
1592(is)X
1686(used)X
1882(to)X
1986(mark)X
2202(those)X
2423(targets)X
2693(whose)X
2952(creation)X
3271(should)X
3541(be)X
3659(sent)X
3836(to)X
3940(another)X
1400 2888(machine)N
1731(if)X
1817(at)X
1913(all)X
2033(possible.)X
2375(This)X
2563(may)X
2746(be)X
2860(used)X
3052(by)X
3171(some)X
3388(exportation)X
3821(schemes)X
4150(if)X
1400 2992(the)N
1537(exportation)X
1968(is)X
2056(expensive.)X
2458(You)X
2637(should)X
2900(ask)X
3045(your)X
3234(administrator)X
3733(if)X
3815(it)X
3893(is)X
3980(neces-)X
1400 3096(sary.)N
748 3248(.EXPORTSAME)N
1400(Tells)X
1612(the)X
1755(export)X
2015(system)X
2295(that)X
2463(the)X
2605(job)X
2752(should)X
3021(be)X
3138(exported)X
3480(to)X
3583(a)X
3656(machine)X
3989(of)X
4096(the)X
1400 3352(same)N
1623(architecture)X
2081(as)X
2196(the)X
2346(current)X
2637(one.)X
2828(Certain)X
3129(operations)X
3537(\(e.g.)X
3756(running)X
4071(text)X
1400 3456(through)N
1714("nroff"\))X
2030(can)X
2192(be)X
2315(performed)X
2721(the)X
2869(same)X
3090(on)X
3217(any)X
3383(architecture)X
3838(\(CPU)X
4077(and)X
1400 3560(operating)N
1756(system)X
2024(type\),)X
2250(while)X
2469(others)X
2707(\(e.g.)X
2886(compiling)X
3268(a)X
3330(program)X
3651(with)X
3831("cc"\))X
4032(must)X
1400 3664(be)N
1510(performed)X
1903(on)X
2018(a)X
2084(machine)X
2410(with)X
2594(the)X
2729(same)X
2937(architecture.)X
3401(Not)X
3559(all)X
3674(export)X
3925(systems)X
1400 3768(will)N
1560(support)X
1846(this)X
1996(attribute.)X
748 3920(.IGNORE)N
1400(Giving)X
1681(a)X
1756(target)X
1993(the)X
3 f
2137(.IGNORE)X
1 f
2552(attribute)X
2882(causes)X
3146(PMake)X
3430(to)X
3534(ignore)X
3794(errors)X
4033(from)X
1400 4024(any)N
1549(of)X
1644(the)X
1774(target's)X
2060(commands,)X
2486(as)X
2581(if)X
2657(they)X
2831(all)X
2942(had)X
3091(`)X
9 f
3120(-)X
1 f
3168(')X
3219(before)X
3465(them.)X
748 4176(.INVISIBLE)N
1400(This)X
1588(allows)X
1849(you)X
2011(to)X
2110(specify)X
2394(one)X
2551(target)X
2782(as)X
2885(a)X
2954(source)X
3213(for)X
3345(another)X
3639(without)X
3939(the)X
4077(one)X
1400 4280(affecting)N
1735(the)X
1865(other's)X
2131(local)X
2325(variables.)X
748 4432(.JOIN)N
1400(This)X
1588(forces)X
1833(the)X
1972(target's)X
2267(shell)X
2465(script)X
2692(to)X
2792(be)X
2906(executed)X
3250(only)X
3438(if)X
3522(one)X
3679(or)X
3782(more)X
3993(of)X
4096(the)X
1400 4536(sources)N
1692(was)X
1857(out-of-date.)X
2299(In)X
2401(addition,)X
2742(the)X
2879(target's)X
3172(name,)X
3414(in)X
3512(both)X
3698(its)X
3 f
3810(.TARGET)X
1 f
1400 4640(variable)N
1711(and)X
1865(all)X
1981(the)X
2115(local)X
2313(variables)X
2657(of)X
2756(any)X
2909(target)X
3136(that)X
3295(depends)X
3609(on)X
3723(it,)X
3821(is)X
3906(replaced)X
1400 4744(by)N
1512(the)X
1643(value)X
1857(of)X
1953(its)X
3 f
2060(.ALLSRC)X
1 f
2461(variable.)X
2812(Another)X
3123(aspect)X
3366(of)X
3462(the)X
3593(.JOIN)X
3827(attribute)X
4145(is)X
1400 4848(it)N
1472(keeps)X
1694(the)X
1824(target)X
2047(from)X
2240(being)X
2458(created)X
2734(if)X
2810(the)X
3 f
9 f
2940(-)X
3 f
2988(t)X
1 f
3039(\257ag)X
3193(was)X
3351(given.)X
748 5000(.MAKE)N
1400(The)X
3 f
1583(.MAKE)X
1 f
1925(attribute)X
2266(marks)X
2527(its)X
2657(target)X
2903(as)X
3021(being)X
3262(a)X
3346(recursive)X
3713(invocation)X
4131(of)X
1400 5104(PMake.)N
1722(This)X
1908(forces)X
2150(PMake)X
2427(to)X
2524(execute)X
2821(the)X
2957(script)X
3181(associated)X
3571(with)X
3756(the)X
3892(target)X
4121(\(if)X
1400 5208(it's)N
1535(out-of-date\))X
1977(even)X
2165(if)X
2241(you)X
2395(gave)X
2583(the)X
3 f
9 f
2713(-)X
3 f
2761(n)X
1 f
2832(or)X
3 f
9 f
2927(-)X
3 f
2975(t)X
1 f
3026(\257ag.)X
748 5360(.NOEXPORT)N
1400(Forces)X
1665(the)X
1804(target)X
2036(to)X
2136(be)X
2249(created)X
2533(locally,)X
2826(even)X
3022(if)X
3106(you've)X
3380(given)X
2 f
3606(PMake)X
1 f
3885(the)X
3 f
9 f
4023(-)X
3 f
4071(L)X
4160(0)X
1 f
1400 5464(\257ag.)N
748 5616(.NOTMAIN)N
1400(Normally,)X
1785(if)X
1864(you)X
2021(do)X
2134(not)X
2272(specify)X
2551(a)X
2615(target)X
2841(to)X
2935(make)X
3151(in)X
3245(any)X
3396(other)X
3601(way,)X
2 f
3793(PMake)X
1 f
4066(will)X
1400 5720(take)N
1578(the)X
1717(\256rst)X
1885(target)X
2117(on)X
2236(the)X
2375(\256rst)X
2543(dependency)X
2994(line)X
3157(of)X
3260(a)X
3329(make\256le)X
3663(as)X
3766(the)X
3904(target)X
4135(to)X
1400 5824(create.)N
1676(Giving)X
1943(a)X
2004(target)X
2227(this)X
2377(attribute)X
2694(keeps)X
2916(it)X
2988(from)X
3181(this)X
3331(fate.)X
10 s
460 6216(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4164(9)X

10 p
%%Page: 10 10
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
748 784(.PRECIOUS)N
1400(When)X
1635(PMake)X
1909(is)X
1993(interrupted,)X
2427(it)X
2501(will)X
2663(attempt)X
2953(to)X
3046(clean)X
3256(up)X
3368(after)X
3553(itself)X
3754(by)X
3866(removing)X
1400 888(any)N
1553(half-made)X
1936(targets.)X
2219(If)X
2303(a)X
2368(target)X
2595(has)X
2737(this)X
2890(attribute,)X
3232(however,)X
2 f
3581(PMake)X
1 f
3855(will)X
4018(leave)X
1400 992(it)N
1472(alone)X
748 1144(.SILENT)N
1400(Marking)X
1740(a)X
1816(target)X
2053(with)X
2246(this)X
2410(attribute)X
2741(keeps)X
2977(its)X
3097(commands)X
3515(from)X
3722(being)X
3954(printed)X
1400 1248(when)N
1612(they're)X
1883(executed.)X
748 1400(.USE)N
1400(By)X
1527(giving)X
1777(a)X
1840(target)X
2065(this)X
2217(attribute,)X
2558(you)X
2714(turn)X
2880(the)X
3012(target)X
3237(into)X
2 f
3398(PMake)X
1 f
3654('s)X
3740(equivalent)X
4131(of)X
1400 1504(a)N
1464(macro.)X
1731(When)X
1966(the)X
2099(target)X
2324(is)X
2407(used)X
2592(as)X
2689(a)X
2752(source)X
3005(for)X
3131(another)X
3419(target,)X
3666(the)X
3798(other)X
4003(target)X
1400 1608(acquires)N
1720(the)X
1854(commands,)X
2284(sources)X
2573(and)X
2726(attributes)X
3081(\(except)X
3 f
3366(.USE)X
1 f
3559(\))X
3614(of)X
3713(the)X
3847(source.)X
4146(If)X
1400 1712(the)N
1531(target)X
1755(already)X
2037(has)X
2177(commands,)X
2604(the)X
3 f
2735(.USE)X
1 f
2951(target's)X
3238(commands)X
3643(are)X
3773(added)X
4005(to)X
4096(the)X
1400 1816(end.)N
1582(If)X
1673(more)X
1887(than)X
2072(one)X
2232(.USE-marked)X
2746(source)X
3008(is)X
3099(given)X
3327(to)X
3428(a)X
3499(target,)X
3754(the)X
3894(rules)X
4097(are)X
1400 1920(applied)N
1682(sequentially.)X
3 f
10 s
460 2072(SPECIAL)N
839(TARGETS)X
1 f
11 s
748 2176(As)N
876(there)X
1083(were)X
1284(in)X
1384(Make,)X
1637(so)X
1746(there)X
1953(are)X
2091(certain)X
2361(targets)X
2626(that)X
2789(have)X
2985(special)X
3260(meaning)X
3594(to)X
3693(PMake.)X
3994(When)X
748 2280(you)N
902(use)X
1041(one)X
1190(on)X
1300(a)X
1361(dependency)X
1803(line,)X
1980(it)X
2052(is)X
2133(the)X
2263(only)X
2442(target)X
2665(that)X
2820(may)X
2994(appear)X
3250(on)X
3360(the)X
3490(left-hand-side)X
4001(of)X
4096(the)X
748 2384(operator.)N
1107(The)X
1266(targets)X
1523(are)X
1652(as)X
1747(follows:)X
748 2536(.BEGIN)N
1350(Any)X
1525(commands)X
1931(attached)X
2249(to)X
2342(this)X
2494(target)X
2719(are)X
2850(executed)X
3187(before)X
3435(anything)X
3768(else)X
3929(is)X
4011(done.)X
1350 2640(You)N
1523(can)X
1667(use)X
1806(it)X
1878(for)X
2002(any)X
2151 0.3317(initialization)AX
2622(that)X
2777(needs)X
2999(doing.)X
748 2792(.DEFAULT)N
1350(This)X
1538(is)X
1628(sort)X
1791(of)X
1895(a)X
1965(.USE)X
2184(rule)X
2352(for)X
2485(any)X
2643(target)X
2875(\(that)X
3067(was)X
3233(used)X
3424(only)X
3611(as)X
3714(a)X
3783(source\))X
4071(that)X
2 f
1350 2896(PMake)N
1 f
1621(can't)X
1819(\256gure)X
2046(out)X
2181(any)X
2330(other)X
2533(way)X
2701(to)X
2792(create.)X
3046(Only)X
3244(the)X
3374(shell)X
3563(script)X
3781(is)X
3862(used.)X
4067(The)X
3 f
1350 3000(.IMPSRC)N
1 f
1750(variable)X
2066(of)X
2171(a)X
2242(target)X
2475(that)X
2640(inherits)X
3 f
2936(.DEFAULT)X
3409('s)X
1 f
3503(commands)X
3916(is)X
4006(set)X
4135(to)X
1350 3104(the)N
1480(target's)X
1766(own)X
1939(name.)X
748 3256(.END)N
1350(This)X
1531(serves)X
1774(a)X
1837(function)X
2155(similar)X
2425(to)X
3 f
2518(.BEGIN)X
1 f
2824(:)X
2873(commands)X
3279(attached)X
3596(to)X
3688(it)X
3761(are)X
3891(executed)X
1350 3360(once)N
1558(everything)X
1977(has)X
2135(been)X
2342(re-created)X
2734(\(so)X
2882(long)X
3080(as)X
3194(no)X
3323(errors)X
3568(occurred\).)X
3967(It)X
4062(also)X
1350 3464(serves)N
1595(the)X
1729(extra)X
1931(function)X
2251(of)X
2350(being)X
2572(a)X
2637(place)X
2849(on)X
2963(which)X
3204(PMake)X
3479(can)X
3626(hang)X
3822(commands)X
1350 3568(you)N
1507(put)X
1644(off)X
1770(to)X
1863(the)X
1995(end.)X
2168(Thus)X
2368(the)X
2500(script)X
2720(for)X
2846(this)X
2998(target)X
3223(will)X
3385(be)X
3492(executed)X
3829(before)X
4077(any)X
1350 3672(of)N
1445(the)X
1575(commands)X
1979(you)X
2133(save)X
2311(with)X
2490(the)X
2620(``.)X
2715(.)X
2752(.''.)X
748 3824(.EXPORT)N
1350(The)X
1520(sources)X
1816(for)X
1951(this)X
2112(target)X
2346(are)X
2486(passed)X
2753(to)X
2854(the)X
2994(exportation)X
3428(system)X
3705(compiled)X
4066(into)X
2 f
1350 3928(PMake)N
1 f
1606(.)X
1691(Some)X
1933(systems)X
2253(will)X
2432(use)X
2590(these)X
2812(sources)X
3116(to)X
3226(con\256gure)X
3599(themselves.)X
4053(You)X
1350 4032(should)N
1607(ask)X
1746(your)X
1929(system)X
2196(administrator)X
2689(about)X
2907(this.)X
748 4184(.IGNORE)N
1350(This)X
1535(target)X
1764(marks)X
2007(each)X
2196(of)X
2297(its)X
2409(sources)X
2700(with)X
2885(the)X
3 f
3021(.IGNORE)X
1 f
3428(attribute.)X
3773(If)X
3859(you)X
4018(don't)X
1350 4288(give)N
1524(it)X
1596(any)X
1745(sources,)X
2052(then)X
2226(it)X
2298(is)X
2379(like)X
2534(giving)X
2782(the)X
3 f
9 f
2912(-)X
3 f
2960(i)X
1 f
3007(\257ag.)X
748 4440(.INCLUDES)N
1350(The)X
1512(sources)X
1800(for)X
1927(this)X
2080(target)X
2306(are)X
2438(taken)X
2654(to)X
2748(be)X
2856(suf\256xes)X
3154(that)X
3312(indicate)X
3617(a)X
3681(\256le)X
3818(that)X
3975(can)X
4121(be)X
1350 4544(included)N
1684(in)X
1783(a)X
1852(program)X
2180(source)X
2439(\256le.)X
2626(The)X
2793(suf\256x)X
3023(must)X
3225(have)X
3421(already)X
3710(been)X
3906(declared)X
1350 4648(with)N
3 f
1539(.SUFFIXES)X
1 f
2018(\(see)X
2191(below\).)X
2510(Any)X
2692(suf\256x)X
2923(so)X
3032(marked)X
3327(will)X
3496(have)X
3693(the)X
3832(directories)X
1350 4752(on)N
1475(its)X
1596(search)X
1857(path)X
2046(\(see)X
3 f
2224(.PATH)X
2528(,)X
1 f
2587(below\))X
2868(placed)X
3135(in)X
3241(the)X
3 f
3386(.INCLUDES)X
1 f
3898(variable,)X
1350 4856(each)N
1540(preceeded)X
1925(by)X
2042(a)X
3 f
9 f
2110(-)X
3 f
2158(I)X
1 f
2221(\257ag.)X
2404(The)X
3 f
2570(.h)X
1 f
2670(suf\256x)X
2899(is)X
2987(already)X
3275(marked)X
3568(in)X
3666(this)X
3823(way)X
3998(in)X
4096(the)X
1350 4960(system)N
1617(make\256le.)X
748 5112(.INTERRUPT)N
1350(When)X
1589(PMake)X
1867(is)X
1955(interrupted,)X
2393(it)X
2472(will)X
2639(execute)X
2937(the)X
3074(commands)X
3485(in)X
3583(the)X
3720(script)X
3945(for)X
4076(this)X
1350 5216(target,)N
1595(if)X
1671(it)X
1743(exists.)X
748 5368(.LIBS)N
1350(This)X
1536(does)X
1725(for)X
1855(libraries)X
2172(what)X
3 f
2371(.INCLUDES)X
1 f
2874(does)X
3063(for)X
3193(include)X
3481(\256les,)X
3678(except)X
3936(the)X
4072(\257ag)X
1350 5472(used)N
1535(is)X
3 f
9 f
1618(-)X
3 f
1666(L)X
1 f
1725(,)X
1771(as)X
1868(required)X
2185(by)X
2297(those)X
2507(linkers)X
2771(that)X
2928(allow)X
3148(you)X
3304(to)X
3397(tell)X
3535(them)X
3736(where)X
3974(to)X
4067(\256nd)X
1350 5576(libraries.)N
1683(The)X
1842(variable)X
2148(used)X
2331(is)X
3 f
2412(.LIBS)X
1 f
2635(.)X
748 5728(.MAIN)N
1350(If)X
1432(you)X
1588(didn't)X
1823(give)X
1999(a)X
2062(target)X
2287(\(or)X
2412(targets\))X
2699(to)X
2791(create)X
3024(when)X
3237(you)X
3392(invoked)X
3699(PMake,)X
3993(it)X
4066(will)X
1350 5832(take)N
1519(the)X
1649(sources)X
1934(of)X
2029(this)X
2179(target)X
2402(as)X
2497(the)X
2627(targets)X
2884(to)X
2975(create.)X
10 s
460 6224(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(10)X

11 p
%%Page: 11 11
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
748 784(.MAKEFLAGS)N
1350(This)X
1535(target)X
1764(provides)X
2095(a)X
2162(way)X
2336(for)X
2466(you)X
2626(to)X
2723(always)X
2995(specify)X
3277(\257ags)X
3471(for)X
3601(PMake)X
3878(when)X
4096(the)X
1350 888(make\256le)N
1683(is)X
1771(used.)X
1983(The)X
2149(\257ags)X
2344(are)X
2480(just)X
2637(as)X
2739(they)X
2920(would)X
3169(be)X
3281(typed)X
3506(to)X
3604(the)X
3741(shell,)X
3959(though)X
1350 992(the)N
3 f
9 f
1480(-)X
3 f
1528(f)X
1 f
1579(and)X
3 f
9 f
1728(-)X
3 f
1776(r)X
1 f
1837(\257ags)X
2025(have)X
2213(no)X
2323(effect.)X
748 1144(.NULL)N
1350(This)X
1542(allows)X
1807(you)X
1974(to)X
2077(specify)X
2365(what)X
2570(suf\256x)X
2 f
2804(PMake)X
1 f
3087(should)X
3356(pretend)X
3654(a)X
3727(\256le)X
3874(has)X
4025(if,)X
4135(in)X
1350 1248(fact,)N
1541(it)X
1628(has)X
1782(no)X
1907(known)X
2183(suf\256x.)X
2441(Only)X
2653(one)X
2816(suf\256x)X
3052(may)X
3240(be)X
3359(so)X
3473(designated.)X
3908(The)X
4081(last)X
1350 1352(source)N
1611(on)X
1731(the)X
1871(dependency)X
2323(line)X
2488(is)X
2579(the)X
2719(suf\256x)X
2951(that)X
3116(is)X
3207(used)X
3400(\(you)X
3592(should,)X
3880(however,)X
1350 1456(only)N
1529(give)X
1703(one)X
1852(suf\256x.)X
2089(.)X
2126(.\).)X
748 1608(.PATH)N
1350(If)X
1433(you)X
1590(give)X
1767(sources)X
2055(for)X
2182(this)X
2335(target,)X
2583(PMake)X
2857(will)X
3020(take)X
3192(them)X
3394(as)X
3491(directories)X
3887(to)X
3980(search)X
1350 1712(for)N
1487(\256les)X
1669(it)X
1754(cannot)X
2024(\256nd)X
2195(in)X
2298(the)X
2440(current)X
2723(directory.)X
3097(If)X
3189(you)X
3355(give)X
3541(no)X
3663(sources,)X
3982(it)X
4066(will)X
1350 1816(clear)N
1543(out)X
1678(any)X
1827(directories)X
2221(added)X
2453(to)X
2544(the)X
2674(search)X
2920(path)X
3094(before.)X
748 1968(.PATH)N
2 f
999(suf\256x)X
1 f
1350(This)X
1537(does)X
1728(a)X
1797(similar)X
2073(thing)X
2285(to)X
3 f
2384(.PATH)X
1 f
2651(,)X
2702(but)X
2844(it)X
2923(does)X
3113(it)X
3192(only)X
3378(for)X
3509(\256les)X
3685(with)X
3871(the)X
4008(given)X
1350 2072(suf\256x.)N
1594(The)X
1753(suf\256x)X
1975(must)X
2169(have)X
2357(been)X
2545(de\256ned)X
2826(already.)X
748 2224(.PRECIOUS)N
1350(Gives)X
1585(the)X
3 f
1722(.PRECIOUS)X
1 f
2227(attribute)X
2551(to)X
2649(each)X
2839(source)X
3097(on)X
3214(the)X
3351(dependency)X
3800(line,)X
3984(unless)X
1350 2328(there)N
1555(are)X
1690(no)X
1806(sources,)X
2119(in)X
2216(which)X
2459(case)X
2638(the)X
3 f
2774(.PRECIOUS)X
1 f
3278(attribute)X
3601(is)X
3688(given)X
3912(to)X
4009(every)X
1350 2432(target)N
1573(in)X
1664(the)X
1794(\256le.)X
748 2584(.RECURSIVE)N
1350(Applies)X
1649(the)X
3 f
1782(.MAKE)X
1 f
2103(attribute)X
2423(to)X
2517(all)X
2631(its)X
2740(sources.)X
3050(It)X
3129(does)X
3314(nothing)X
3608(if)X
3686(you)X
3842(don't)X
4052(give)X
1350 2688(it)N
1422(any)X
1571(sources.)X
748 2840(.SHELL)N
1350(Tells)X
2 f
1553(PMake)X
1 f
1828(to)X
1923(use)X
2065(some)X
2276(other)X
2482(shell)X
2674(than)X
2851(the)X
2984(Bourne)X
3268(Shell.)X
3519(The)X
3681(sources)X
3969(for)X
4096(the)X
1350 2944(target)N
1577(are)X
1710(organized)X
2083(as)X
2 f
2182(keyword)X
3 f
2480(=)X
2 f
2530(value)X
1 f
2747(strings.)X
3030(If)X
3114(a)X
2 f
3179(value)X
1 f
3396(contains)X
3716(whitespace,)X
4154(it)X
1350 3048(may)N
1540(be)X
1661(surrounded)X
2094(by)X
2220(double-quotes)X
2757(to)X
2863(make)X
3091(it)X
3178(a)X
3254(single)X
3502(word.)X
3741(The)X
3915(possible)X
1350 3152(sources)N
1635(are:)X
3 f
1350 3304(path=)N
2 f
1571(path)X
1 f
1638 3408(Tells)N
1850(where)X
2099(the)X
2242(shell)X
2444(actually)X
2759(resides.)X
3060(If)X
3153(you)X
3320(specify)X
3609(this)X
3772(and)X
3934(nothing)X
1638 3512(else,)N
1850(PMake)X
2152(will)X
2343(use)X
2513(the)X
2674(last)X
2850(component)X
3295(of)X
3421(the)X
3582(path)X
3786(to)X
3907(\256nd)X
4096(the)X
1638 3616(speci\256cation.)N
2136(Use)X
2302(this)X
2460(if)X
2544(you)X
2705(just)X
2862(want)X
3062(to)X
3160(use)X
3306(a)X
3374(different)X
3706(version)X
3994(of)X
4096(the)X
1638 3720(Bourne)N
1919(or)X
2014(C)X
2095(Shell)X
2299(\(PMake)X
2599(knows)X
2850(how)X
3023(to)X
3114(use)X
3253(the)X
3383(C)X
3464(Shell)X
3668(too\).)X
3 f
1350 3872(name=)N
2 f
1605(name)X
1 f
1638 3976(This)N
1824(is)X
1912(the)X
2049(name)X
2269(by)X
2386(which)X
2630(the)X
2767(shell)X
2963(is)X
3051(to)X
3149(be)X
3260(known.)X
3549(It)X
3631(is)X
3718(a)X
3785(single)X
4024(word)X
1638 4080(and,)N
1814(if)X
1895(no)X
2010(other)X
2217(keywords)X
2584(are)X
2717(speci\256ed)X
3056(\(other)X
3292(than)X
3 f
3470(path)X
1 f
3641(\),)X
3718(it)X
3794(is)X
3879(the)X
4013(name)X
1638 4184(by)N
1753(which)X
1995(PMake)X
2271(attempts)X
2598(to)X
2694(\256nd)X
2858(a)X
2924(speci\256cation)X
3397(for)X
3526(the)X
3661(it.)X
3760(You)X
3938(can)X
4087(use)X
1638 4288(this)N
1807(if)X
1902(you)X
2075(would)X
2336(just)X
2505(rather)X
2751(use)X
2909(the)X
3058(C)X
3158(Shell)X
3381(than)X
3574(the)X
3723(Bourne)X
4022(Shell)X
1638 4392(\(``)N
3 f
1725(.SHELL:)X
2093(name=csh)X
1 f
2470('')X
2550(will)X
2710(do)X
2820(it\).)X
3 f
1350 4544(quiet=)N
2 f
1591(echo-off)X
1902(command)X
1 f
1638 4648(The)N
1813(command)X
2 f
2199(PMake)X
1 f
2486(should)X
2758(send)X
2956(to)X
3062(stop)X
3246(the)X
3391(shell)X
3595(from)X
3803(printing)X
4120(its)X
1638 4752(commands.)N
2068(Once)X
2279(echoing)X
2584(is)X
2669(off,)X
2819(it)X
2895(is)X
2980(expected)X
3319(to)X
3413(remain)X
3683(off)X
3810(until)X
3998(expli-)X
1638 4856(citly)N
1818(turned)X
2065(on.)X
3 f
1350 5008(echo=)N
2 f
1571(echo-on)X
1876(command)X
1 f
1638 5112(The)N
1797(command)X
2167(PMake)X
2438(should)X
2695(give)X
2869(to)X
2960(turn)X
3124(echoing)X
3425(back)X
3613(on)X
3723(again.)X
3 f
1350 5264(\256lter=)N
2 f
(printed)S
1858(echo-off)X
2169(command)X
1 f
1638 5368(Many)N
1866(shells)X
2090(will)X
2251(echo)X
2440(the)X
2571(echo-off)X
2891(command)X
3262(when)X
3475(it)X
3548(is)X
3629(given.)X
3869(This)X
4048(key-)X
1638 5472(word)N
1851(tells)X
2031(PMake)X
2312(in)X
2413(what)X
2616(format)X
2883(the)X
3023(shell)X
3222(actually)X
3534(prints)X
3767(the)X
3907(echo-off)X
1638 5576(command.)N
2031(Whereever)X
2439(PMake)X
2710(sees)X
2878(this)X
3028(string)X
3251(in)X
3342(the)X
3472(shell's)X
3724(output,)X
3994(it)X
4066(will)X
1638 5680(delete)N
1887(it)X
1975(and)X
2140(any)X
2305(following)X
2686(whitespace,)X
3137(up)X
3263(to)X
3370(and)X
3535(including)X
3907(the)X
4052(next)X
1638 5784(newline.)N
10 s
460 6176(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(11)X

12 p
%%Page: 12 12
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
3 f
1350 784(echoFlag=)N
2 f
1738(\257ag)X
1892(to)X
1983(turn)X
2152(echoing)X
2453(on)X
1 f
1638 888(The)N
1800(\257ag)X
1957(to)X
2051(pass)X
2227(to)X
2321(the)X
2454(shell)X
2646(to)X
2740(turn)X
2907(echoing)X
3211(on)X
3324(at)X
3413(the)X
3546(start.)X
3767(If)X
3850(either)X
4076(this)X
1638 992(or)N
1737(the)X
1870(next)X
2047(\257ag)X
2204(begins)X
2459(with)X
2641(a)X
2705(`)X
9 f
2734(-)X
1 f
2782(',)X
2858(the)X
2991(\257ags)X
3182(will)X
3345(be)X
3453(passed)X
3712(to)X
3806(the)X
3939(shell)X
4131(as)X
1638 1096(separate)N
1948(arguments.)X
2359(Otherwise,)X
2764(the)X
2894(two)X
3048(will)X
3208(be)X
3313(concatenated.)X
3 f
1350 1248(errFlag=)N
2 f
1684(\257ag)X
1838(to)X
1929(turn)X
2098(error)X
2305(checking)X
2640(on)X
1 f
1638 1352(Flag)N
1817(to)X
1908(give)X
2082(the)X
2212(shell)X
2401(to)X
2492(turn)X
2656(error)X
2848(checking)X
3188(on)X
3298(at)X
3384(the)X
3514(start.)X
3 f
1350 1504(check=)N
2 f
1615(command)X
1978(to)X
2069(turn)X
2238(error)X
2445(checking)X
2780(on)X
1 f
1638 1608(The)N
1809(command)X
2191(to)X
2294(make)X
2519(the)X
2661(shell)X
2862(check)X
3101(for)X
3237(errors)X
3474(or)X
3580(to)X
3682(print)X
3882(the)X
4023(com-)X
1638 1712(mand)N
1864(that's)X
2090(about)X
2316(to)X
2415(be)X
2528(executed)X
2870(\(%s)X
3035(indicates)X
3378(where)X
3621(the)X
3758(command)X
4135(to)X
1638 1816(print)N
1827(should)X
2084(go\),)X
2245(if)X
2321(hasErrCtl)X
2681(is)X
2762("no".)X
3 f
1350 1968(ignore=)N
2 f
1640(command)X
2003(to)X
2094(turn)X
2263(error)X
2470(checking)X
2805(off)X
1 f
1638 2072(The)N
1807(command)X
2187(to)X
2287(turn)X
2460(error)X
2661(checking)X
3010(off)X
3143(or)X
3247(the)X
3386(command)X
3765(to)X
3865(execute)X
4165(a)X
1638 2176(command)N
2008(ignoring)X
2329(any)X
2478(errors.)X
2726("%s")X
2927(takes)X
3130(the)X
3260(place)X
3468(of)X
3563(the)X
3693(command.)X
3 f
1350 2328(hasErrCtl=)N
2 f
1781(yes)X
1915(or)X
2015(no)X
1 f
1638 2432(This)N
1825(takes)X
2036(a)X
2105(value)X
2326(that)X
2488(is)X
2576(either)X
3 f
2806(yes)X
1 f
2952(or)X
3 f
3054(no)X
1 f
3147(,)X
3198(telling)X
3454(how)X
3634(the)X
3771("check")X
4077(and)X
1638 2536("ignore")N
1973(commands)X
2393(should)X
2666(be)X
2787(used.)X
3030(NOTE:)X
3327(If)X
3422(this)X
3587(is)X
3683("no",)X
3902(both)X
4096(the)X
1638 2640(check)N
1870(and)X
2024(ignore)X
2276(commands)X
2685(should)X
2946(contain)X
3232(a)X
3297(\\n)X
3392(at)X
3482(their)X
3670(end)X
3823(if)X
3903(the)X
4037(shell)X
1638 2744(requires)N
1943(a)X
2004(newline)X
2305(before)X
2551(executing)X
2916(a)X
2977(command.)X
1350 2896(The)N
1526(strings)X
1800(that)X
1972(follow)X
2241(these)X
2461(keywords)X
2840(may)X
3030(be)X
3151(enclosed)X
3497(in)X
3604(single)X
3853(or)X
3964(double)X
1350 3000(quotes)N
1608(\(the)X
1773(quotes)X
2031(will)X
2197(be)X
2308(stripped)X
2620(off\))X
2779(and)X
2933(may)X
3112(contain)X
3399(the)X
3534(usual)X
3747(C)X
3833(backslash-)X
1350 3104(characters.)N
748 3256(.SILENT)N
1350(Applies)X
1652(the)X
3 f
1788(.SILENT)X
1 f
2160(attribute)X
2482(to)X
2578(each)X
2766(of)X
2866(its)X
2977(sources.)X
3289(If)X
3374(there)X
3577(are)X
3711(no)X
3826(sources)X
4116(on)X
1350 3360(the)N
1480(dependency)X
1922(line,)X
2099(then)X
2273(it)X
2345(is)X
2426(as)X
2521(if)X
2597(you)X
2751(gave)X
2939(PMake)X
3210(the)X
3 f
9 f
3340(-)X
3 f
3388(s)X
1 f
3444(\257ag.)X
748 3512(.SUFFIXES)N
1350(This)X
1544(is)X
1640(used)X
1838(to)X
1944(give)X
2132(new)X
2314(\256le)X
2463(suf\256xes)X
2772(for)X
2910(PMake)X
3195(to)X
3300(handle.)X
3593(Each)X
3805(source)X
4070(is)X
4165(a)X
1350 3616(suf\256x)N
1575(PMake)X
1849(should)X
2109(recognize.)X
2498(If)X
2580(you)X
2736(give)X
2912(a)X
3 f
2975(.SUFFIXES)X
1 f
3446(dependency)X
3890(line)X
4047(with)X
1350 3720(no)N
1465(sources,)X
1777(PMake)X
2052(will)X
2216(forget)X
2452(about)X
2674(all)X
2789(the)X
2923(suf\256xes)X
3222(it)X
3298(knew)X
3514(\(this)X
3697(also)X
3865(nukes)X
4096(the)X
1350 3824(null)N
1513(suf\256x\).)X
1811(For)X
1958(those)X
2169(targets)X
2429(that)X
2587(need)X
2777(to)X
2870(have)X
3060(suf\256xes)X
3357(de\256ned,)X
3662(this)X
3814(is)X
3897(how)X
4072(you)X
1350 3928(do)N
1460(it.)X
748 4080(In)N
843(addition)X
1155(to)X
1246(these)X
1449(targets,)X
1728(a)X
1789(line)X
1944(of)X
2039(the)X
2169(form)X
2 f
1036 4236(attribute)N
1 f
1363(:)X
2 f
1410(sources)X
1 f
748 4392(applies)N
1020(the)X
2 f
1150(attribute)X
1 f
1477(to)X
1568(all)X
1679(the)X
1809(targets)X
2066(listed)X
2280(as)X
2 f
2375(sources)X
1 f
2665(except)X
2917(as)X
3012(noted)X
3230(above.)X
3 f
10 s
460 4544(THE)N
661(POWER)X
996(OF)X
1140(SUFFIXES)X
1 f
11 s
748 4648(One)N
928(of)X
1035(the)X
1177(best)X
1353(aspects)X
1641(of)X
1748(both)X
2 f
1939(Make)X
1 f
2168(and)X
2 f
2329(PMake)X
1 f
2612(comes)X
2871(from)X
3076(their)X
3272(understanding)X
3805(of)X
3912(how)X
4096(the)X
748 4752(suf\256x)N
975(of)X
1075(a)X
1141(\256le)X
1281(pertains)X
1586(to)X
1681(its)X
1791(contents)X
2111(and)X
2264(their)X
2452(ability)X
2705(to)X
2800(do)X
2914(things)X
3156(with)X
3339(a)X
3404(\256le)X
3543(based)X
3769(solely)X
4006(on)X
4120(its)X
748 4856(suf\256x.)N
2 f
1021(PMake)X
1 f
1299(also)X
1469(has)X
1614(the)X
1750(ability)X
2005(to)X
2102(\256nd)X
2267(a)X
2334(\256le)X
2475(based)X
2703(on)X
2819(its)X
2931(suf\256x,)X
3181(supporting)X
3586(different)X
3917(types)X
4131(of)X
748 4960(\256les)N
932(being)X
1165(in)X
1270(different)X
1609(directories.)X
2061(The)X
2234(former)X
2509(ability)X
2772(derives)X
3062(from)X
3269(the)X
3413(existence)X
3777(of)X
3886(so-called)X
748 5064(transformation)N
1310(rules)X
1524(while)X
1762(the)X
1912(latter)X
2136(comes)X
2403(from)X
2616(the)X
2766(speci\256cation)X
3254(of)X
3369(search)X
3635(paths)X
3863(using)X
4096(the)X
3 f
748 5168(.PATH)N
1 f
1037(target.)X
3 f
10 s
748 5320(TRANSFORMATION)N
1559(RULES)X
1 f
11 s
748 5472(A)N
849(special)X
1132(type)X
1322(of)X
1433(dependency,)X
1913(called)X
2162(a)X
2239(transformation)X
2796(rule,)X
2993(consists)X
3310(of)X
3420(a)X
3496(target)X
3734(made)X
3962(of)X
4072(two)X
748 5576(known)N
1010(suf\256xes)X
1306(stuck)X
1515(together)X
1827(followed)X
2163(by)X
2274(a)X
2336(shell)X
2526(script)X
2745(to)X
2837(transform)X
3202(a)X
3264(\256le)X
3400(of)X
3496(one)X
3646(suf\256x)X
3869(into)X
4030(a)X
4091(\256le)X
748 5680(of)N
845(the)X
977(other.)X
1226(The)X
1387(\256rst)X
1548(suf\256x)X
1772(is)X
1855(the)X
1987(suf\256x)X
2211(of)X
2308(the)X
2440(source)X
2693(\256le)X
2830(and)X
2981(the)X
3113(second)X
3380(is)X
3462(that)X
3618(of)X
3714(the)X
3845(target)X
4069(\256le.)X
748 5784(E.g.)N
916(the)X
1050(target)X
1277(``.c.o,'')X
1568(followed)X
1907(by)X
2021(commands,)X
2451(would)X
2697(de\256ne)X
2938(a)X
3002(transformation)X
3546(from)X
3742(\256les)X
3914(with)X
4096(the)X
10 s
460 6176(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(12)X

13 p
%%Page: 13 13
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
748 784(``.c'')N
957(suf\256x)X
1189(to)X
1290(those)X
1508(with)X
1697(the)X
1837(``.o'')X
2050(suf\256x.)X
2325(A)X
2419(transformation)X
2969(rule)X
3137(has)X
3285(no)X
3404(source)X
3664(\256les)X
3842(associated)X
748 888(with)N
931(it,)X
1029(though)X
1300(attributes)X
1655(may)X
1833(be)X
1942(given)X
2164(to)X
2259(one)X
2412(in)X
2507(the)X
2641(usual)X
2853(way.)X
3046(These)X
3281(attributes)X
3635(are)X
3767(then)X
3944(applied)X
748 992(to)N
853(any)X
1016(target)X
1253(that)X
1422(is)X
1517(on)X
1641(the)X
1785(``target)X
2080(end'')X
2301(of)X
2410(a)X
2485(transformation)X
3040(rule.)X
3257(The)X
3430(suf\256xes)X
3738(that)X
3906(are)X
4048(con-)X
748 1096(catenated)N
1106(must)X
1303(be)X
1411(already)X
1695(known)X
1959(to)X
2 f
2053(PMake)X
1 f
2327(in)X
2421(order)X
2631(for)X
2758(their)X
2944(concatenation)X
3458(to)X
3551(be)X
3658(recognized)X
4068(as)X
4165(a)X
748 1200(transformation,)N
1317(i.e.)X
1453(the)X
1589(suf\256xes)X
1889(must)X
2088(have)X
2281(been)X
2474(the)X
2609(source)X
2865(for)X
2994(a)X
3060(.SUFFIXES)X
3514(target)X
3742(at)X
3833(some)X
4046(time)X
748 1304(before)N
1005(the)X
1146(transformation)X
1698(is)X
1790(de\256ned.)X
2126(Many)X
2364(transformations)X
2950(are)X
3090(de\256ned)X
3382(in)X
3483(the)X
3623(system)X
3900(make\256le)X
748 1408(\(qv.)N
3 f
935(FILES)X
1 f
1190(\))X
1245(and)X
1398(I)X
1453(refer)X
1643(you)X
1800(there)X
2001(for)X
2128(more)X
2334(examples)X
2692(as)X
2790(well)X
2967(as)X
3065(to)X
3159(\256nd)X
3321(what)X
3517(is)X
3601(already)X
3885(available)X
748 1512(\(you)N
937(should)X
1200(especially)X
1581(note)X
1761(the)X
1897(various)X
2184(variables)X
2530(used)X
2719(to)X
2816(contain)X
3104(\257ags)X
3298(for)X
3428(the)X
3563(compilers,)X
3960(assem-)X
748 1616(blers,)N
965(etc.,)X
1135(used)X
1319(to)X
1411(transform)X
1776(the)X
1907(\256les.)X
2099(These)X
2332(variables)X
2673(allow)X
2892(you)X
3047(to)X
3139(customize)X
3520(the)X
3651(transformations)X
748 1720(to)N
842(your)X
1028(own)X
1204(needs)X
1429(without)X
1724(having)X
1989(to)X
2083(rede\256ne)X
2391(them\).)X
2666(A)X
2753(transformation)X
3296(rule)X
3457(may)X
3633(be)X
3740(de\256ned)X
4023(more)X
748 1824(than)N
923(once,)X
1134(but)X
1270(only)X
1450(the)X
1581(last)X
1726(such)X
1909(de\256nition)X
2270(is)X
2351(remembered)X
2813(by)X
2 f
2923(PMake)X
1 f
3179(.)X
3245(This)X
3424(allows)X
3676(you)X
3830(to)X
3921(rede\256ne)X
748 1928(the)N
878(transformations)X
1453(in)X
1544(the)X
1674(system)X
1941(make\256le)X
2267(if)X
2343(you)X
2497(wish.)X
748 2080(Transformation)N
1328(rules)X
1531(are)X
1670(used)X
1863(only)X
2052(when)X
2274(a)X
2345(target)X
2578(has)X
2727(no)X
2847(commands)X
3261(associated)X
3655(with)X
3844(it,)X
3947(both)X
4135(to)X
748 2184(\256nd)N
908(any)X
1058(additional)X
1435(\256les)X
1605(on)X
1716(which)X
1954(it)X
2027(depends)X
2338(and)X
2488(to)X
2580(attempt)X
2869(to)X
2961(\256gure)X
3189(out)X
3325(just)X
3475(how)X
3648(to)X
3739(make)X
3952(the)X
4082(tar-)X
748 2288(get)N
879(should)X
1137(it)X
1209(end)X
1358(up)X
1468(being)X
1686(out-of-date.)X
2143(When)X
2375(a)X
2436(transformation)X
2977(is)X
3058(found)X
3285(for)X
3409(a)X
3470(target,)X
3715(another)X
4001(of)X
4096(the)X
748 2392(seven)N
970(``local'')X
1280(variables)X
1620(mentioned)X
2015(earlier)X
2262(is)X
2343(de\256ned:)X
1036 2544(.IMPSRC)N
1403(\(<\))X
1324 2648(The)N
1500(name/path)X
1907(of)X
2019(the)X
2165(source)X
2432(from)X
2641(which)X
2894(the)X
3040(target)X
3279(is)X
3376(to)X
3483(be)X
3604(transformed)X
4067(\(the)X
1324 2752(``implied'')N
1733(source\).)X
748 2904(For)N
892(example,)X
1235(given)X
1453(the)X
1583(following)X
1948(make\256le:)X
1036 3060(a.out)N
1232(:)X
1279(a.o)X
1406(b.o)X
1324 3164($\(CC\))N
1566($\(.ALLSRC\))X
748 3320(and)N
907(a)X
978(directory)X
1328(containing)X
1733(the)X
1873(\256les)X
2052(a.o,)X
2211(a.c)X
2342(and)X
2500(b.c,)X
2 f
2658(PMake)X
1 f
2938(will)X
3107(look)X
3295(at)X
3390(the)X
3529(list)X
3669(of)X
3773(suf\256xes)X
4077(and)X
748 3424(transformations)N
1339(given)X
1573(in)X
1680(the)X
1826(built-in)X
2125(rules)X
2334(and)X
2499(\256nd)X
2674(that)X
2845(the)X
2991(suf\256xes)X
3302(``.c'')X
3517(and)X
3682(``.o'')X
3902(are)X
4047(both)X
748 3528(known)N
1010(and)X
1160(there)X
1358(is)X
1439(a)X
1500(transformation)X
2041(rule)X
2200(de\256ned)X
2481(from)X
2674(one)X
2823(to)X
2914(the)X
3044(other)X
3247(with)X
3426(the)X
3556(command)X
3926(``$\(CC\))X
748 3632($\(CFLAGS\))N
1221(-c)X
1323($\(.IMPSRC\).'')X
1906(Having)X
2199(found)X
2438(this,)X
2622(it)X
2706(can)X
2862(then)X
3047(check)X
3285(the)X
3426(modi\256cation)X
3906(times)X
4131(of)X
748 3736(both)N
933(a.c)X
1061(and)X
1216(b.c)X
1349(and)X
1504(execute)X
1801(the)X
1937(command)X
2313(from)X
2512(the)X
2648(transformation)X
3195(rule)X
3359(as)X
3459(necessary)X
3827(in)X
3923(order)X
4135(to)X
748 3840(update)N
1005(the)X
1135(\256les)X
1304(a.o)X
1431(and)X
1580(b.o.)X
2 f
748 3992(PMake)N
1 f
1004(,)X
1050(unlike)X
2 f
1295(Make)X
1 f
1514(before)X
1762(it,)X
1858(has)X
1999(the)X
2131(ability)X
2381(to)X
2473(apply)X
2692(several)X
2964(transformations)X
3540(to)X
3632(a)X
3694(\256le)X
3830(even)X
4019(if)X
4096(the)X
748 4096(intermediate)N
1213(\256les)X
1383(do)X
1494(not)X
1630(exist.)X
1864(Given)X
2102(a)X
2164(directory)X
2505(containing)X
2901(a)X
2963(.o)X
3052(\256le)X
3188(and)X
3338(a)X
3400(.q)X
3488(\256le,)X
3645(and)X
3794(transforma-)X
748 4200(tions)N
945(from)X
1141(.q)X
1232(to)X
1326(.l,)X
1420(.l)X
1492(to)X
1586(.c)X
1672(and)X
1824(.c)X
1910(to)X
2003(.o,)X
2 f
2115(PMake)X
1 f
2388(will)X
2550(de\256ne)X
2789(a)X
2852(transformation)X
3395(from)X
3590(.q)X
9 f
3680 MX
(->)174 987 oc
1 f
3791(.o)X
3881(using)X
4096(the)X
748 4304(three)N
952(transformation)X
1499(rules)X
1697(you)X
1856(de\256ned.)X
2186(In)X
2286(the)X
2421(event)X
2639(of)X
2739(two)X
2898(paths)X
3111(between)X
3431(the)X
3566(same)X
3774(suf\256xes,)X
4096(the)X
748 4408(shortest)N
1049(path)X
1228(will)X
1393(be)X
1503(chosen)X
1774(between)X
2094(the)X
2229(target)X
2457(and)X
2611(the)X
2746(\256rst)X
2910(existing)X
3217(\256le)X
3357(on)X
3472(the)X
3607(path.)X
3829(So)X
3948(if)X
4028(there)X
748 4512(were)N
942(also)X
1108(a)X
1171(transformation)X
1714(from)X
1909(.l)X
1980(\256les)X
2151(to)X
2244(.o)X
2333(\256les,)X
2 f
2525(PMake)X
1 f
2797(would)X
3040(use)X
3180(the)X
3311(path)X
3486(.q)X
9 f
3575 MX
(->)174 987 oc
1 f
3685(.l)X
9 f
3755 MX
(->)174 987 oc
1 f
3865(.o)X
3954(instead)X
748 4616(of)N
843(.q)X
9 f
931 MX
(->)174 987 oc
1 f
1040(.l)X
9 f
1109 MX
(->)174 987 oc
1 f
1218(.c)X
9 f
1301 MX
(->)174 987 oc
1 f
1410(.o.)X
748 4768(Once)N
959(an)X
1068(existing)X
1374(\256le)X
1513(is)X
1598(found,)X
2 f
1851(PMake)X
1 f
2126(will)X
2290(continue)X
2620(to)X
2715(look)X
2898(at)X
2987(and)X
3139(record)X
3388(transformations)X
3966(until)X
4154(it)X
748 4872(comes)N
998(to)X
1092(a)X
1156(\256le)X
1294(to)X
1388(which)X
1628(nothing)X
1923(it)X
1998(knows)X
2252(of)X
2350(can)X
2497(be)X
2605(transformed,)X
3077(at)X
3166(which)X
3405(point)X
3611(it)X
3685(will)X
3847(stop)X
4018(look-)X
748 4976(ing)N
883(and)X
1032(use)X
1171(the)X
1301(path)X
1475(it)X
1547(has)X
1686(already)X
1967(found.)X
748 5128(What)N
964(happens)X
1277(if)X
1356(you)X
1513(have)X
1704(a)X
1768(.o)X
1859(\256le,)X
2019(a)X
2083(.q)X
2174(\256le)X
2312(and)X
2464(a)X
2527(.r)X
2602(\256le,)X
2761(all)X
2874(with)X
3055(the)X
3187(same)X
3392(pre\256x,)X
3643(and)X
3794(transforma-)X
748 5232(tions)N
951(from)X
1153(.q)X
9 f
1250 MX
(->)174 987 oc
1 f
1367(.o)X
1463(and)X
1620(.r)X
9 f
1701 MX
(->)174 987 oc
1 f
1818(.o?)X
1975(Which)X
2240(transformation)X
2789(will)X
2957(be)X
3070(used?)X
2 f
3322(PMake)X
1 f
3601(uses)X
3782(the)X
3920(order)X
4135(in)X
748 5336(which)N
987(the)X
1119(suf\256xes)X
1416(were)X
1610(given)X
1830(on)X
1941(the)X
3 f
2072(.SUFFIXES)X
1 f
2542(line)X
2698(to)X
2790(decide)X
3043(between)X
3359(transformations:)X
3960(which-)X
748 5440(ever)N
921(suf\256x)X
1143(came)X
1351(\256rst,)X
1532(wins.)X
1764(So)X
1879(if)X
1955(the)X
2085(three)X
2283(suf\256xes)X
2578(were)X
2770(declared)X
1036 5596(.SUFFIXES)N
1485(:)X
1532(.o)X
1620(.q)X
1708(.r)X
748 5752(the)N
878(.q)X
9 f
966 MX
(->)174 987 oc
1 f
1075(.o)X
1163(transformation)X
1704(would)X
1946(be)X
2051(applied.)X
2355(Similarly,)X
2729(if)X
2805(they)X
2979(were)X
3171(declared)X
3491(as)X
10 s
460 6152(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(13)X

14 p
%%Page: 14 14
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
1036 784(.SUFFIXES)N
1485(:)X
1532(.o)X
1620(.r)X
1693(.q)X
748 940(the)N
879(.r)X
9 f
953 MX
(->)174 987 oc
1 f
1063(.o)X
1152(transformation)X
1694(would)X
1937(be)X
2043(used.)X
2271(You)X
2445(should)X
2703(keep)X
2892(this)X
3043(in)X
3135(mind)X
3339(when)X
3551(writing)X
3828(such)X
4011(rules.)X
748 1044(Note)N
946(also)X
1115(that)X
1275(because)X
1580(the)X
1715(placing)X
2002(of)X
2102(a)X
2168(suf\256x)X
2395(on)X
2510(a)X
3 f
2575(.SUFFIXES)X
1 f
3048(line)X
3207(doesn't)X
3492(alter)X
3675(the)X
3809(precedence)X
748 1148(of)N
853(previously-de\256ned)X
1545(transformations,)X
2152(it)X
2234(is)X
2325(sometimes)X
2735(necessary)X
3108(to)X
3209(clear)X
3412(the)X
3552(whole)X
3798(lot)X
3923(of)X
4027(them)X
748 1252(out)N
883(and)X
1032(start)X
1206(from)X
1399(scratch.)X
1692(This)X
1871(is)X
1952(what)X
2145(the)X
3 f
2275(.SUFFIXES)X
1 f
2722(-only)X
2930(line,)X
3107(mentioned)X
3502(earlier,)X
3771(will)X
3931(do.)X
3 f
10 s
460 1404(SEARCH)N
826(PATHS)X
2 f
11 s
748 1508(PMake)N
1 f
1033(also)X
1210(supports)X
1543(the)X
1686(notion)X
1947(of)X
2055(multiple)X
2386(directories)X
2793(in)X
2897(a)X
2971(more)X
3187(\257exible,)X
3509(easily-used)X
3940(manner)X
748 1612(than)N
924(has)X
1065(been)X
1255(available)X
1598(in)X
1691(the)X
1823(past.)X
2033(You)X
2208(can)X
2354(de\256ne)X
2593(a)X
2656(list)X
2789(of)X
2886(directories)X
3282(in)X
3375(which)X
3613(to)X
3705(search)X
3952(for)X
4077(any)X
748 1716(and)N
901(all)X
1016(\256les)X
1189(that)X
1348(aren't)X
1579(in)X
1674(the)X
1808(current)X
2083(directory)X
2427(by)X
2541(giving)X
2793(the)X
2927(directories)X
3324(as)X
3422(sources)X
3710(to)X
3804(the)X
3 f
3937(.PATH)X
1 f
748 1820(target.)N
998(The)X
1162(search)X
1413(will)X
1578(only)X
1762(be)X
1872(conducted)X
2261(for)X
2389(those)X
2601(\256les)X
2774(used)X
2961(only)X
3144(as)X
3243(sources,)X
3554(on)X
3668(the)X
3802(assumption)X
748 1924(that)N
903(\256les)X
1072(used)X
1255(as)X
1350(targets)X
1607(will)X
1767(be)X
1872(created)X
2148(in)X
2239(the)X
2369(current)X
2640(directory.)X
748 2076(The)N
907(line)X
1036 2232(.PATH)N
1309(:)X
1356(RCS)X
748 2388(would)N
991(tell)X
2 f
1128(PMake)X
1 f
1400(to)X
1492(look)X
1672(for)X
1797(any)X
1946(\256les)X
2115(it)X
2187(is)X
2268(seeking)X
2559(\(including)X
2944(ones)X
3127(made)X
3340(up)X
3450(by)X
3560(means)X
3807(of)X
3902(transfor-)X
748 2492(mation)N
1021(rules\))X
1248(in)X
1344(the)X
1479(RCS)X
1673(directory)X
2018(as)X
2118(well)X
2297(as)X
2397(the)X
2532(current)X
2808(one.)X
2984(Note,)X
3204(however,)X
3554(that)X
3713(this)X
3867(searching)X
748 2596(is)N
831(only)X
1012(done)X
1207(if)X
1285(the)X
1417(\256le)X
1554(is)X
1637(used)X
1822(only)X
2003(as)X
2100(a)X
2163(source)X
2416(in)X
2509(the)X
2641(make\256le.)X
2991(I.e.)X
3127(if)X
3205(the)X
3337(\256le)X
3474(cannot)X
3733(be)X
3839(created)X
4116(by)X
748 2700(commands)N
1152(in)X
1243(the)X
1373(make\256le.)X
748 2852(A)N
833(search)X
1079(path)X
1253(speci\256c)X
1544(to)X
1635(\256les)X
1804(with)X
1983(a)X
2044(given)X
2262(suf\256x)X
2484(can)X
2628(also)X
2792(be)X
2897(speci\256ed)X
3232(in)X
3323(much)X
3541(the)X
3671(same)X
3874(way.)X
1036 3008(.PATH.h)N
1375(:)X
1422(h)X
1488(/usr/include)X
748 3164(causes)N
1000(the)X
1131(search)X
1378(for)X
1503(header)X
1760(\256les)X
1930(to)X
2022(be)X
2128(conducted)X
2513(in)X
2605(the)X
2736(h)X
2803(and)X
2953(/usr/include)X
3392(directory)X
3732(as)X
3827(well)X
4001(as)X
4096(the)X
748 3268(current)N
1019(one.)X
748 3420(When)N
983(expanding)X
1375(wildcards,)X
1764(these)X
1970(paths)X
2181(are)X
2313(also)X
2480(used.)X
2688(If)X
2771(the)X
2903(pattern)X
3172(has)X
3313(a)X
3376(recognizable)X
3850(suf\256x,)X
4096(the)X
748 3524(search)N
1000(path)X
1180(for)X
1310(that)X
1471(suf\256x)X
1699(is)X
1786(used.)X
2018(Otherwise,)X
2428(the)X
2563(path)X
2742(de\256ned)X
3028(with)X
3212(the)X
3347(regular)X
3 f
3623(.PATH)X
1 f
3917(target)X
4145(is)X
748 3628(used.)N
748 3780(When)N
986(a)X
1053(\256le)X
1194(is)X
1281(found)X
1514(somewhere)X
1942(other)X
2151(than)X
2331(the)X
2467(current)X
2744(directory,)X
3112(its)X
3224(name)X
3443(is)X
3530(replaced)X
3855(by)X
3970(its)X
4081(full)X
748 3884(pathname)N
1113(in)X
1204(any)X
1353(``local'')X
1663(variables.)X
748 4036(Two)N
933(types)X
1143(of)X
1240(suf\256xes)X
1537(are)X
1668(given)X
1888(special)X
2157(attention)X
2491(when)X
2705(a)X
2768(search)X
3016(path)X
3191(is)X
3273(de\256ned)X
3555(for)X
3680(them.)X
3902(On)X
4032(most)X
748 4140(systems,)N
1075(the)X
1209(C)X
1294(compiler)X
1634(lets)X
1783(you)X
1941(specify)X
2221(where)X
2461(to)X
2556(\256nd)X
2718(header)X
2977(\256les)X
3149(\(.h)X
3269(\256les\))X
3470(by)X
3583(means)X
3833(of)X
3 f
9 f
3931(-)X
3 f
3979(I)X
1 f
4038(\257ags)X
748 4244(similar)N
1021(to)X
1117(those)X
1330(used)X
1518(by)X
2 f
1633(PMake)X
1 f
1889(.)X
1960(If)X
2045(a)X
2111(search)X
2362(path)X
2541(is)X
2627(given)X
2850(for)X
2979(any)X
3133(suf\256x)X
3360(used)X
3548(as)X
3648(a)X
3713(source)X
3968(for)X
4096(the)X
3 f
748 4348(.INCLUDES)N
1 f
1250(target,)X
1500(the)X
1635(variable)X
3 f
1946($\(.INCLUDES\))X
1 f
2550(will)X
2715(be)X
2825(set)X
2950(to)X
3046(contain)X
3333(all)X
3449(the)X
3584(directories)X
3982(on)X
4096(the)X
748 4452(path,)N
945(in)X
1037(the)X
1168(order)X
1376(given,)X
1617(in)X
1708(a)X
1769(format)X
2026(which)X
2263(can)X
2407(be)X
2512(passed)X
2768(directly)X
3060(to)X
3151(the)X
3281(C)X
3362(compiler.)X
3742(Similarly,)X
4116(on)X
748 4556(some)N
966(systems,)X
1299(one)X
1457(may)X
1640(give)X
1823(directories)X
2226(to)X
2326(search)X
2581(for)X
2714(libraries)X
3034(to)X
3134(the)X
3273(compiler)X
3618(by)X
3737(means)X
3993(of)X
3 f
9 f
4097(-)X
3 f
4145(L)X
1 f
748 4660(\257ags.)N
982(Directories)X
1397(on)X
1509(the)X
1641(search)X
1889(path)X
2065(for)X
2191(a)X
2254(suf\256x)X
2478(which)X
2717(was)X
2877(the)X
3009(source)X
3262(of)X
3359(the)X
3 f
3490(.LIBS)X
1 f
3736(target)X
3960(will)X
4121(be)X
748 4764(placed)N
1000(in)X
1091(the)X
3 f
1221($\(.LIBS\))X
1 f
1568(variable)X
1874(ready)X
2091(to)X
2182(be)X
2287(passed)X
2543(to)X
2634(the)X
2764(compiler.)X
3 f
10 s
460 4916(LIBRARIES)N
932(AND)X
1139(ARCHIVES)X
1 f
11 s
748 5020(Two)N
939(other)X
1150(special)X
1425(forms)X
1660(of)X
1762(sources)X
2054(are)X
2190(recognized)X
2605(by)X
2 f
2722(PMake)X
1 f
2978(.)X
3051(Any)X
3231(source)X
3489(that)X
3651(begins)X
3910(with)X
4096(the)X
748 5124(characters)N
1129(``-l'')X
1324(or)X
1422(ends)X
1608(in)X
1701(a)X
1764(suf\256x)X
1988(that)X
2145(is)X
2228(a)X
2291(source)X
2544(for)X
2670(the)X
3 f
2802(.LIBS)X
1 f
3049(target)X
3274(is)X
3357(assumed)X
3684(to)X
3777(be)X
3884(a)X
3947(library,)X
748 5228(and)N
899(any)X
1050(source)X
1303(that)X
1460(contains)X
1778(a)X
1841(left)X
1983(parenthesis)X
2403(in)X
2496(it)X
2570(is)X
2653(considered)X
3058(to)X
3151(be)X
3257(a)X
3319(member)X
3631(\(or)X
3756(members\))X
4131(of)X
748 5332(an)N
853(archive.)X
748 5484(Libraries)N
1098(are)X
1237(treated)X
1509(specially)X
1855(mostly)X
2127(in)X
2227(how)X
2409(they)X
2592(appear)X
2857(in)X
2957(the)X
3096(local)X
3299(variables)X
3648(of)X
3752(those)X
3969(targets)X
748 5588(that)N
904(depend)X
1181(on)X
1292(them.)X
1514(If)X
1595(the)X
1726(system)X
1994(supports)X
2315(the)X
3 f
9 f
2446(-)X
3 f
2494(L)X
1 f
2576(\257ag)X
2731(when)X
2944(linking,)X
3240(the)X
3371(name)X
3585(of)X
3680(the)X
3810(library)X
4067(\(i.e.)X
748 5692(its)N
858(``-l'')X
1054(form\))X
1280(is)X
1365(used)X
1552(in)X
1647(all)X
1762(local)X
1960(variables.)X
2 f
2348(PMake)X
1 f
2623(assumes)X
2942(that)X
3101(you)X
3259(will)X
3423(use)X
3565(the)X
3698($\(.LIBS\))X
4038(vari-)X
748 5796(able)N
930(in)X
1034(the)X
1177(appropriate)X
1613(place.)X
1878(If,)X
1993(however,)X
2352(the)X
2495(system)X
2774(does)X
2969(not)X
3116(have)X
3316(this)X
3478(feature,)X
3778(the)X
3920(name)X
4145(is)X
10 s
460 6188(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(14)X

15 p
%%Page: 15 15
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
748 784(expanded)N
1107(to)X
1198(its)X
1304(full)X
1449(pathname)X
1814(before)X
2060(it)X
2132(is)X
2213(placed)X
2465(in)X
2556(any)X
2705(local)X
2899(variable.)X
748 936(One)N
922(problem)X
1244(with)X
1429(libraries)X
1746(is)X
1833(they)X
2013(have)X
2207(a)X
2273(table)X
2472(of)X
2572(contents)X
2893(in)X
2989(them)X
3193(and)X
3347(when)X
3564(the)X
3699(\256le)X
3839(is)X
3925(touched)X
748 1040(\(so)N
893(the)X
1039(\256le's)X
1253(modi\256cation)X
1738(time)X
1934(and)X
2099(the)X
2244(time)X
2439(listed)X
2668(in)X
2774(the)X
2919(table)X
3128(of)X
3238(contents)X
3569(don't)X
3792(match\),)X
4096(the)X
748 1144(library)N
1012(is)X
1100(declared)X
1427(to)X
1525(be)X
1637(``out-of-date'')X
2173(by)X
2290(the)X
2427(linker)X
2662(and)X
2818(the)X
2955(\256nal)X
3141(linking)X
3421(stage)X
3630(of)X
3731(creating)X
4043(your)X
748 1248(program)N
1079(fails)X
1264(miserably.)X
1667(To)X
1798(avoid)X
2027(this)X
2188(problem,)X
2537(when)X
2760(you)X
2925(use)X
3075(the)X
3 f
9 f
3216(-)X
3 f
3264(t)X
1 f
3326(\257ag,)X
2 f
3513(PMake)X
1 f
3795(updates)X
4096(the)X
748 1352(time)N
928(of)X
1023(the)X
1153(table)X
1347(of)X
1442(contents)X
1758(for)X
1882(the)X
2012(library,)X
2291(as)X
2386(well)X
2560(as)X
2655(the)X
2785(library)X
3042(itself.)X
748 1504(The)N
910(process)X
1198(of)X
1296(creating)X
1605(a)X
1669(library)X
1929(or)X
2027(archive)X
2311(can)X
2458(be)X
2566(a)X
2630(painful)X
2905(one,)X
3079(what)X
3275(with)X
3456(all)X
3569(the)X
3701(members)X
4048(hav-)X
748 1608(ing)N
887(to)X
982(be)X
1091(kept)X
1269(outside)X
1550(the)X
1684(archive)X
1968(as)X
2066(well)X
2243(as)X
2341(inside)X
2577(it)X
2652(in)X
2746(order)X
2956(to)X
3050(keep)X
3241(them)X
3443(from)X
3639(being)X
3860(recreated.)X
2 f
748 1712(PMake)N
1 f
1026(has)X
1171(been)X
1365(set)X
1491(up,)X
1629(however,)X
1981(to)X
2078(allow)X
2302(you)X
2462(to)X
2559(reference)X
2913(\256les)X
3088(that)X
3249(are)X
3384(in)X
3481(an)X
3592(archive)X
3879(in)X
3976(a)X
4043(rela-)X
748 1816(tively)N
972(painless)X
1278(manner.)X
1608(The)X
1767(speci\256cation)X
2235(of)X
2330(an)X
2435(archive)X
2716(member)X
3027(is)X
3108(written)X
3380(as:)X
2 f
1036 1972(archive)N
1 f
1300(\()X
2 f
1329(member)X
1 f
1633([)X
2 f
1662(member)X
1 f
1944(...]\))X
748 2128(Both)N
959(the)X
1105(open)X
1314(and)X
1479(close)X
1698(parenthesis)X
2132(are)X
2277(required)X
2608(and)X
2773(there)X
2987(may)X
3177(be)X
3298(any)X
3463(number)X
3770(of)X
3881(members)X
748 2232(between)N
1069(them)X
1274(\(except)X
1560(0,)X
1653(that)X
1813(is\).)X
1950(Members)X
2309(may)X
2488(also)X
2657(include)X
2944(wildcards)X
3313(characters.)X
3740(When)X
3977(such)X
4165(a)X
748 2336(source)N
1004(is)X
1090(examined,)X
1482(it)X
1559(is)X
1645(the)X
1780(modi\256cation)X
2254(time)X
2439(of)X
2539(the)X
2674(member,)X
3012(as)X
3112(recorded)X
3446(in)X
3542(the)X
3677(archive,)X
3985(that)X
4145(is)X
748 2440(used)N
931(to)X
1022(determine)X
1397(its)X
1503(datedness.)X
748 2592(If)N
830(an)X
937(archive)X
1220(member)X
1533(has)X
1674(no)X
1786(commands)X
2192(associated)X
2578(with)X
2759(it,)X
2 f
2855(PMake)X
1 f
3128(goes)X
3313(through)X
3611(a)X
3673(special)X
3941(process)X
748 2696(to)N
849(\256nd)X
1017(commands)X
1430(for)X
1563(it.)X
1688(First,)X
1903 0.4732(implicit)AX
2211(sources)X
2505(are)X
2643(sought)X
2909(using)X
3131(the)X
3270(``member'')X
3706(portion)X
3992(of)X
4096(the)X
748 2800(speci\256cation.)N
1258(So)X
1392(if)X
1487(you)X
1660(have)X
1867(something)X
2276(like)X
2450(``libcompat.a\(procFork.o\)'')X
3468(for)X
3611(a)X
3691(target,)X
2 f
3955(PMake)X
1 f
748 2904(attempts)N
1077(to)X
1174(\256nd)X
1339(sources)X
1630(for)X
1760(the)X
1896(\256le)X
2037(``procFork.o,'')X
2591(even)X
2785(if)X
2867(it)X
2945(doesn't)X
3232(exist.)X
3449(If)X
3535(such)X
3724(sources)X
4015(exist,)X
2 f
748 3008(PMake)N
1 f
1020(then)X
1195(looks)X
1409(for)X
1534(a)X
1596(transformation)X
2138(rule)X
2298(from)X
2492(the)X
2622(member's)X
2996(suf\256x)X
3218(to)X
3309(the)X
3439(archive's)X
3783(\(in)X
3903(this)X
4053(case)X
748 3112(from)N
941(.o)X
9 f
1029 MX
(->)174 987 oc
1 f
1138(.a\))X
1250(and)X
1399(tacks)X
1602(those)X
1810(commands)X
2214(on)X
2324(as)X
2419(well.)X
748 3264(To)N
868(make)X
1081(these)X
1284(transformations)X
1859(easier)X
2086(to)X
2177(write,)X
2402(three)X
2600(local)X
2794(variables)X
3134(are)X
3263(de\256ned)X
3544(for)X
3668(the)X
3798(target:)X
748 3416(.ARCHIVE)N
1182(\(%\))X
1036 3520(The)N
1195(path)X
1369(to)X
1460(the)X
1590(archive)X
1871(\256le.)X
748 3672(.MEMBER)N
1174(\(!\))X
1036 3776(The)N
1195(actual)X
1428(member)X
1739(name)X
1952(\(literally)X
2279(the)X
2409(part)X
2568(in)X
2659(parentheses\).)X
748 3928(.TARGET)N
1139(\(@\))X
1036 4032(The)N
1209(path)X
1397(to)X
1502(the)X
1646(\256le)X
1795(which)X
2046(will)X
2220(be)X
2339(archived,)X
2700(if)X
2790(it)X
2875(is)X
2969(only)X
3161(a)X
3235(source,)X
3521(or)X
3629(the)X
3772(same)X
3988(as)X
4096(the)X
3 f
1036 4136(.MEMBER)N
1 f
1486(variable)X
1792(if)X
1868(it)X
1940(is)X
2021(also)X
2185(a)X
2246(target.)X
748 4288(Using)N
994(the)X
1138(transformations)X
1727(already)X
2022(in)X
2127(the)X
2270(system)X
2550(make\256le,)X
2911(a)X
2985(make\256le)X
3324(for)X
3461(a)X
3535(library)X
3805(might)X
4047(look)X
748 4392(something)N
1138(like)X
1293(this:)X
1036 4548(OBJS)N
1263(=)X
1335(procFork.o)X
1745(procExec.o)X
2165(procEnviron.o)X
2693(fsRead.o)X
1036 4652(.o.a)N
1185(:)X
1324 4756(...)N
1324 4860(rm)N
1444(-f)X
1524($\(.MEMBER\))X
1036 5068(lib.a)N
1213(:)X
1260(lib.a\($\(OBJS\)\))X
1324 5172(ar)N
1414(cru)X
1548($\(.TARGET\))X
2041($\(.OODATE\))X
1324 5276(ranlib)N
1552($\(.TARGET\))X
748 5480(You)N
921(might)X
1150(be)X
1255(wondering,)X
1675(at)X
1761(this)X
1911(point,)X
2137(why)X
2310(I)X
2361(did)X
2496(not)X
2631(de\256ne)X
2868(the)X
2998(.o)X
9 f
3086 MX
(->)174 987 oc
1 f
3195(.a)X
3278(transformation)X
3819(like)X
3974(this:)X
1036 5636(.o.a)N
1185(:)X
1324 5740(ar)N
1414(r)X
1465($\(.ARCHIVE\))X
2001($\(.TARGET\))X
1324 5844(...)N
10 s
460 6236(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(15)X

16 p
%%Page: 16 16
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
1324 784(rm)N
1444(-f)X
1524($\(.TARGET\))X
748 940(The)N
911(reason)X
1166(is)X
1251(simple:)X
1538(you)X
1696(cannot)X
1957(execute)X
2252(``ar'')X
2461(on)X
2574(the)X
2707(same)X
2913(\256le)X
3051(several)X
3325(times)X
3542(at)X
3631(once.)X
3844(If)X
3927(you)X
4084(try,)X
748 1044(you)N
904(end)X
1055(up)X
1167(with)X
1348(a)X
1411(corrupted)X
1772(archive.)X
2099(So)X
2216(rather)X
2445(than)X
2621(reduce)X
2 f
2879(PMake)X
1 f
3152(to)X
3245(executing)X
3612(only)X
3792(one)X
3942(job)X
4078(at)X
4165(a)X
748 1148(time,)N
950(I)X
1001(chose)X
1223(to)X
1314(archive)X
1595(all)X
1706(the)X
1836(out-of-date)X
2249(\256les)X
2418(at)X
2504(once)X
2692(\(this)X
2871(turns)X
3069(out)X
3204(to)X
3295(be)X
3400(faster)X
3617(anyway\).)X
3 f
10 s
460 1300(OUTPUT)N
1 f
11 s
748 1404(When)N
988(creating)X
1302(targets)X
1567(in)X
1666(parallel,)X
1983(several)X
2261(shells)X
2491(are)X
2627(executing)X
2999(at)X
3092(once,)X
3309(each)X
3499(wanting)X
3812(to)X
3910(write)X
4120(its)X
748 1508(own)N
928(two)X
1088(cent's)X
1326(worth)X
1559(onto)X
1744(the)X
1880(screen.)X
2176(This)X
2361(output)X
2615(must)X
2815(be)X
2926(captured)X
3257(by)X
2 f
3373(PMake)X
1 f
3650(in)X
3747(some)X
3961(way)X
4135(in)X
748 1612(order)N
957(to)X
1050(prevent)X
1338(the)X
1470(screen)X
1718(from)X
1913(being)X
2133(\256lled)X
2339(with)X
2520(garbage)X
2822(even)X
3011(more)X
3215(indecipherable)X
3757(than)X
3932(one)X
4082(can)X
748 1716(already)N
1035(get)X
1171(from)X
1370(these)X
1579(programs.)X
2 f
1983(PMake)X
1 f
2260(has)X
2405(two)X
2564(ways)X
2771(of)X
2871(doing)X
3099(this,)X
3276(one)X
3430(of)X
3530(which)X
3772(provides)X
4102(for)X
748 1820(much)N
979(cleaner)X
1268(output)X
1529(and)X
1691(a)X
1765(clear)X
1971(delineation)X
2398(between)X
2725(the)X
2867(output)X
3127(of)X
3234(different)X
3571(jobs,)X
3774(the)X
3916(other)X
4131(of)X
748 1924(which)N
989(provides)X
1318(a)X
1383(more)X
1589(immediate)X
1988(response)X
2320(so)X
2423(one)X
2575(can)X
2722(tell)X
2861(what)X
3057(is)X
3141(really)X
3367(happening.)X
3803(The)X
3965(former)X
748 2028(is)N
834(done)X
1032(by)X
1147(notifying)X
1497(the)X
1631(user)X
1803(when)X
2019(the)X
2153(creation)X
2463(of)X
2562(a)X
2627(given)X
2849(target)X
3076(starts,)X
3310(capturing)X
3669(the)X
3803(output,)X
4077(and)X
748 2132(transferring)N
1182(it)X
1256(to)X
1349(the)X
1481(screen)X
1729(when)X
1942(the)X
2073(job)X
2209(\256nishes,)X
2523(preceeded)X
2902(by)X
3013(an)X
3119(indication)X
3496(as)X
3592(to)X
3684(which)X
3922(job)X
4058(pro-)X
748 2236(duced)N
992(the)X
1134(output.)X
1438(The)X
1609(latter)X
1824(is)X
1916(done)X
2120(by)X
2241(catching)X
2573(the)X
2714(output)X
2973(of)X
3079(the)X
3220(shell)X
3420(\(and)X
3609(its)X
3726(children\))X
4077(and)X
748 2340(buffering)N
1105(it)X
1185(until)X
1378(an)X
1491(entire)X
1722(line)X
1884(is)X
1972(received,)X
2321(then)X
2502(printing)X
2811(that)X
2973(line)X
3135(preceeded)X
3520(by)X
3637(the)X
3774(name)X
3994(of)X
4096(the)X
748 2444(job)N
886(from)X
1082(which)X
1322(the)X
1455(line)X
1612(came.)X
1866(The)X
2027(name)X
2242(of)X
2339(the)X
2471(job)X
2608(is)X
2691(just)X
2843(the)X
2975(target)X
3200(which)X
3439(is)X
3522(being)X
3742(created)X
4020(by)X
4132(it.)X
748 2548(Since)N
971(I)X
1027(prefer)X
1263(this)X
1418(second)X
1689(method,)X
2003(it)X
2080(is)X
2166(the)X
2301(one)X
2455(used)X
2643(by)X
2758(default.)X
3073(The)X
3236(\256rst)X
3399(method)X
3690(will)X
3854(be)X
3963(used)X
4150(if)X
748 2652(the)N
3 f
9 f
878(-)X
3 f
926(P)X
1 f
1002(\257ag)X
1156(is)X
1237(given)X
1455(to)X
2 f
1546(PMake)X
1 f
1802(.)X
3 f
10 s
460 2804(PARALLELISM)N
1 f
11 s
748 2908(As)N
872(mentioned)X
1272(before,)X
2 f
1545(PMake)X
1 f
1821(attempts)X
2147(to)X
2242(create)X
2478(several)X
2753(targets)X
3014(at)X
3104(once.)X
3340(On)X
3473(some)X
3685(systems)X
3990(where)X
748 3012(load)N
932(balancing)X
1307(or)X
1412(process)X
1707(migration)X
2083(is)X
2173(in)X
2273(effect,)X
2526(the)X
2665(amount)X
2961(of)X
3065(concurrency)X
3530(which)X
3776(can)X
3929(be)X
4043(used)X
748 3116(will)N
915(be)X
1027(much)X
1252(greater)X
1525(than)X
1706(on)X
1823(others.)X
2089(During)X
2367(the)X
2504(development)X
2989(of)X
2 f
3091(PMake)X
1 f
3347(,)X
3398(I)X
3456(found)X
3690(that)X
3852(while)X
4077(one)X
748 3220(could)N
967(create)X
1200(up)X
1311(to)X
1402(\256ve)X
1556(targets)X
1813(at)X
1899(once)X
2087(on)X
2197(a)X
2258(Sun)X
2417(3)X
2483(without)X
2775(making)X
3062(the)X
3192(machine)X
3513(unuseable,)X
3909(attempt-)X
748 3324(ing)N
885(the)X
1017(same)X
1222(feat)X
1378(on)X
1490(a)X
1553(Sun)X
1714(2)X
1782(would)X
2026(grind)X
2236(the)X
2368(machine)X
2690(into)X
2851(the)X
2982(dirt,)X
3150(most)X
3345(likely)X
3570(making)X
3858(the)X
3989(whole)X
748 3428(process)N
1034(run)X
1174(slower)X
1431(than)X
1606(it)X
1679(would)X
1922(have)X
2111(under)X
2 f
2334(Make)X
1 f
2536(.)X
2603(In)X
2699(addition,)X
3034(the)X
3165(use)X
3304(of)X
2 f
3399(PMake)X
1 f
3670(on)X
3780(a)X
3841(multi-user)X
748 3532(machine)N
1078(\(in)X
1207(contrast)X
1516(to)X
1615(a)X
1684(workstation\))X
2159(calls)X
2351(for)X
2483(judicious)X
2837(use)X
2984(of)X
3087(concurrency)X
3551(to)X
3650(avoid)X
3876(annoying)X
748 3636(the)N
880(other)X
1085(users.)X
1311(The)X
1472(ability)X
1723(to)X
1816(execute)X
2109(tasks)X
2309(in)X
2402(parallel,)X
2713(in)X
2806(combination)X
3272(with)X
3453(the)X
3585(execution)X
3951(of)X
4047(only)X
748 3740(one)N
897(shell)X
1086(per)X
1220(target,)X
1465(brings)X
1707(about)X
1925(decreases)X
2283(in)X
2374(creation)X
2680(time)X
2860(on)X
2970(the)X
3100(order)X
3307(of)X
3402(25%)X
9 f
3563(-)X
1 f
3611(60%.)X
748 3892(The)N
3 f
9 f
917(-)X
3 f
965(J)X
1 f
1041(and)X
3 f
9 f
1200(-)X
3 f
1248(L)X
1 f
1339(\257ags)X
1537(are)X
1676(used)X
1869(to)X
1970(control)X
2252(the)X
2392(number)X
2693(of)X
2798(shells)X
3031(executing)X
3405(at)X
3500(once)X
3697(and)X
3855(should)X
4121(be)X
748 3996(used)N
932(to)X
1024(\256nd)X
1184(the)X
1315(best)X
1480(level)X
1675(for)X
1800(your)X
1984(machine.)X
2328(Once)X
2536(this)X
2687(is)X
2769(found,)X
3019(the)X
3150(default)X
3418(level)X
3613(can)X
3758(be)X
3864(set)X
3985(at)X
4071(that)X
748 4100(point)N
952(and)X
2 f
1101(PMake)X
1 f
1372(recompiled.)X
3 f
10 s
460 4252(BACKWARD-COMPATABILITY)N
2 f
11 s
748 4356(PMake)N
1 f
1020(was)X
1179(designed)X
1515(to)X
1607(be)X
1713(as)X
1808(backwards-compatible)X
2627(with)X
2 f
2806(Make)X
1 f
3023(as)X
3118(possible.)X
3473(In)X
3568(spite)X
3757(of)X
3852(this,)X
4024(how-)X
748 4460(ever,)N
943(there)X
1141(are)X
1270(a)X
1331(few)X
1484(major)X
1712(differences)X
2124(which)X
2361(may)X
2535(cause)X
2752(problems)X
3102(when)X
3314(using)X
3527(old)X
3662(make\256les:)X
748 4612(1\))N
1036(The)X
1200(variable)X
1510(substitution,)X
1971(as)X
2070(mentioned)X
2469(earlier,)X
2742(is)X
2827(very)X
3009(different)X
3338(and)X
3491(will)X
3655(cause)X
3876(problems)X
1036 4716(unless)N
1278(the)X
1408(make\256le)X
1734(is)X
1815(converted)X
2184(or)X
2279(the)X
3 f
9 f
2409(-)X
3 f
2457(V)X
1 f
2542(\257ag)X
2696(is)X
2777(given.)X
748 4868(2\))N
1036(Because)X
1355(targets)X
1615(are)X
1747(created)X
2026(in)X
2120(parallel,)X
2432(certain)X
2697(sequences)X
3078(which)X
3318(depend)X
3597(on)X
3710(the)X
3843(sources)X
4131(of)X
1036 4972(a)N
1097(target)X
1320(being)X
1538(created)X
1814(sequentially)X
2263(will)X
2423(fail)X
2563(miserably.)X
2955(E.g.:)X
1324 5128(prod)N
1507(:)X
1554($\(PROGRAM\))X
2112(clean)X
1036 5284(This)N
1223(is)X
1312(liable)X
1539(to)X
1638(cause)X
1863(some)X
2079(of)X
2182(the)X
2320(object)X
2566(\256les)X
2743(to)X
2842(be)X
2955(removed)X
3293(after)X
3484(having)X
3754(been)X
3950(created)X
1036 5388(during)N
1297(the)X
1436(current)X
1716(invocation)X
2120(\(or,)X
2275(at)X
2370(the)X
2509(very)X
2696(least,)X
2911(the)X
3049(object)X
3295(\256les)X
3472(will)X
3640(not)X
3783(be)X
3896(removed)X
1036 5492(when)N
1252(the)X
1386(program)X
1710(has)X
1853(been)X
2044(made\),)X
2311(leading)X
2596(to)X
2690(errors)X
2919(in)X
3013(the)X
3146(\256nal)X
3328(linking)X
3604(stage.)X
3832(This)X
4014(prob-)X
1036 5596(lem)N
1191(cannot)X
1448(even)X
1636(be)X
1741(gotten)X
1984(around)X
2250(by)X
2360 0.4821(limiting)AX
2664(the)X
2794(maximum)X
3175(concurrency)X
3631(to)X
3722(one,)X
3893(since)X
4096(the)X
1036 5700(traversal)N
1364(of)X
1462(the)X
1595(dependency)X
2040(graph)X
2265(is)X
2349(done)X
2545(in)X
2639(a)X
2703(breadth-\256rst,)X
3180(rather)X
3410(than)X
3587(a)X
3650(depth-\256rst)X
4036(way.)X
1036 5804(This)N
1217(can)X
1363(only)X
1544(be)X
1651(gotten)X
1896(around)X
2163(by)X
2274(rewriting)X
2620(the)X
2751(make\256le,)X
3100(or)X
3196(by)X
3307(invoking)X
2 f
3644(PMake)X
1 f
3916(with)X
4096(the)X
10 s
460 6196(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(16)X

17 p
%%Page: 17 17
10 s 10 xH 0 xS 1 f
11 s
460 392(PMAKE)N
2037(User)X
2224(Commands)X
3897(PMAKE)X
3 f
9 f
1036 784(-)N
3 f
1084(M)X
1 f
1189(\257ag.)X
748 936(One)N
922(other)X
1131(possible)X
1448(con\257ict)X
1741(arises)X
1969(because)X
2 f
2275(PMake)X
1 f
2552(forks)X
2760(only)X
2945(one)X
3100(shell)X
3295(to)X
3391(execute)X
3687(the)X
3822(commands)X
748 1040(to)N
850(re-create)X
1190(a)X
1262(target.)X
1540(This)X
1730(means)X
1988(that)X
2154(changes)X
2470(of)X
2575(directory,)X
2947(environment,)X
3447(etc.,)X
3626(remain)X
3903(in)X
4004(effect)X
748 1144(throughout)N
1158(the)X
1289(creation)X
1596(process.)X
1904(It)X
1981(also)X
2146(allows)X
2399(for)X
2524(a)X
2586(more)X
2790(natural)X
3058(entry)X
3262(of)X
3358(shell)X
3547(constructs,)X
3948(such)X
4131(as)X
748 1248(the)N
889(``for'')X
1140(and)X
1300(``while'')X
1645(loops)X
1869(in)X
1971(the)X
2112(Bourne)X
2404(shell,)X
2626(without)X
2929(the)X
3070(need)X
3268(for)X
3402(backslashes)X
3849(and)X
4008(semi-)X
748 1352(colons)N
1003(required)X
1321(by)X
1433(the)X
1565(one-shell-per-command)X
2430(paradigm)X
2787(used)X
2972(by)X
2 f
3084(Make)X
1 f
3286(.)X
3354(This)X
3535(shouldn't)X
3892(pose)X
4077(any)X
748 1456(serious)N
1020(dif\256culties)X
1421(\(or)X
1546(even)X
1735(any)X
1885(trivial)X
2120(ones)X
2304(so)X
2405(far)X
2525(as)X
2621(I)X
2673(can)X
2818(see\),)X
3004(but)X
3140(should,)X
3419(in)X
3510(fact,)X
3686(make)X
3899(life)X
4039(a)X
4100(lit-)X
748 1560(tle)N
862(easier.)X
1114(It)X
1193(is,)X
1299(however,)X
1648(possible)X
1962(to)X
2055(have)X
2 f
2245(PMake)X
1 f
2518(execute)X
2811(each)X
2996(command)X
3368(in)X
3461(a)X
3524(single)X
3759(shell)X
3950(by)X
4062(giv-)X
748 1664(ing)N
883(it)X
955(the)X
3 f
9 f
1085(-)X
3 f
1133(B)X
1 f
1214(\257ag.)X
3 f
10 s
460 1816(FILES)N
1 f
11 s
748 1920(Make\256le)N
1083(or)X
1178(make\256le)X
1821(default)X
2088(input)X
2292(\256le)X
748 2024 0.1829(/usr/public/lib/pmake/sys.mk)AN
1821(System)X
2103(make\256le)X
2429(\(the)X
2588(built-in)X
2871(rules\))X
3 f
10 s
460 2176(ENVIRONMENT)N
11 s
748 2280(PMAKE)N
1 f
1164(Flags)X
1377(PMake)X
1648(should)X
1905(always)X
2171(use)X
2310(when)X
2522(invoked.)X
3 f
10 s
460 2432(SEE)N
643(ALSO)X
2 f
11 s
748 2536(make)N
1 f
940(\(1\))X
1064(for)X
1188(a)X
1249(more)X
1452(complete)X
1798(explanation)X
2232(of)X
2327(the)X
2457(lower-case)X
2859(\257ags)X
3047(to)X
2 f
3138(PMake)X
1 f
3394(.)X
3 f
10 s
460 2688(KEYWORDS)N
1 f
11 s
748 2792(make,)N
983(transformation)X
3 f
10 s
460 2944(AUTHOR)N
1 f
11 s
748 3048(Adam)N
985(de)X
1090(Boor)X
10 s
460 6152(Sprite)N
671(v1.0)X
1889(Modi\256ed:)X
2244(21)X
2344(January)X
2614(1989)X
4124(17)X

17 p
%%Trailer
xt

xs
