/*
 *   OUTPATH is where to send the output.  If you want a .ps file to
 *   be created by default, set this to "".  If you want to automatically
 *   invoke a pipe (as in lpr), make the first character an exclamation
 *   point or a vertical bar, and the remainder the command line to
 *   execute.
 */
#define OUTPATH ""
/*   (Actually OUTPATH will be overridden by an `o' line in config.ps.) */
/*
 *   Names of config and prologue files:
 */
#ifndef CONFIGFILE
#define CONFIGFILE "config.ps"
#endif

#ifdef MSDOS
#define DVIPSRC "dvips.ini"
#else
#define DVIPSRC ".dvipsrc"
#endif

#define HEADERFILE "tex.pro"
#define CHEADERFILE "texc.pro"
#define PSFONTHEADER "texps.pro"
#define IFONTHEADER "finclude.pro"
#define SPECIALHEADER "special.pro"
#define PSMAPFILE "psfonts.map"

/* arguments to fopen */
#define READ            "r"

/* directories are separated in the path by PATHSEP */
/* DIRSEP is the char that separates directories from files */
#ifdef MSDOS
#define READBIN		"rb"	/* MSDOS must use binary mode */
#define PATHSEP         ';'
#define DIRSEP		'\\'
#else
#ifdef VMS
#define READBIN		"rb"	/* VMS must use binary mode */
#define PATHSEP         ','
#define DIRSEP		':'
#else
#define READBIN		"r"	/* UNIX doesn't care */
#define PATHSEP         ':'
#define DIRSEP          '/'
#endif
#endif

extern void error() ;

#ifdef VMS
#define TFMPATH "TEX$FONTS:"
#define PKPATH "TEX_PK"
#define VFPATH  "TEX_VF:"
#define CONFIGPATH "TEX$POSTSCRIPT"
#define HEADERPATH  "TEX$POSTSCRIPT:,SYS$LOGIN:"
#define FIGPATH  "TEX$POSTSCRIPT:"
#define FLIPATH ""
#define FLINAME ""
#else
#ifdef MSDOS
#define TFMPATH "c:\\emtex\\tfm"
#define PKPATH "c:\\texfonts\\pixel.lj\\%ddpi\\%f.%p"
#define VFPATH  "c:\\texfonts\\vf"
#define HEADERPATH  ".;c:\\emtex\\ps"
#define FIGPATH  ".;c:\\emtex\\texinput"
#define CONFIGPATH ".;c:\\emtex\\ps"
#define FLIPATH  "c:\\texfonts"
#define FLINAME  "lj_0;lj_h;lj_1;lj_2;lj_3;lj_4;lj_5a;lj_5b;lj_sli"
#else
#ifndef TFMPATH
#define TFMPATH "/usr/lib/tex/fonts/tfm"
#endif
#ifndef PKPATH
#define PKPATH  "/usr/lib/tex/fonts/pk"
#endif
#ifndef VFPATH
#define VFPATH  "/usr/lib/tex/fonts/vf"
#endif
#ifndef CONFIGPATH
#define CONFIGPATH  "/usr/lib/tex/ps"
#endif
#ifndef HEADERPATH
#define HEADERPATH  "/usr/lib/tex/ps"
#endif
#ifndef FIGPATH
#define FIGPATH  ".:..:/usr/lib/tex/inputs"
#endif
#ifndef FLIPATH
#define FLIPATH ".:/usr/lib/tex/fonts"
#endif
#ifndef FLINAME
#define FLINAME "lj_0:lj_h:lj_1:lj_2:lj_3:lj_4:lj_5a:lj_5b"
#endif
#endif
#endif
