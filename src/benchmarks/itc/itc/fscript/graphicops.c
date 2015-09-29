/* Graphic opcode information table */

struct GR_opinfo {
    char    nargs;
    char    HasStringArgument:1;
}           GR_opinfo[] = {
	2,	0,	/* GR_MOVE */
	2,	0,	/* GR_LINE */
	0,	0,	/* GR_GETDIMS */
	2,	0,	/* GR_GETDIMSR */
	1,	0,	/* GR_SETFUNC */
	0,	1,	/* GR_SETTTL */
	4,	0,	/* GR_SETDIMS */
        3,	0,	/* GR_MOUSECHANGE */
	0,	0,	/* GR_SETRAWIN */
	0,	0,	/* GR_DISABLEINPUT */
	0,	0,	/* GR_ENABLEINPUT */
	1,	0,	/* GR_SETMOUSEINTEREST */
	6,	0,	/* GR_RASTEROP */
	4,	0,	/* GR_RASTERSMASH */
	0,	0,	/* GR_DELETEWINDOW */
	0,	1,	/* GR_DEFINEFONT */
	1,	0,	/* GR_DEFINEFONTR */
	1,	0,	/* GR_SELECTFONT */
	0,	1,	/* GR_SETMOUSEPREFIX */
	1,	0,	/* GR_MYPGRPIS */
	0,	1,	/* GR_ADDMENU */
	0,	0,	/* GR_DISABLENEWLINES */
	4,	-1,	/* GR_SHOWBITS */
	1,	0,	/* GR_SENDFONT */
	1,	0,	/* GR_HEREISFONT */
	2,	0,	/* GR_SETCURSOR */
	1,	0,	/* GR_SETSPACESHIM */
	1,	0,	/* GR_SETCHARSHIM */
	1,	1,	/* GR_DEFINECOLOR */
	2,	0,	/* GR_HEREISCOLOR */
	1,	0,	/* GR_SELECTCOLOR */
	0,	0,	/* GR_HIDEME */
	0,	0,	/* GR_EXPOSEME */
	0,	1,	/* GR_SETPROGRAMNAME */
	1,	0,	/* GR_SETMOUSEGRID */
	5,	0,	/* GR_DEFINEREGION */
	1,	0,	/* GR_SELECTREGION */
	0,	0,	/* GR_ZAPREGIONSS */
	4,	0,	/* GR_SETCLIPRECTANGLE */
	0,	0,	/* GR_ACQUIREINPUTFOCUS */
	0,	0,	/* GR_GIVEUPINPUTFOCUS */
	0,	0,	/* GR_IHANDLEACQUISITION */
	0,	0,	/* GR_WRITETOCUTBUFFER */
	1,	0,	/* GR_READFROMCUTBUFFER */
	1,	0,	/* GR_HEREISCUTBUFFER */
	1,	0,	/* GR_ROTATECUTRING */
	5,	0,	/* GR_SAVEREGION */
	3,	0,	/* GR_RESTOREREGION */
	1,	0,	/* GR_FORGETREGION */
	3,	0,	/* GR_HEREISREGION */
	8,	0,	/* GR_FILLTRAPEZOID */
	4,	0,	/* GR_ZOOMFROM */
	0,	1,	/* GR_SETMENUPREFIX */
	2,	0,	/* GR_LINKREGION */
	1,	1,	/* GR_NAMEREGION */
};
