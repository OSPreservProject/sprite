#ifndef DEFPRT
#define DEFPRT "lw"
#endif

#define TROFF 	"/sprite/cmds.$MACHINE/troff_p"
#define LPR	"/sprite/cmds.$MACHINE/lpr"
#define DTBL 	"/sprite/cmds.$MACHINE/tbl"
#define DEQN 	"/sprite/cmds.$MACHINE/eqn"
#define GRN 	"/sprite/cmds.$MACHINE/grn"
#define PIC 	"/sprite/cmds.$MACHINE/pic"
#define REFER 	"/sprite/cmds.$MACHINE/refer"
#define IDEAL 	"/sprite/cmds.$MACHINE/ideal"

#define TMPFILE "/tmp/ditXXXXX"

static char tempfile[] = TMPFILE;

char	*printer = DEFPRT;	/* printer to use */
char	*ditentry;		/* ditcap entry to use */
char	*type;			/* type of printer */
char	*font;			/* font directory */

#include <stdio.h>
#define	NULL	0
#define BUFSIZ	1024
char	ditline[BUFSIZ];	/* variables needed to read the ditcap lines*/
char	ditbuf[BUFSIZ/2];	/*  and the ditcap file  */
char	*bp;

#define BLANK ""
char	files[BUFSIZ] = BLANK;	/* files to print  */
char	flags[BUFSIZ/4] = BLANK;/* troff options - pass to TROFF */
char	spool[BUFSIZ/4] = BLANK;/* spooling options - pass to LPR  */
char	line1[BUFSIZ] = BLANK;	/* lines to be executed   */
char	line2[BUFSIZ] = BLANK;
char	line3[BUFSIZ] = BLANK;
char	type_font[BUFSIZ/4] = BLANK;
char	temp[BUFSIZ] = BLANK;	/* temporary buffer */

char	*FD;			/* ditcap characteristics  */
char	*TR;
char	*LO;
char	*F1;
char	*F2;
char	*F3;
char	*F4;
char	*OL;
char	*FA;
char	*OA;
char	*FT;
char	*OT;
char	*FP;
char	*OP;
char	*PV;
char	*LP;
char	*TB;
char	*EQ;
char	*GR;
char	*PI;
char	*RF;
char	*ID;
char	*SE;


