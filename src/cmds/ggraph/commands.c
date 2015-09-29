#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <math.h>
#include <ctype.h>
#include "ggraph.h"
#include "commands.h"
#include "ggraphdefs.h"

int interact;			/* switch for interactive mode */
/****************************************************************
 *								*
 *	readcmds - read the graph commands in 			*
 *								*
 ****************************************************************/
readcmds (infile)
FILE *infile;
{

    ++curgraph;			/* a new graph */
    cg.gtitle.t_xpos = 0.0;
    cg.gtitle.t_ypos = 0.0;
    cg.gtype = -1;
    cg.gframe.fsize = -1;
    cg.gtitle.t_size = -1;
    cg.legend.t_size = -1;
    cg.yvert = -1;
    cg.xtick.t_size = -1;
    cg.ytick.t_size = -1;
    cg.xlabel.t_size = -1;
    cg.ylabel.t_size = -1;
    cg.gtitle.t_font = -1;
    cg.legend.t_font = -1;
    cg.xlabel.t_font = -1;
    cg.ylabel.t_font = -1;
    cg.xtick.t_font = -1;
    cg.ytick.t_font = -1;
    cg.xgrid_type = -1;
    cg.ygrid_type = -1;
    cg.xpreci1 = -1;
    cg.ypreci1 = -1;
    cg.xpreci2 = -1;
    cg.ypreci2 = -1;
    cg.symbolsz = -1;
    cg.logxsw = -1;
    cg.logysw = -1;
    cg.gminx = HUGE;
    cg.gminy = HUGE;
    cg.gmaxx = -HUGE;
    cg.gmaxy = -HUGE;
    curline = -1;
    rreadcmds(infile);
    return;
}

/****************************************************************
 *								*
 *	rreadcmds - recursivly read the graph commands in	*
 *								*
 ****************************************************************/
