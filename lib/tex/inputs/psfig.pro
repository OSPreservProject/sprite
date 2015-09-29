%!
% Psfig/TeX Release 1.2
% dvips version
%
% All software, documentation, and related files in this distribution of
% psfig/tex are Copyright 1987, 1988 Trevor J. Darrell
%
% Permission is granted for use and non-profit distribution of psfig/tex 
% providing that this notice be clearly maintained, but the right to
% distribute any portion of psfig/tex for profit or as part of any commercial
% product is specifically reserved for the author.
%
%
% $Header: figtex.pro,v 1.5 87/07/24 20:14:50 trevor Exp $
% $Source: $
%
/TeXscale { 65536 div } def

/DocumentInitState [ matrix currentmatrix currentlinewidth currentlinecap
currentlinejoin currentdash currentgray currentmiterlimit ] cvx def

/startTexFig {
% usage :  x y bb-llx bb-lly bb-urx bb-ury startFig
	/SavedState save def
	userdict maxlength dict begin
	currentpoint transform

	DocumentInitState setmiterlimit setgray setdash setlinejoin setlinecap
		setlinewidth setmatrix

	itransform moveto

	/ury exch TeXscale def
	/urx exch TeXscale def
	/lly exch TeXscale def
	/llx exch TeXscale def
	/y exch TeXscale def
	/x exch TeXscale def
	
	currentpoint /cy exch def /cx exch def

	/sx x urx llx sub div def 	% scaling for x
	/sy y ury lly sub div def	% scaling for y

	sx sy scale			% scale by (sx,sy)

	cx sx div llx sub
	cy sy div ury sub translate
	
	/DefFigCTM matrix currentmatrix def

	/initmatrix {
		DefFigCTM setmatrix
	} def
	/defaultmatrix {
		DefFigCTM exch copy
	} def

	/initgraphics {
		DocumentInitState setmiterlimit setgray setdash 
			setlinejoin setlinecap setlinewidth setmatrix
		DefFigCTM setmatrix
	} def

	/showpage {
		initgraphics
	} def
% /erasepage and /copypage added for MatLab support (tli)
 	/erasepage {
 		initgraphics
 	} def
 	/copypage {} def

} def
% Args are llx lly urx ury (in figure coordinates)
/clipFig {
	currentpoint 6 2 roll
	newpath 4 copy
	4 2 roll moveto
	6 -1 roll exch lineto
	exch lineto
	exch lineto
	closepath clip
	newpath
	moveto
} def
% doclip, if called, will always be just after a `startfig'
/doclip { llx lly urx ury clipFig } def
/endTexFig {
	end SavedState restore
} def