main(argc, argv)
int argc;
char **argv;
{
	char *cp;

	int     gottype=0, gotjob=0, gotfont=0, gotdit=0;
	int     t=0, a=0;
	int	tbl=0, eqn=0, grn=0, seqn=0, pic=0, refer=0, ideal=0, verbose=0;
	int	first=1;
	int	debug = 0;

	char 	*operand();
	char 	*getenv();
	char 	*gettype();
	char	*mktemp();
	int	strlen();
	int	strcmp();

	if (cp=getenv("PRINTER")) printer = cp;
	if (cp=getenv("TYPESETTER")) printer = cp;
	ditentry = printer;

	while (--argc) {
	    if (**++argv != '-')	/* if not an option add to files */
		sprintf(files,"%s %s",files,*argv);
	    else
	      switch ((*argv)[1]) {  	/* process options  */

		case 'P':	/* final output typesetter name */
			printer = operand(&argc, &argv);
			if (!gotdit) ditentry = printer;
			break;

		case 'T':	/* printer type */
			type = operand(&argc, &argv);
			gottype = 1;
			break;

		case 'F':	/* font directory */
			font = operand(&argc, &argv);
			gotfont = 1;
			break;

		case 'D':	/* ditcap entry */
			ditentry = operand(&argc, &argv);
			gotdit = 1;
			break;

		case 't':	
			if ((*argv)[2]) { 
				if (strcmp(*argv,"-tbl") == 0) 
					tbl = 1;             /* -tbl option */
				else 		       /* some other option */
					sprintf(flags,"%s %s",flags,*argv);
			} else {                      /* must be -t option  */
				sprintf(flags,"%s %s",flags,*argv);
				t = 1;
			}
			break;

		case 'a':	/* ascii approximation to standard output  */
			if ((*argv)[2]) { 
				sprintf(flags,"%s %s",flags,*argv);
			} else {                      /* must be -a option  */
				sprintf(flags,"%s %s",flags,*argv);
				a = 1;
			}
			break;

		case 'm':	/* either -mmac TROFF option or
					  -m spooling option   */
			if ((*argv)[2]) 
				sprintf(flags,"%s %s",flags,*argv);
			else
				sprintf(spool,"%s %s",spool,*argv);
			break;

		case 'h':	/* spooling options  */
			if ((*argv)[2]) 
				sprintf(flags,"%s %s",flags,*argv);
			else
				sprintf(spool,"%s %s",spool,*argv);
			break;

		case '#':	/* spooling options  */
			sprintf(spool,"%s %s",spool,*argv);
			break;

		case 'C':	/* change class name - to spooling options */
			cp = operand(&argc, &argv);
			sprintf(temp," -C%s%s",cp,spool);
			strcpy(spool,temp);
			break;

		case 'J':	/* change job name - to spooling options */
			cp = operand(&argc, &argv);
			sprintf(temp," -J%s%s",cp,spool);
			strcpy(spool,temp);
			gotjob = 1;
			break;

		case 'e':	
			if (strcmp(*argv,"-eqn") == 0) 
				eqn = 1;             
			else 		       
				sprintf(flags,"%s %s",flags,*argv);
			break;

		case 'd':	
			if (strcmp(*argv,"-deqn") == 0) 
				eqn = 1;             
			else if (strcmp(*argv,"-dtbl") == 0) 
				tbl = 1;             
			else if (strcmp(*argv,"-debug") == 0) 
				debug = 1;             
			else
				sprintf(flags,"%s %s",flags,*argv);
			break;

		case 's':	
			if (strcmp(*argv,"-seqn") == 0) {
				eqn = 1;             
				seqn = 1;             
			} else 		       
				sprintf(flags,"%s %s",flags,*argv);
			break;

		case 'g':	
			if (strcmp(*argv,"-grn") == 0) 
				grn = 1;             
			else 		       
				sprintf(flags,"%s %s",flags,*argv);
			break;

		case 'p':	
			if (strcmp(*argv,"-pic") == 0) 
				pic = 1;             
			else 		       
				sprintf(flags,"%s %s",flags,*argv);
			break;

		case 'r':	
			if (strcmp(*argv,"-refer") == 0) 
				refer = 1;             
			else 		       
				sprintf(flags,"%s %s",flags,*argv);
			break;

		case 'v':
			verbose = 1;
			break;
			
		case 'i':	
			if (strcmp(*argv,"-ideal") == 0) 
				ideal = 1;             
			else 		       
				sprintf(flags,"%s %s",flags,*argv);
			break;

/* here's the bug: if argument is just '-', it IS a file, i.e. stdin */
		case '\0':
			sprintf(files,"%s %s",files,*argv);
			break;
/* end of bug fix [as, 5/25/89] */

		default:	/* option to be passed to TROFF  */
			sprintf(flags,"%s %s",flags,*argv);
	    }
	}
      
    
	if (!gotjob) {		/* make sure there is a jobname  */
		sprintf(temp," -Jditroff%s",spool);
		strcpy(spool,temp);
	}

				/* get the ditcap line from the ditcap  */
				/* file or environment variable  */
	getditline(ditentry);

				/* get the printer type for the given   */
				/*  ditcap entry from the ditcap file   */
	if (!gottype)  type = gettype(ditentry); 
				/* get the options in the ditcap line   */
	ditoptions();


				/* start building up the lines that we */
				/* will pass to UNIX to be executed    */
				/* according to */
				/* options and then execute the lines */

	if (seqn) {
		if (SE == NULL) {
		     fprintf (stderr,"ditroff: se characteristic not defined in the ditcap file \n");
		     fprintf (stderr,"          no special equation characters defined\n");
		} else if (strlen(files) == 0) {
			fprintf (stderr,"ditroff: warning: ");
			fprintf (stderr,"you can't use the -seqn option ");
			fprintf (stderr,"when ditroff \n");
			fprintf (stderr,"                  ");
			fprintf (stderr,"is not given files as arguements \n");
			fprintf (stderr,"                  ");
			fprintf (stderr,"(ie. when the input is to come");
			fprintf (stderr," from standard input) \n");
		} else {
		     sprintf(temp," %s%s",SE,files);
		     strcpy(files,temp);
		}
	}

	sprintf(type_font," -T%s",type);
	if (gotfont)
		sprintf(type_font,"%s -F%s",type_font,font);
	else if (FD != NULL)
		sprintf(type_font,"%s -F%s",type_font,FD);

	if (refer) {
		sprintf(line1,"%s%s|",RF,files);
		first = 0;
	}
	if (tbl){
		if (first)
			sprintf(line1,"%s%s|",TB,files);
		else
			sprintf(line1,"%s%s|",line1,TB);
		first = 0;
	}
	if (grn){
		if (first)
			sprintf(line1,"%s%s%s|",GR,type_font,files);
		else
			sprintf(line1,"%s%s%s|",line1,GR,type_font);
		first = 0;
	}
	if (pic){
		if (first)
			sprintf(line1,"%s%s%s|",PI,type_font,files);
		else
			sprintf(line1,"%s%s%s|",line1,PI,type_font);
		first = 0;
	}
	if (ideal){
		if (first)
			sprintf(line1,"%s%s%s|",ID,type_font,files);
		else
			sprintf(line1,"%s%s%s|",line1,ID,type_font);
		first = 0;
	}
	if (eqn){
		if (first)
			sprintf(line1,"%s%s%s|",EQ,type_font,files);
		else
			sprintf(line1,"%s%s%s|",line1,EQ,type_font);
		first = 0;
	}


				/* add the end of the line depending  */
				/* on the options                     */

	if (a) {		/* ascii approximation   */
		sprintf(line1,"%s%s%s",line1,TR,type_font);
		if (OA != NULL) sprintf(line1,"%s %s",line1,OA);
		sprintf(line1,"%s%s",line1,flags);
		if (first) sprintf(line1,"%s%s",line1,files);

		if (FA != NULL) sprintf(line1,"%s|%s",line1,FA);

		if (debug)
	        	printf("line1 is \n  %s \n",line1);
		else
			system(line1);

	}
	else if (t) {  		/* send to standard output  */
		sprintf(line1,"%s%s%s",line1,TR,type_font);
		if (OT != NULL) sprintf(line1,"%s %s",line1,OT);
		sprintf(line1,"%s%s",line1,flags);
		if (first) sprintf(line1,"%s%s",line1,files);

		if (FT != NULL) sprintf(line1,"%s|%s",line1,FT);

		if (debug)
	        	printf("line1 is \n  %s \n",line1);
		else
			system(line1);

	}
	else if (PV != NULL) {	/* use the previewer  */
		cp = mktemp(tempfile);
		strcpy(temp,cp);

		sprintf(line1,"%s%s%s",line1,TR,type_font);
		if (OP != NULL) sprintf(line1,"%s %s",line1,OP);
		sprintf(line1,"%s%s",line1,flags);
		if (first) sprintf(line1,"%s%s",line1,files);

		if (FP != NULL) sprintf(line1,"%s|%s",line1,FP);

		sprintf(line1,"%s > %s",line1,temp);
		sprintf(line2,"%s %s",PV,temp);
		sprintf(line3,"/sprite/cmds.$MACHINE/rm -f %s",temp);

		if (debug) {
	        	printf("line1 is \n  %s \n",line1);
	        	printf("line2 is \n  %s \n",line2);
	        	printf("line3 is \n  %s \n",line3); 
		} else {
			system(line1);
			system(line2); 
			system(line3);
		}

	}
	else { 			/* standard troff to printer  */
		sprintf(line1,"%s%s%s",line1,TR,type_font);
		if (OL != NULL) sprintf(line1,"%s %s",line1,OL);
		sprintf(line1,"%s%s",line1,flags);
		if (first) sprintf(line1,"%s%s",line1,files);

		if (F1 != NULL) sprintf(line1,"%s|%s",line1,F1);
		if (F2 != NULL) sprintf(line1,"%s|%s",line1,F2);
		if (F3 != NULL) sprintf(line1,"%s|%s",line1,F3);
		if (F4 != NULL) sprintf(line1,"%s|%s",line1,F4);

		sprintf(line1,"%s|%s",line1,LP);
		sprintf(line1,"%s -P%s",line1,printer);
		sprintf(line1,"%s%s",line1,spool);
		if (LO != NULL) sprintf(line1,"%s %s",line1,LO);

		if (debug) 
	        	printf("line1 is \n  %s \n",line1);
		else 
			system(line1);

	}


}


