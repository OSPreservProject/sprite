#include <stdio.h>
#include <errno.h>
#include "ggraph.h"
#include "commands.h"
#include "ggraphdefs.h"

char *commands[MAXCOMMAND] = {"xgr", "ygr", "xax", "yax", "xti", "yti",
 "xtl", "ytl", "gti", "das", "xla", "yla", "tif", "dae", "xst", "yst",
"lty", "lcu", "lof", "lsy", "dra", "lla", "llp", "sla","fra", "frt",
"xgt", "ygt", "gtp", "ytp", "xtp", "tis", "xts", "yts", "lth", "typ",
"xpr", "ypr", "syz", "ssw", "tft", "xft", "yft", "rea", "ver", "lox", 
"loy", "log", "qui", "xfi", "yfi", "fir", "com", "cro", "crx", "cry",
"leg", "leb", "les", "sle", "slh", "slf", "sls", "xtf", "ytf", "xtz",
"ytz", "sxt", "syt", "llf", "lls", "xle", "yle", "sun"};

char *justify_names[] = { "BOTLEFT", "BOTRIGHT", "CENTCENT", 
				 "", "", "", "", "", "", "", 
				 "TOPLEFT", "TOPCENT", "TOPRIGHT", 
				 "CENTLEFT", "CENTRIGHT", "BOTCENT" };

int ycharsz[5] = {0, 10, 14, 22, 30};
int xcharsz[5] = {0, 6, 8, 10, 15};
int descenders[5] = {0, 1, 3, 2, 6};
struct agraph graph[MAXGRAPH];

float   graphx, graphy;		/* a point on the graph */

int     curline = -1;
int     curgraph = -1;
int     errno;

char    firstline[] = FIRSTLINE; /* first line of gremlin file */
char    sfirstline[] = SFIRSTLINE; /* first line of gremlin file */

int     xgridf = TRUE;
int     ygridf = TRUE;
int     xaxisf = TRUE;
int     yaxisf = TRUE;
int     xtickf = TRUE;
int     ytickf = TRUE;
int     xticklf = TRUE;
int     yticklf = TRUE;
int     titlef = TRUE;
int     framef = FALSE;
int     symbsw = TRUE;
int     crossxsw = FALSE;
int     crossysw = FALSE;
int 	legendf = FALSE;
int 	legendbox = FALSE;
int 	legendside = TRUE;

FILE *outfile = NULL;		/* output file */
FILE *fopen ();

int debug;			/* debbuging switch */
char graphname[40];
int version = SUN_GREMLIN;	/* we are for the suns */