rreadcmds (readfile)
FILE *readfile;
{
    char    cmd[4];
    char    cargs[80];
    register int    line1;
    register int    ncmd;
    register int i;
    int     ilarg, ilarg1;
    char    larg[40];
    char    linename[20];
    register char   c;
    FILE *infile;
    FILE *oinfile;		/* place to hole file pointer */

    infile = readfile;
    while (TRUE) {
	if((interact) && (infile == stdin))
	  printf("(gg)");
	while (isspace(c = getc (infile)));/* strip blanks */
	ungetc (c, infile);
	if (NULL == fgets (cmd, 4, infile))/* read a command */
	    return;
	while (isspace(c = getc (infile)));/* strip blanks */
	ungetc (c, infile);
	fgets (cargs, 81, infile);/* read command arguments */
	if (strlen (cargs) > 1)
	    cargs[strlen (cargs) - 1] = NULL;
	for(i=0;((i != 79) && (cargs[i] != COMMENTCHAR) && 
		 (cargs[i] != NULL));++i);
	cargs[i] = NULL;
	strip(cargs);
	if ((ncmd = findcmd (cmd)) == ERROR) {/* what command was it */
	    printf ("unrecognized command |%s|\n", cmd);
	    ncmd = MAXCOMMAND+1;
	}
	switch (ncmd) {
	    case XGRID: 
	        xgridf = onoff(cargs);
		break;
	    case YGRID: 
	        ygridf = onoff(cargs);
		break;
	    case XAXIS: 
		xaxisf = onoff(cargs);
		break;
	    case YAXIS: 
		yaxisf = onoff(cargs);
		break;
	    case XTICK: 
		xtickf = onoff(cargs);
		break;
	    case YTICK: 
		ytickf = onoff(cargs);
		break;
	    case XTICKL: 
		xticklf = onoff(cargs);
		break;
	    case YTICKL: 
		yticklf = onoff(cargs);
		break;
	    case TITFLG: 
		titlef = onoff(cargs);
		break;
	    case TITLE: 
		strcpy (cg.gtitle.t_text, cargs);
		break;
	    case XLABLE: 
		strcpy (cg.xlabel.t_text, cargs);
		break;
	    case YLABLE: 
		strcpy (cg.ylabel.t_text, cargs);
		break;
	    case DATASTART: 
		if(debug)fprintf (stderr, "reading data |%s|\n", cargs);
		cg.logxtick = -1;
		cg.logytick = -1;
		cg.dtickx = 0.0;
		cg.dticky = 0.0;
		cg.maxlines = readpoints (infile,cargs);/* read the points in */
		if(debug)fprintf (stderr, "Done reading %s\n", cargs);
		break;
	    case DRAW: 
		if (outfile == NULL) {
		/* is there any place to put this */
		    if (strlen (cargs) > 1) {
		    /* open output file */
			for(i=0;((i != 79) && (cargs[i] != COMMENTCHAR) &&
			  (cargs[i] != BLANK) && (cargs[i] != NULL));++i);
			cargs[i] = NULL;
			if ((outfile = fopen (cargs, "w")) == NULL) {
			    perror ("can't open output file using stdout");
			    outfile = stdout;
			}
			setbuf (outfile, NULL);/* no buffering */
		    }
		    else {
			outfile = stdout;
			setbuf (outfile, NULL);/* no buffering */
		    }
		}
		else {
		    if (strlen (cargs) > 1) {
			fclose (outfile);
		    /* open output file */
			i=0;
			while((i != 79) && (cargs[i] != COMMENTCHAR) &&
			  (cargs[i] != BLANK) && (cargs[i] != NULL))++i;
			cargs[i] = NULL;
			if ((outfile = fopen (cargs, "w")) == NULL) {
			    perror ("can't open output file using stdout");
			    exit (0);
			}
			else
			    setbuf (outfile, NULL);/* no buffering */
		    }
		}
	    /* draw graph */
		if(debug) fprintf (stderr, "Drawing graph %s\n", cargs);
		strcpy(graphname, cargs);
		setdefault ();	/* set defaults for graph */
		setlim ();	/* set the limits for the graph */
		gremlinheader ();/* set up header for file */
		switch (cg.gtype) {/* draw the graph */
		    case LINEAR: 
			plotpoints ();
			break;
		    case HIST: 
			hist ();
			break;
		    case LOG: 
		    case PIE: 
		    case BAR: 
		    default: 
			fprintf (stderr, "Graph type unsupported %d\n",
				cg.gtype);
			break;
		}
	    /* draw and label axis */
		if (!cg.axline_type)
		    cg.axline_type = BRUSH_THICK;
		drawaxis ();
		if (framef)
		    drawframe ();
		if (titlef && (strlen(cg.gtitle.t_text) > 0))	/* draw the title */
		    drawctext (cg.gtitle.t_xpos, cg.gtitle.t_ypos,
			    cg.gtitle.t_font, cg.gtitle.t_size, 
			    cg.gtitle.t_text, TOPCENTER_TEXT);
		if(legendf)
		  drawlegend();
		fprintf (outfile, "%4.1f\n", LASTPOINT);/* close file */
		fclose(outfile);
		if(debug)fprintf (stderr, "Graph done\n");
		cg.xorigin = 0.0;
		cg.yorigin = 0.0;
		break;
	    case SETX: 
		cg.maxtickx = 0;
		cg.mintickx = 0;
		cg.stickx = 0;
		cg.numtickx = 0;
		if(cg.logxsw == TRUE)
		  sscanf (cargs, "%d %f %f %f ", &cg.numtickx, &cg.logxtick,
		  &cg.stickx, &cg.xorigin);
		else
		  sscanf (cargs, "%d %f %f %f ", &cg.numtickx, &cg.dtickx,
			&cg.stickx, &cg.xorigin);
		break;
	    case SETY: 
		cg.maxticky = 0;
		cg.minticky = 0;
		cg.sticky = 0;
		cg.numticky = 0;
		if(cg.logysw == TRUE)
		  sscanf (cargs, "%d %f %f %f ", &cg.numticky, &cg.logytick,
		  &cg.sticky, &cg.yorigin);
		else
		  sscanf (cargs, "%d %f %f %f ", &cg.numticky, &cg.dticky,
			&cg.sticky, &cg.yorigin);
		break;
	    case LTYPE: 
/* 		linearg(cargs, foobar); */
		sscanf (cargs, "%s %d", linename, &ilarg);
		if((line1 = findline (linename)) != ERROR)
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		      cg.lines[i]->ltype = ilarg;
		  }else
		    cg.lines[line1]->ltype = ilarg;
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case LCURVE: 
		sscanf (cargs, "%s %d", linename, &ilarg);
		if((line1 = findline (linename)) != ERROR)
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		      cg.lines[i]->ctype = ilarg;
		  }else
		  cg.lines[line1]->ctype = ilarg;
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case LONOFF: 
		sscanf (cargs, "%s %s", linename, larg);
		if((line1 = findline (linename)) != ERROR){
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i){
		    if (strncmp ("off", larg, 4) == 0)
		      cg.lines[i]->lonoff = 0;
		    if (strncmp ("on", larg, 4) == 0)
		      cg.lines[i]->lonoff = 1;
		    }
		  }else{
		    if (strncmp ("off", larg, 4) == 0)
		      cg.lines[line1]->lonoff = 0;
		    if (strncmp ("on", larg, 4) == 0)
		      cg.lines[line1]->lonoff = 1;
		  }
		}
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case LSYM: 
		sscanf (cargs, "%s %d", linename, &ilarg);
		if((line1 = findline (linename)) != ERROR)
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		      cg.lines[i]->mtype = ilarg;
		  }else
		      cg.lines[line1]->mtype = ilarg;
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case LINELABEL: 
		sscanf (cargs, "%s %s", linename, larg);
		if((line1 = findline (linename)) != ERROR){
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i){
		    if (strncmp ("off", larg, 4) == 0)
		      cg.lines[i]->llabsw = 0;
		    if (strncmp ("on", larg, 4) == 0)
		      cg.lines[i]->llabsw = 1;
		    }
		  }else{
		    if (strncmp ("off", larg, 4) == 0)
		      cg.lines[line1]->llabsw = 0;
		    if (strncmp ("on", larg, 4) == 0)
		      cg.lines[line1]->llabsw = 1;
		  }
		}
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case LLINELABPOS: 
		sscanf (cargs, "%s %f %f", linename, &graphx, &graphy);
		if((line1 = findline (linename)) != ERROR){
		    cg.lines[line1]->llabel.t_xpos = graphx;
		    cg.lines[line1]->llabel.t_ypos = graphy;
		}
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case SLINELABEL: 
		sscanf (cargs, "%s ", linename);
		if((line1 = findline (linename)) != ERROR)
		  strcpy (cg.lines[line1]->llabel.t_text,
		     &cargs[strlen (linename) + 1]);
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case FRAME: 
		framef = onoff(cargs);
		break;
	    case FRAMETHICK: 
		sscanf (cargs, "%d", &ilarg);
		cg.gframe.fsize = ilarg;
		break;
	    case XGRTYPE: 
		sscanf (cargs, "%d", &ilarg);
		cg.xgrid_type = ilarg;
		break;
	    case YGRTYPE: 
		sscanf (cargs, "%d", &ilarg);
		cg.ygrid_type = ilarg;
		break;
	    case TITLEPOS: 
		sscanf (cargs, "%d %d", &ilarg, &ilarg1);
		cg.gtitle.t_xpos = ilarg;
		cg.gtitle.t_ypos = ilarg1;
		break;
	    case XPOS: 
		sscanf (cargs, "%d %d", &ilarg, &ilarg1);
		cg.xlabel.t_xpos = ilarg;
		cg.xlabel.t_ypos = ilarg1;
		break;
	    case YPOS: 
		sscanf (cargs, "%d %d", &ilarg, &ilarg1);
		cg.ylabel.t_xpos = ilarg;
		cg.ylabel.t_ypos = ilarg1;
		break;
	    case TITLESIZE: 
		sscanf (cargs, "%d", &ilarg);
		cg.gtitle.t_size = ilarg;
		break;
	    case YSIZE: 
		sscanf (cargs, "%d", &ilarg);
		cg.ylabel.t_size = ilarg;
		break;
	    case XSIZE: 
		sscanf (cargs, "%d", &ilarg);
		cg.xlabel.t_size = ilarg;
		break;
	    case LTHICK: 
		sscanf (cargs, "%s %d", linename, &ilarg);
		if((line1 = findline (linename)) != ERROR)
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		      cg.lines[i]->lsize = ilarg;
		  }else
		    cg.lines[line1]->lsize = ilarg;
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case GTYPE: 
		sscanf (cargs, "%d", &ilarg);
		cg.gtype = ilarg;
		break;
	    case XPRECISION:
		sscanf (cargs, "%d %d", &ilarg, &ilarg1);
		cg.xpreci1 = ilarg + ilarg1;
		cg.xpreci2 = ilarg1;
		break;
	    case YPRECISION:
		sscanf (cargs, "%d %d", &ilarg, &ilarg1);
		cg.ypreci1 = ilarg + ilarg1;
		cg.ypreci2 = ilarg1;
		break;
	    case SYMBOLSZ: 
		sscanf (cargs, "%d", &ilarg);
		cg.symbolsz = (float)ilarg;
		break;
	    case SYMBOLSW: 
		symbsw = onoff(cargs);
		break;
	    case TITLEFT: 
		sscanf (cargs, "%d", &ilarg);
		cg.gtitle.t_font = ilarg;
		break;
	    case XFONT: 
		sscanf (cargs, "%d", &ilarg);
		cg.xlabel.t_font = ilarg;
		break;
	    case YFONT: 
		sscanf (cargs, "%d", &ilarg);
		cg.ylabel.t_font = ilarg;
		break;
	    case READ:
		oinfile = infile;
		if((!strlen(cargs) > 1)){
		  fprintf(stderr, "No file name in read command\n");
		  break;
		}
		for(i=0;((i != 79) && (cargs[i] != COMMENTCHAR) &&
		  (cargs[i] != BLANK) && (cargs[i] != NULL));++i);
		cargs[i] = NULL;
		if ((infile = fopen (cargs, "r")) == NULL) {/* open input file */
		    perror ("can't open input file");
		    exit (-1);
		}
		rreadcmds(infile);
		fclose(infile);
		infile = oinfile;
		break;
	    case VERTICALT: 
		cg.yvert = onoff(cargs);
		break;
	    case LOGX: 
		cg.dtickx = 0.0;
		cg.maxtickx = 0;
		cg.numtickx = 0;
		cg.logxsw = onoff(cargs);
		break;
	    case LOGY: 
		cg.dticky = 0.0;
		cg.maxticky = 0;
		cg.numticky = 0;
		cg.logysw = onoff(cargs);
		break;
	    case LOGLOG: 
		cg.dtickx = 0.0;
		cg.dticky = 0.0;
		cg.maxtickx = 0;
		cg.maxticky = 0;
		cg.numtickx = 0;
		cg.numticky = 0;
		if (strncmp ("off", cargs, 4) == 0){
		    cg.logxsw = FALSE;
		    cg.logysw = FALSE;
		}
		if (strncmp ("on", cargs, 4) == 0){
		    cg.logxsw = TRUE;
		    cg.logysw = TRUE;
		}
		break;
	    case QUIT:
		return;
		break;
	    case FIRST_LAB:
		if (strncmp ("off", cargs, 4) == 0){
		    cg.firstxsw = 1;
		    cg.firstysw = 1;
		}
		if (strncmp ("on", cargs, 4) == 0){
		    cg.firstysw = 0;
		    cg.firstxsw = 0;
		}
		break;
	    case FIRST_LAB_X:
		cg.firstxsw = onoff(cargs);
		break;
	    case FIRST_LAB_Y:
		cg.firstysw = onoff(cargs);
		break;
	    case COMMENT:	/* comment line do nothing */
		break;
	    case AXCROSS:
		if (strncmp ("off", cargs, 4) == 0){
		    crossysw = FALSE;
		    crossxsw = FALSE;
		}
		if (strncmp ("on", cargs, 4) == 0){
		    crossxsw = TRUE;
		    crossysw = TRUE;
		}
		break;
	    case AXCROSS_X:
		crossxsw = onoff(cargs);
		break;
	    case AXCROSS_Y:
		crossysw = onoff(cargs);
		break;
	    case LEGEND:
		cg.xlabel.t_xpos = 0.0;
		cg.xlabel.t_ypos = 0.0;
		legendf = onoff(cargs);
		break;
	    case LEGEND_SIDE:
		cg.xlabel.t_xpos = 0.0;
		cg.xlabel.t_ypos = 0.0;
		legendside = onoff(cargs);
		break;
	    case LEGEND_BOX:
		legendbox = onoff(cargs);
		break;
	    case SET_LEGEND_LABEL: 
		sscanf (cargs, "%s ", linename);
		if((line1 = findline (linename)) != ERROR)
		  strcpy (cg.lines[line1]->llelabel.t_text,
		     &cargs[strlen (linename) + 1]);
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case SET_LEGEND_HEADING: 
		strcpy (cg.legend.t_text, cargs);
		break;
	    case SET_LEGEND_HEAD_FONT: 
		sscanf (cargs, "%d", &ilarg);
		cg.legend.t_font = ilarg;
		break;
	    case SET_LEGEND_HEAD_SIZE: 
		sscanf (cargs, "%d", &ilarg);
		cg.legend.t_size = ilarg;
		break;
	    case XTICK_FONT:
		sscanf (cargs, "%d", &ilarg);
		cg.xtick.t_font = ilarg;
		break;
	    case YTICK_FONT:
		sscanf (cargs, "%d", &ilarg);
		cg.ytick.t_font = ilarg;
		break;
	    case XTICK_SIZE:
		sscanf (cargs, "%d", &ilarg);
		cg.xtick.t_size = ilarg;
		break;
	    case YTICK_SIZE:
		sscanf (cargs, "%d", &ilarg);
		cg.ytick.t_size = ilarg;
		break;
	    case XSET: 
		cg.maxtickx = 0;
		cg.mintickx = 0;
		cg.stickx = 0;
		cg.numtickx = 0;
		if(cg.logxsw == TRUE)
		  sscanf (cargs, "%f %f %f %d", &cg.logxtick, &cg.gminx,
		  &cg.gmaxx, &cg.numtickx);
		else{
		  sscanf (cargs, "%f %f %d", &cg.gminx, &cg.gmaxx, &cg.numtickx);
		  if(cg.numtickx)
		    cg.dtickx = (cg.gmaxx - cg.gminx)/cg.numtickx;
		  cg.stickx = cg.gminx;
		}
		break;
	    case YSET: 
		cg.maxticky = 0;
		cg.minticky = 0;
		cg.sticky = 0;
		cg.numticky = 0;
		if(cg.logysw == TRUE)
		  sscanf (cargs, "%f %f %f %d", &cg.logytick, &cg.gminy,
		  &cg.gmaxy, &cg.numticky);
		else{
		  sscanf (cargs, "%f %f %d", &cg.gminy, &cg.gmaxy, &cg.numticky);
		  if(cg.numticky)
		    cg.dticky = (cg.gmaxy - cg.gminy)/cg.numticky;
		  cg.sticky = cg.gminy;
		}
		break;
	    case LINE_LAB_FONT:
		sscanf (cargs, "%s %d", linename, &ilarg);
		if((line1 = findline (linename)) != ERROR)
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		      cg.lines[i]->llabel.t_font = ilarg;
		  }else
		    cg.lines[line1]->llabel.t_font = ilarg;
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case LINE_LAB_SIZE:
		sscanf (cargs, "%s %d", linename, &ilarg);
		if((line1 = findline (linename)) != ERROR)
		  if(line1 == ALLINES){
		    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		      cg.lines[i]->llabel.t_size = ilarg;
		  }else
		    cg.lines[line1]->llabel.t_size = ilarg;
		else
		  fprintf(stderr,"Illegal line name %s\n", linename);
		break;
	    case SET_UNITS:
		sscanf (cargs, "%d", cg.graph_units);
		break;
	    case XAXIS_LENGTH:
		sscanf (cargs, "%d", cg.xaxis_length);
		break;
	    case YAXIS_LENGTH:
		sscanf (cargs, "%d", cg.yaxis_length);
		break;
	    default: 
		fprintf(stderr, "Command found but not found\n");
		break;
	}
    }
}