/*----------------------------------------------------------------------------*
 | Routine:	char  * operand (& argc,  & argv)
 |
 | Results:	returns address of the operand given with a command-line
 |		option.  It uses either "-Xoperand" or "-X operand", whichever
 |		is present.  The program is terminated if no option is present.
 |
 | Side Efct:	argc and argv are updated as necessary.
 *----------------------------------------------------------------------------*/

char *operand(argcp, argvp)
int * argcp;
char ***argvp;
{
	if ((**argvp)[2]) return(**argvp + 2); /* operand immediately follows */
	if ((--*argcp) <= 0) {			/* no operand */
	    fprintf(stderr,"ditroff: command-line option operand missing.\n");
	    exit(8);
	}
	return(*(++(*argvp)));			/* operand is next word */
}


char 	*dgetstr();

/*----------------------------------------------------------------------------*
 | Routine:	getditline(printer)
 |
 | Results:	gets the ditcap line for the printer specified
 *----------------------------------------------------------------------------*/

getditline(device)
	char	*device;
{
	int 	status;

	if ((status = dgetent(ditline,device)) < 0) {
	     fprintf(stderr,"ditroff: can't open the ditcap file \n");
	     exit(1);
	}
	else if (status == 0) {
	     fprintf(stderr,"ditroff: can't find device %s in the ditcap file \n", device);
	     exit(1);
	}
	bp = ditbuf;
}

