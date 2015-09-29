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
#define HEADERFILE "tex.pro"
#define CHEADERFILE "texc.pro"
#define PSFONTHEADER "texps.pro"
#define SPECIALHEADER "special.pro"
#define PSMAPFILE "psfonts.map"