findcmd (cmdstr)
char   *cmdstr;
{
    register int    i;
 
    if (strlen (cmdstr) == 1) 
      return(MAXCOMMAND+1);
    for (i = 0; i != MAXCOMMAND; ++i) {
	if (strncmp (commands[i], cmdstr, 3) == 0)
	    return (i);
    }
    return (ERROR);
}

findline (line_name)
char   *line_name;
{
    register int    i;
    if(strncmp("all", line_name, 20) == 0) /* special case all */
      return(ALLINES);
    for (i = 0; ((i != MAXLINES) && (cg.lines[i] != NULL)); ++i) {
	if (strncmp (cg.lines[i]->lname, line_name, 20) == 0)
	    return (i);
    }
    return (ERROR);
}

/* strip blanks from front and back of line */
strip(strip_line)
char strip_line[];
{
    register int i;		/* a counter */
    char temp_line[80];		/* place to hold line while we work on it */

    for(i=0;i!=80;++i)
      temp_line[i] = strip_line[i];
    i = 0;			/* reset pointer */
    while(isspace(temp_line[i]))/* strip leading blanks */
      ++i;
    for(;((i!=80) && (strip_line[i] != NULL));++i)/* move back */
      strip_line[i] = temp_line[i];
    --i;
    while(isspace(strip_line[i]))/* strip leading blanks */
      --i;
    ++i;
    strip_line[i] = NULL;
}

onoff(command_arg)
char *command_arg;
{   
    
    if (strncmp ("off", command_arg, 4) == 0)
	return(FALSE);
    if (strncmp ("on", command_arg, 4) == 0)
	return(TRUE);
    else
	return(ERROR);
}

linearg(command_arg, line_field)
char *command_arg;
int line_field;
{   
char linename[20];
register int line1, i;
int ilarg;		

    sscanf (command_arg, "%s %d", linename, &ilarg);
    if((line1 = findline (linename)) != ERROR)
	if(line1 == ALLINES){
	    for(i=0;((i!=MAXLINES) && (cg.lines[i] != NULL));++i)
		cg.lines[i]->ltype = ilarg;
	}else
    	    cg.lines[line1]->ltype = ilarg;
    else
	fprintf(stderr,"Illegal line name %s\n", linename);
    return;
}

globalarg()
{   
}