/*----------------------------------------------------------------------------*
 | Routine:	char *gettype (printer)
 |
 | Results:	returns the printer type by looking at the ditcap line
 *----------------------------------------------------------------------------*/

char *
gettype(device)
	char	*device;
{
	char 	*cp;

	if ((cp = dgetstr("ty", &bp)) == NULL) {
	     fprintf(stderr,"ditroff: no type specified for device %s in the ditcap file \n", device);
	     exit(1);
	}
	return (cp);
}


/*----------------------------------------------------------------------------*
 | Routine:	ditoptions()
 |
 | Results:	gets the options from the ditcap line
 *----------------------------------------------------------------------------*/

ditoptions()
{
	char *cp;

	FD = dgetstr("fd",&bp);

	TR = TROFF;
	if (cp=dgetstr("tr",&bp)) TR = cp; 

	LO = dgetstr("lo",&bp);
	F1 = dgetstr("f1",&bp);
	F2 = dgetstr("f2",&bp);
	F3 = dgetstr("f3",&bp);
	F4 = dgetstr("f4",&bp);
	OL = dgetstr("ol",&bp);
	FA = dgetstr("fa",&bp);
	OA = dgetstr("oa",&bp);
	FT = dgetstr("ft",&bp);
	OT = dgetstr("ot",&bp);
	FP = dgetstr("fp",&bp);
	OP = dgetstr("op",&bp);
	PV = dgetstr("pv",&bp);

	LP = LPR;
	if (cp=dgetstr("lp",&bp)) LP = cp; 

	TB = DTBL;
	if (cp=dgetstr("tb",&bp)) TB = cp; 

	EQ = DEQN;
	if (cp=dgetstr("eq",&bp)) EQ = cp; 

	GR = GRN;
	if (cp=dgetstr("gr",&bp)) GR = cp; 

	PI = PIC;
	if (cp=dgetstr("pi",&bp)) PI = cp; 

	RF = REFER;
	if (cp=dgetstr("rf",&bp)) RF = cp; 

	ID = IDEAL;
	if (cp=dgetstr("id",&bp)) ID = cp; 

	SE = dgetstr("se",&bp);
}
