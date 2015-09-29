#ifndef lint
static char RCSid[] = "$Header: ded.c,v 1.9 84/04/25 04:05:32 lepreau Exp $";
#endif

/*
 * Directory editor.
 *	ded <options> [dir] [files]
 *
 * Stuart Cracraft, SRI, Sept 1980
 * Jay Lepreau, Univ of Utah, 1981, 82, 83, 84
 *
 * 	The following may be defined as quoted strings to override
 * 	the defaults.  Only the last need be a full pathname.
 * DEDNAME  	ded's own name.
 * PAGER	The pager. "more" is default.
 * PRINTER	Print program. "lpr" is default.
 * DFLTEDITOR	Editor if EDITOR is not in environment.  Default vi.
 * HELPFILE	Location of help info. /usr/local/lib/ded.hlp is default.
 *
 * Define USG for System III/V et al; BSD41 or BSD42 for Berkeley Vmunix.
 *
 *      Note: if you make improvements, we'd like to get them too.
 *          Stuart Cracraft: mclure@sri-unix, ucbvax!menlo70!sri-unix!mclure
 *	    Jay Lepreau: lepreau@utah-cs, harpo!utah-cs!lepreau
 *
 * Modifications for USG 4.0 by Jerry Schwarz (eagle!jerry) 6/7/82
 */

#ifndef DEDNAME
# define DEDNAME "ded"
#endif

#ifndef PAGER
# define PAGER "more"
#endif

#ifndef PRINTER
# define PRINTER "lpr"
#endif

#ifndef DFLTEDITOR
# define DFLTEDITOR "vi"
#endif

#ifndef HELPFILE
# define HELPFILE "/usr/local/lib/ded.hlp"
#endif

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#ifdef USG
# include <sys/termio.h>
# include <sys/types.h>
# include <sys/sysmacros.h>
#else
# include <sgtty.h>
# include <sys/ioctl.h>
#endif USG
#include <sys/param.h>
#include <sys/stat.h>
#ifdef BSD42
# include <sys/dir.h>
#else
# include <dir.h>			/* will be ndir.h on some systems */
# define MAXPATHLEN 512			/* dunno if this is right */
#endif BSD42

#include "ded.h"

#if !(defined(BSD41) || defined(BSD42) || defined(BSD29))
# define vfork fork
#endif

#ifdef BSD41
# define signal sigset
#endif

/*
 * CTRL and META must be defined differently under gcc, apparently.
 */
#ifndef CTRL				
# define CTRL(c)	(c & 037)	
#endif /* CTRL */ 
#define META(c)		(c | 0200)

/*	Sort Orders - must stay in this order */
#define NOSORT		0		/* directory order */
#define NAME		1		/* filename as string */
#define NUMBER		2		/* filename as number */
#define	SIZE		3
#define	WRITE		4
#define	READ		5
#define NEEDSORT	NAME		/* lowest order requiring sort */
#define NEEDSTAT	SIZE		/* lowest order requiring stat() */
char sortstr[] = "dfnswr";		/* must be in order by above */

#define ESC	    '\033'
#define CURSOR		43	/* X-coord of cursor, just b4 file */

#define DIVCHAR	'-'	/* makes up the divider */
char   divider[132];	/* divides the windows */

char	*dedname    = DEDNAME;
char	*pager	    = PAGER;
char	*printer    = PRINTER;
char	*dflteditor = DFLTEDITOR;
char	*helpfile   = HELPFILE;

#ifdef USG
struct termio	ioctlb;
#else
struct sgttyb	ioctlb;
# ifdef TIOCGLTC
struct ltchars	oltc;
# endif
# ifdef LPASS8
int lpass8 = LPASS8;
# endif
#endif USG

int	internflg[10];
#define recursive internflg[0]		/* recursive invocation of ded */
/* 1 is passed down combuf */

int	lflg;
int	splitflg;		/* Split screen? */
int	allstatflg;		/* All files stat'ed already?  */
short	dpyflg = FALSE;		/* In display mode? */
int	sortyp = NAME;		/* Key to sort on */
int	rflg = 1;		/* Reverse sort flag */
int	totfiles;		/* Total files */
int	nprint;			/* chars printed in this line so far */
int	blurb;			/* 1=>thing in echo area,0 otherwise  */
int	numdeleted;		/* Number of files marked deleted */
time_t	ttime;			/* Temp time variable */
time_t	year;			/* Six months ago */
char	tempbuf[BUFSIZ];	/* random temporary buffer */

int	curfile = 0;		/* Current file */
int	topfile = 0;		/* File at top of screen */
int	curline = 0;		/* Line that we're on */
int	scrlen = 999;	/* Default length of ded index part of screen: */
	/* 999 ==> 2 windows (half size), 0 ==> 1 window (full size) */
int	Worklen = 0;		/* Length of 'working window',the other part*/
int	Worktop = 0;		/* Top of    " " */
int	Tscrlen;		/*Total length of screen,less 1 for cmd line*/
int	scrwid = 79;		/* Width of screen */
char	*thisdir;
#define MAXFLGS 10
char	*flgargv[MAXFLGS+1];
int	freeargv;
struct stat statbuf;

char   *CL,
       *UP,			/* Termcap strings we'll use */
       *DO,
       *HO,
       *CM,
       *CD,			/* clear to end of display */
       *CE,
       *AL,			/* insert line */
       *VS,			/* visual start */
       *VE;
char	PC;
short	autowrap;
short	hardtabs;
char   *xsuff = "";		/* default suffixes to exclude */
int	sigint;
char	bufout[BUFSIZ];
struct lbuf *file;

#ifdef notyet
long
# ifdef xBSD42
    stbtok();
# else
    sztob();
# endif xBSD42
#endif notyet

int	compar();
int	fileselect();
int	catchint();
char   *catargs();
char   *bldnam();
extern char *getname(), *getenv(), *ctime(), *index(), *re_comp(),
	*gets(), *strcpy(), *strcat();
extern char *tgetstr(), *tgoto();

#ifdef SIGTSTP
int	onstop();
# define mask(s)    (1 << ((s)-1))
#else
# define stopcatch()
# define stopdefault()
#endif SIGTSTP

main(argc, argv)
int	argc;
char   *argv[];
{
    register	int	i,
    	    		cc;
    int	    special,		/* flag says had % or # in cmd */
	    otherwin,
	    status,
	    pid,
	    direction,
	    j;
    char    nambuf[BUFSIZ];
    char    combuf[BUFSIZ+100];	/* Holds last ! command */
    register int command;	/* Holds last command */
    register char   *t,
    	   	    *tt;

 /* Get terminal type */

    combuf[0] = '\0';
    setbuf(stdout, (char *) 0);	/* for v7 speed up, vax compatibility -fjl */
    getcap();

    freeargv = 2;
 /* Process arg flags. They must be first! */
    for (i=1; i<argc; i++) {
	if (argv[i][0] != '-')
	    break;
	if (i > MAXFLGS-2) {
	    fprintf(stderr, "Too many flags, %s ignored\n", argv[i]);
	    continue;
	}
	cc = argv[i][1];
	if (cc != 'I')			/* may change in future */
	    flgargv[freeargv++] = argv[i]; /* leave room for execv params */
	switch (cc) {
	  case 'w':			/* Window size */
	    if (argv[i][2] == 'h')
		scrlen = 999;		/* Half */
	    else if (argv[i][2] == 'f')
		scrlen = 0;		/* Full */
	    else
		scrlen = atoi(&argv[i][2]);
	    break;

	  case 's':			/* Initial sort order */
	  case 'r':
	    if ((t = index(sortstr, argv[i][2])) == 0)
		fprintf(stderr, "Unknown sort order '%c', ignored\n",
		  argv[i][2]);
	    else {
		sortyp = t - sortstr;
		rflg = (cc == 's') ? 1 : -1;
	    }
	    break;
	    
	  case 'x':			/* exclude these file suffixes */
	    xsuff = &argv[i][2];
	    break;

	  case 'I':			/* internal flags from higher ded */
	    for (t = &argv[i][2]; *t; t++) {
		internflg[*t - '0']++;
		if (*t == '1') {	/* passed down combuf */
		    t += 2;		/* skip over 1 and ending quote */
		    tt = index(t, '\'');	/* ending quote */
		    *tt = '\0';
		    strcpy(combuf, t);
		    t = tt;
		}
	    }
	    break;

	  default:
	    fprintf(stderr, "Unknown option %s, ignored.\n", argv[i]);
	    break;
	}
    }
    argc -= (i - 1);
    argv += (i - 1);

    if (scrlen == 0)			/* full screen */
	scrlen = Tscrlen;
    else if (scrlen == 999)		/* means split in half */
	scrlen = (Tscrlen - 1) >> 1;	
    if (scrlen < 2)
	scrlen = 2;
    if (Tscrlen < scrlen)
	scrlen = Tscrlen;
    splitflg = (Tscrlen > scrlen+1);	/* 1 extra line for separator */
    if (splitflg) {
	Worklen = Tscrlen - (scrlen+1);	/* size of 'working' window */
	Worktop = scrlen + 1;		/* bottom half for now */
    }
    else
	Worklen = 0;
    
    tt = divider + scrwid - 14;
    for (t = divider; t < tt;)	/* arbitrary length */
    	*t++ = DIVCHAR;
    *t = '\0';
    (void) time(&ttime);
    year = ttime - 6L * 30L * 24L * 60L * 60L;	    /* 6 months ago */
    lflg = 1;

    printf("Reading");
    if (argc == 1)
	Readdir(".");
    else if (argc == 2) {	/* for now, we insist on a directory */
	thisdir = argv[1];
    	if (stat(thisdir, &statbuf) < 0) {
	    printf("\n\007Can't stat %s.\n", argv[1]);
	    if (recursive)
		sleep(1);	/* let him see it */
	    exit(1);
    	}
	if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
	    printf("\n\007Sorry, I can't handle this yet.\n");
	    if (recursive)
		sleep(1);	/* let him see it */
	    exit(1);
	}
	Readdir(thisdir);
    } else {
	/* this alloc's one extra but that's to the good */
	file = ALLOC(struct lbuf, argc);
	if (file == NULL) {
	    printf("\n\007Out of memory.\n");
	    if (recursive)
		sleep(1);	/* let him see it */
	    exit(1);
	}
	while (--argc > 0) {
	    if ((totfiles % 10) == 0)
		(void) putchar('.');
	    file[totfiles].namep = *++argv;
	    file[totfiles++].namlen = strlen(*argv);
	}
    }
    if (totfiles == 0) {
	printf("\n?Empty directory\007\n");
	if (recursive)
	    sleep(1);	/* let him see it */
	exit(0);
    }
    if (sortyp >= NEEDSTAT)
	statall();
    if (sortyp > NOSORT)
	qsort((char *)file, totfiles, sizeof (struct lbuf), compar);

    /*
     * Put this down here so user can interrupt
     * a mistaken read on a huge directory.
     */
    (void) signal(SIGINT, SIG_IGN);

    stopcatch();
    setdpy();
    showscreen(FALSE);
    curxy(CURSOR, 0);
    /*
     * Make quit command different from cmd to quit typing a file,
     * as one hits EOF before one knows and mainline gets the quit.
     *
     * modified 10-3-84 by F. Douglis to accept q after all.
     */
    while ((command = getchar()) != 'Q' && command != 'q') {
	if (blurb)
	    clearmsg();
#if defined(TIOCGLTC) && defined(SIGTSTP)
	if (command == oltc.t_suspc)
	    command = 'z';
#endif
	switch (command) {
#ifdef SIGTSTP
	    case 'z':
		onstop();
		break;
#endif SIGTSTP

	    case 'x': 			/* Abort completely */
		blank(); 		/* x: To be like Mail, readnews, etc*/
		unsetdpy();
		exit(1);
		break;
		
	    case '1':			/* go to top */
	    case '\'':			/* apostrophe, for now */
		curline = 0;
		curfile = 0;
	    	if (topfile != 0) {	/* not on this screen */
		    topfile = 0;	/* curses should be doing this!! */
		    showscreen(FALSE);
		}
		curxy(CURSOR, 0);
		break;

	    case '$':			/* go to end */
	    case 'G':
		if (topfile + scrlen <= totfiles) { /* not on last screen */
		    topfile = totfiles - scrlen + 1;
		    showscreen(FALSE);
		}
		curfile = totfiles - 1;
		curline = totfiles - topfile - 1;
		curxy(CURSOR, curline);
		break;

	    case 'P': 		/* Print a file */
	    case 'p':		/* do a pr2 */
		if ((i = file[curfile].ltype) == 'd' || i == 'L') {
		    telluser("?Can only print files");
		    break;
		}
		tempbuf[0] = '\0';
		(void) bldnam(tempbuf, curfile);
		telluser("Printing...");
		while ((pid = vfork()) == -1)
		    sleep(3);
		if (pid == 0) {
#ifdef UTAH
		    if (command == 'p')		/* little print */
		    	execlp("pr2", "pr2", tempbuf, (char *) 0);
		    else				/* big print */
#endif
		    	execlp(printer, printer, tempbuf, (char *) 0);
		    telluser("?Can't find print program '%s'.\n", printer);
		    _exit(1);
		}
		while ((i = wait(&status)) != pid && i != -1)
		    continue;
		break;

	    case '.':		/* repeat the previous !/% command */
		if (combuf[0] == '\0') {
		    telluser("?No previous command to repeat\007");
		    break;
		}
	    case '!': 		/* Execute a system command in lower window */
	    			/* May garbage the screen, but quicker. */
	    case '%':		/* Like ! but clear the screen */
		if (command != '.')
		    otherwin = splitflg && command == '!';
		promptwin("Command: ", otherwin);
		tempbuf[0] = 'x';	/* dummy kludge */
		if ((command == '.' ? strcpy(tempbuf, combuf) : gets(tempbuf))
		  != 0 && tempbuf[0] != '\0') {
		    extern   char *skipto();
		    register char *op,		/* old ptr */
				  *np;		/* new ptr */
		    char     bldbuf[BUFSIZ];
		    static   char quote[3] = "\\";

		    (void) strcpy(combuf, tempbuf); /* remember the command */
		    bldbuf[0] = '\0';	
		    op = tempbuf;
		    special = 0;
		    while (cc = *(np = skipto(op, "\\%#@"))) {
			special++;		/* set flag */
			*np++ = '\0';	     	/* zap and go past it*/
			(void) strcat(bldbuf, op);	/* 1st part */
			switch (cc) {
			    case '%':		/* complete file name */
				(void) bldnam(bldbuf, curfile);
				break;

			    case '#':		/* # sign, last comp only */
				(void) strcat(bldbuf, file[curfile].namep);
				break;

			    case '@':		/* "current" directory */
				if (thisdir)
				    (void) strcat(bldbuf, thisdir);
				else
				    (void) strcat(bldbuf, ".");
				break;

			    case '\\':
				special--;
				quote[1] = *np++;
				(void) strcat(bldbuf, quote);
			}
			op = np;
		    }
		    (void) strcat(bldbuf, op);
		    /*
		     * sh is stupid, so must escape any # comment chars.
		     * Really only need to do when preceding char !isspace,
		     * but what the hey.
		     */
		    op = bldbuf;
		    while (t = index(op, '#')) {
			if (*(t-1) != '\\') {
			    /* shift the right part over 1 char */
			    for (tt = op + strlen(op) + 1; tt > t; tt--)
				*tt = *(tt-1);
			    *t = '\\';	    /* stick in the \ */
			    op = t + 2;	    /* skip over the \# */
			}
			else op = t + 1;
		    }

		    if (!otherwin)
		    	blank();
		    unsetdpy();
		    /*
		     * Display expanded or previous command.
		     * system() call takes long enuf for him to see it
		     * w/o any special delay.
		     */
		    if (special || command == '.')
			printf( "%s\n", bldbuf);
		    (void) signal(SIGINT, SIG_DFL); /* temp kludge here... */
		    stopdefault();
		    (void) system(bldbuf);
		    (void) signal(SIGINT, SIG_IGN);
		    printf("CR to return...");
		    (void) getchar();
		    stopcatch();
		    setdpy();
		    if (!otherwin)
		    	showscreen(FALSE);
		    curxy(CURSOR, curline);
		}
		else			/* CR only, or EOF, or error */
		    fixup(otherwin || tempbuf[0] == 'x', 1);	/* means EOF*/
		break;

	    case 'r': 		/* Reverse sort */
		curxy(0, Tscrlen);
		printf("reverse ");
		rflg = -1;
	    case 's': 		/* Normal sort */
		if (command == 's') {
		    curxy(0, Tscrlen);
		    rflg = 1;
		}
		printf("sort by [s,f,n,r,w]: ");
		command = getchar();
		while ((t = index(sortstr+NEEDSORT, command)) == 0 &&
		  (command != CTRL('g'))) {
		    curxy(0, Tscrlen);
		    ceol();
		    if (rflg == -1)
			printf("reverse ");
		    printf("sort by size(s), filename(f), filename as number(n), read(r) or write(w) date: ");
		    command = getchar();
		}
		if (command == CTRL('g')) {		/* abort */
		    (void) putchar(CTRL('g'));		/* echo it */
		    clearmsg();
		    break;
		}
	    	sortyp = t - sortstr;
		printf("%c", command);
		if (sortyp >= NEEDSTAT)
		    statall();
		qsort((char *)file, totfiles, sizeof (struct lbuf), compar);
		topfile = 0;
		curfile = 0;
		curline = 0;
		showscreen(FALSE);
		curxy(CURSOR, 0);
		break;

	    case 'e': { 		/* Edit a file or directory */
					/* This section needs cleanup */
		char *execname;
		char **execargv;
		char *edargv[3];
		char *comptr;

		tempbuf[0] = '\0';
		(void) bldnam(tempbuf, curfile);
		if ((i = file[curfile].ltype) == 'd' || i == 'L') {
		    execname = dedname;
		    execargv = flgargv;
		    flgargv[0] = dedname;
		    if (combuf[0]) {	/* should be after fork */
			comptr = ALLOC(char, 5 + strlen(combuf) + 2);
			strcpy(comptr, "-I01'");	/* 5 */
			strcat(comptr, combuf);		/* strlen(combuf) */
			strcat(comptr, "'");		/* 1 + null */
			flgargv[1] = comptr;
		    }
		    else {
			flgargv[1] = "-I0";	/* flag 0 means recursive */
			comptr = 0;
		    }
		    flgargv[freeargv] = tempbuf;
		    flgargv[freeargv+1] = 0;
		}
		else {
		    if ((t = getenv("EDITOR")) == NULL)
			t = dflteditor;
		    execname = t;
		    execargv = edargv;
		    edargv[0] = t;
		    edargv[1] = tempbuf;
		    edargv[2] = 0;
		}
		blank();
		unsetdpy();
		stopdefault();		/* both parent and child */
		while ((pid = vfork()) == -1)
		    sleep(3);
		if (pid == 0) {
		    (void) signal(SIGINT, SIG_DFL);	/* child only */
		    execvp(execname, execargv);
		    printf("Can't find %s\n", execname);
		    _exit(1);
		}
		if (comptr)
		    free(comptr);
		while ((i = wait(&status)) != pid && i != -1)
		    continue;
		stopcatch();
		setdpy();
		showscreen(FALSE);
		curxy(CURSOR, curline);
		break;
	    }

	    case 'm': 		/* 'more' a file */
		if ((i = file[curfile].ltype) == 'd' || i == 'L') {
		    telluser("?Can only page thru files");
		    break;
		}
		(void) strcpy(tempbuf, pager);
		(void) strcat(tempbuf, " ");
		(void) bldnam(tempbuf, curfile);
		blank();
		unsetdpy();
		(void) signal(SIGINT, SIG_DFL);	/* temp kludge here... */
		stopdefault();
		(void) system(tempbuf);
#ifdef notdef
		if (!sigint)
#endif
		    printf("\nCR to return...");
		(void) signal(SIGINT, SIG_IGN);
		stopcatch();
		setdpy();
 	   	(void) getchar();   /* setdpy 1st means non CR will work */
		showscreen(FALSE);
		curxy(CURSOR, curline);
		break;

	    case 'T':	/* use full screen; basically just cat */
	    case 't':	/* type in other window and wrap */
		if ((i = file[curfile].ltype) == 'd' || i == 'L') {
		    telluser("?Can only type files");
		    break;
		}
		tempbuf[0] = '\0';
		/* little t means wait at EOP (really just mean split) */
		if (type(bldnam(tempbuf, curfile), command == 't'))
		    showscreen(FALSE);
		curxy(CURSOR, curline);
		break;

	    case CTRL('l'):   /* Refresh screen */
	    case 'L':	    /* Re-stat this page of the display & redisplay */
		showscreen(command == 'L');
		curxy(CURSOR, curline);
		break;
		
	    case 'l':	    /* Re-stat this file & redisplay */
		tempbuf[0] = '\0';
		if (gstat(bldnam(tempbuf, curfile), curfile) < 0)
			telluser("?Can't stat %s", tempbuf);
		curxy(0, curline);
		pentry(curfile);
		curxy(CURSOR, curline);
		break;

	    case CTRL('v'):		/* emacs lovers */
	    case CTRL('f'):		/* for vi lovers */
	    case 'f':			/* forward window */
		fscreen();
		break;

	    /* meta-v works if you have LPASS8 in your kernel and a metakey */
	    case META('v'):		/* for emacs lovers */
	    case CTRL('b'):		/* for vi lovers */
	    case 'b':			/* backward window */
		bscreen();
		break;
		
	    case 'j':			/* for vi lovers */
	    case CTRL('n'):		/* for emacs lovers */
	    case '\r':			/* for regular folks */
	    case '\n':	 		/* next file */
		if (curfile == totfiles - 1)
		    telluser("?At end of files");
		else
		    if (curline == scrlen - 1) {
			topfile = curfile;
			curline = 0;
			showscreen(FALSE);
			curxy(CURSOR, 0);
			downline();
		    }
		    else
			downline();
		break;
		
	    case '\b':			/* nice on my terminal */
	    case CTRL('p'):		/* for emacs lovers */
	    case 'k':			/* for vi and rogue lovers */
	    case '-':			/* more vi */
		if (curfile == 0)
		    telluser("?At start of files");
		else
		    if (curline == 0) {
			topfile = curfile - scrlen + 1;
			curline = scrlen - 1;
			showscreen(FALSE);
			curxy(CURSOR, curline);
			upline();
		    }
		    else
			upline();
		break;

	    case 'h':	    /* Help */
		if (type(helpfile, TRUE))		/* wait */
		    showscreen(FALSE);
		curxy(CURSOR, curline);
		break;

	    case 'd': 		/* delete file */
		if (file[curfile].flg&DELETED)
		    telluser("?Already marked deleted");
		else {
		    numdeleted++;
		    file[curfile].flg |= DELETED;
		    printf("D%c", '\b');	/* XXX */
		    if (curfile == totfiles - 1) /* last file, no motion */
			break;
		    if (curline + 1 == scrlen)
			fscreen();
		    downline();
		}
		break;

	    case 'u': 		/* undelete file */
		if (!file[curfile].flg&DELETED)
		    telluser("?Not marked deleted");
		else {
		    numdeleted--;
		    file[curfile].flg &= ~DELETED;
		    printf(" %c", '\b');	/* XXX */
		}
		break;
		
	    case '/':			/* search for a filename */
		direction = 1;
	    case '?':			/* backwards search */
		if (command == '?')
		    direction = -1;
		promptwin("Regular Expr: ", otherwin = splitflg);
		(void) gets(tempbuf);
		if (t = re_comp(tempbuf)) {
		    fixup(otherwin, 1);
		    telluser("%s", t);		/* error */
		    break;
  		}
		/* start at next file if we`re using the last regexp */
		for (i = curfile + direction * (tempbuf[0] == '\0');
		     direction == 1 ? i < totfiles : i >= 0;
		     i += direction) {
		    register struct lbuf *p = &file[i];

		    switch (re_exec(p->namep)) {
		    	case -1:
			    fixup(otherwin, 1);
			    telluser("Internal error in re_exec");
			    goto mainloop;
			    
			case 1:				/* found it */
			    curfile = i;
			    /* moved to new screen? */
			    if ( direction == 1 ?
			      (curfile - topfile > scrlen-1) :	/* forward */
			      (curfile < topfile) ) {	/* backward */
			        setdpy();
				displayfile(curfile);
			    }
			    else {			/* same screen */
				curline = curfile - topfile;
				fixup(otherwin, 1);
			    }
			    goto mainloop;
		    }
		}
		fixup(otherwin, 1);
		telluser("No match");
		break;
		    
	    default: 
		/*
	        if (command == 'q')
		    telluser("Quit command is now 'Q' ('x' to abort w/o deleting)");
		else
		*/
		    telluser("Unknown command %c. Type h for help", command);
		break;
	}
mainloop:;
	/* This is so ^D works repeatedly to escape ! and / commands */
	if (feof(stdin))
	    clearerr(stdin);
    }

    /* 'Q' typed */
    if (numdeleted) {
	blank();
	setbuf(stdout, bufout);	/* buffered output here, faster */
	printf("The following %s marked for deletion:\n",
		(numdeleted == 1) ? "is" : "are");
	typefiles();
	printf("\nShall I delete %s? ",
		(numdeleted == 1) ? "this" : "these");
	(void) fflush(stdout);
	setbuf(stdout, (char *) 0);
	if ((command = getchar()) != 'y') {
	    showscreen(FALSE);
	    curxy(CURSOR, curline);
	    goto mainloop;
	}
	else {
	    printf("y\n");
	    /*
	     * unlink`ing a lot of files takes a while, so do it
	     * asyncronously.  We really need to estimate how big
	     * any subdirs are, too.
	     * If recursive the user normally can ^Z and 'bg' it.
	     */
	    if (
#ifdef SIGTSTP
		recursive &&
#endif
		    numdeleted > 20 && fork() > 0)
		exit(0);		/* parent */
	    stopdefault();		/* so can be put into background */
	    unsetdpy();
	    for (i = 0; i < totfiles; i++)
		if (file[i].flg&DELETED) {
		    nambuf[0] = '\0';
		    (void) bldnam(nambuf, i);
		    if (file[i].ltype == 'd')
			rm(nambuf);
		    else if (unlink(nambuf) < 0)
			printf("Delete of %s failed.\n", nambuf);
		}
	    exit(0);
	}
    }
    else {
	blank();
	unsetdpy();
	exit(0);
    }
    /*NOTREACHED*/
}					/* end of main() */

/*
 * Display files marked for deletion.
 */
typefiles()
{
    register int i, j;     
    int longsiz, maxperln, numout, fillen;

    /* First find longest filename */
    longsiz = numout = 0;
    for (i = 0; i < totfiles; i++)
	if (file[i].flg&DELETED && file[i].namlen > longsiz)
	    longsiz = file[i].namlen;
    maxperln = max(1, scrwid / (longsiz + 4));	/* 3 blanks plus type char */

    for (i = 0; i < totfiles; i++)
	if (file[i].flg&DELETED) {
	    /* print type char, do not expand symlinks */
	    pname(&file[i], scrwid, TRUE, FALSE);
	    numout++;
	    if ((numout % maxperln) == 0)
		(void) putchar('\n');
	    else {			/* append filler */
		fillen = longsiz + 3 - file[i].namlen;
		for (j = 0; j < fillen; j++)
		    (void) putchar(' ');
	    }
	}
}

fscreen()
{
    if (topfile + scrlen - 1 > totfiles - 1)
	telluser("?No remaining windows");
    else {
	topfile = topfile + scrlen - 1;
	curfile = topfile;
	curline = 0;
	showscreen(FALSE);
	curxy(CURSOR, 0);
    }
}

bscreen()
{
    if (topfile == 0 && curfile == 0)
	telluser("?No previous windows");
    else {
	topfile = max(topfile - scrlen + 1, 0);
	curfile = topfile;
	curline = 0;
	showscreen(FALSE);
	curxy(CURSOR, 0);
    }
}

/*
 * Blank and redisplay the index.
 * If restat is set, then stat the files again.
 */
showscreen(restat)
int restat;
{
    int	    i,
	    numprint;

    blank();
    home();
    setbuf(stdout, bufout);	/* buffered output here, faster! --fjl */
    numprint = 0;
    for (i = topfile; (numprint < scrlen) && (i < totfiles); i++) {
	if (restat) {
	    tempbuf[0] = '\0';
	    (void) gstat(bldnam(tempbuf, i), i);
	}
	pentry(i);
	numprint++;
	(void) putchar('\n');
    }
    if (splitflg)
    	printf("%s\n", divider);
    (void) fflush(stdout);		/* reset for display functions */
    setbuf(stdout, (char *) 0);
}

	/* Reads directory dir */
Readdir(dir)
char   *dir;
{
    /* Want to cast the last arg to (int ())0 but can`t in C */
    if ((totfiles = Scandir(dir, &file, fileselect, NULL)) < 0) {
	if (totfiles == -1)
	    printf("\n\007Sorry, %s unreadable.\n", dir);
	else				/* -2 */
	    printf("\n\007Out of memory.\n");
	if (recursive)
	    sleep(1);	/* let him see it */
	exit(1);			/* or could be out of room */
    }
}

	/* Stats the given file */
int
gstat(filename, nfile)
char *filename;
int nfile;
{
    register struct lbuf *p = &file[nfile];

    p->flg |= STATDONE;
    p->lmode = 0;
    p->lino = 0;
    p->ltype = '-';

    if (lstat(filename, &statbuf) < 0)
	return -1;
    p->lino = statbuf.st_ino;
    p->lsize = statbuf.st_size;
    switch (statbuf.st_mode & S_IFMT) {
	case S_IFDIR: 
	    p->ltype = 'd';
	    break;
#ifdef S_IFLNK
	case S_IFLNK: {
	    struct stat stbuf;

	    if (stat(filename, &stbuf) == 0 &&
	      (stbuf.st_mode & S_IFMT) == S_IFDIR)
		p->ltype = 'L';
	    else
		p->ltype = 'l';
	    break;
	}
#endif
#ifdef S_IFSOCK
	case S_IFSOCK:
	    p->ltype = 's';
	    break;
#endif	    
	case S_IFBLK: 
	    p->ltype = 'b';
	    p->lsize = statbuf.st_rdev;
	    break;
	case S_IFCHR: 
	    p->ltype = 'c';
	    p->lsize = statbuf.st_rdev;
	    break;
    }
    p->lmode = statbuf.st_mode & ~S_IFMT;
    p->luid = statbuf.st_uid;
    p->lgid = statbuf.st_gid;
    p->lnl = statbuf.st_nlink;
    p->latime = statbuf.st_atime;
    p->lmtime = statbuf.st_mtime;
    return 0;
}

pentry(whichone)
int	whichone;
{
    register struct lbuf *p = &file[whichone];
    register char  *cp;
    time_t tim;
    char buf[MAXPATHLEN];

    if (! (p->flg&STATDONE)) {
	*buf = '\0';
	(void) gstat(bldnam(buf, whichone), whichone);
    }
    nprint = 0;			/* keep track of how many chars printed */
#ifdef notyet
    if (iflg)
	printf("%5u ", p->lino);
    if (sflg)
	printf("%4ld ",
#ifdef xBSD42				/* not yet */
	    stbtok(p->lblks));
#else
	    sztob(p->lsize));
#endif
#endif notyet
    if (lflg) {
	(void) putchar(p->ltype); nprint++;
	pmode(p->lmode);			/* 10 chars */
	printf("%2d ", p->lnl); nprint += 3;	/* XXX */
	if (cp = getname(p->luid))
	    printf("%-9.9s", cp);
	else
	    printf("%-9u", p->luid);
	nprint += 9;
	if (p->ltype == 'b' || p->ltype == 'c')
	    printf("%3d,%3d", major((int) p->lsize), minor((int) p->lsize));
	else
	    printf("%7ld", p->lsize);
	nprint += 7;
	if (sortyp == READ)
	    tim = p->latime;
	else
	    tim = p->lmtime;
	cp = ctime(&tim);
	if (tim < year)
	    printf(" %-7.7s %-4.4s ", cp + 4, cp + 20);
	else
	    printf(" %-12.12s ", cp + 4);
	nprint += 14;
    }
    printf("%c ", p->lino == 0 ? '-' : (p->flg&DELETED ? 'D' : ' '));
    nprint += 2;
    /* 45 chars so far */
    /* TRUE, TRUE ==> prt the "type", expand symlinks */
    pname(p, scrwid - nprint, TRUE, TRUE);
}

int
compar(p1, p2)
register struct lbuf *p1, *p2;
{
    register char *p;
    register long n1, n2;
    static char digits[] = "0123456789";
    extern long atol();
    extern char *skipover();

    switch (sortyp) {
	case NAME:
	    return rflg * strcmp(p1->namep, p2->namep);
	case NUMBER:
	    if (*skipover(p1->namep, digits) != '\0' ||
		*skipover(p2->namep, digits) != '\0')
		return rflg * strcmp(p1->namep, p2->namep);
	    n1 = atol(p1->namep);
	    n2 = atol(p2->namep);
	    return rflg * (n1 - n2);
	case SIZE:
	    return rflg * (p2->lsize - p1->lsize);
	case WRITE:
	    return rflg * (p2->lmtime - p1->lmtime);
	case READ:
	    return rflg * (p2->latime - p1->latime);
	default:
	    telluser("?Syserr - impossible sort order %d\007", sortyp);
	    return 0;
    }
    /*NOTREACHED*/
}

ceod()
{
    putpad(CD);
}

ceol()
{
    putpad(CE);
}

blank()
{
    putpad(CL);
}

home()
{
    if (HO == 0)
	curxy(0, 0);
    else
	putpad(HO);
}

insline()
{
    putpad(AL);
}

/*
   Yes, folks, we use direct cursor addressing to get to next line!
   Before you mumble "What sort of cretin would do this?" here's
   the reason. We don't use \n since that obviously won't work.
   We don't use \012 since virgin version 7 makes that into a crlf.
   We don't use raw mode since we type out help files efficently,
   and we don't want to switch modes all the time. So enjoy. -- SMC
 */
downline()
{
    curxy(CURSOR, ++curline);
    curfile++;
}

upline()
{
    putpad(UP);
    curline--;
    curfile--;
}

/*VARARGS1*/
telluser(msg, args)
char   *msg;
{
    curxy(0, Tscrlen);
    ceol();
    printf(msg, args);
    curxy(CURSOR, curline);
    blurb++;
}

clearmsg()
{
    curxy(0, Tscrlen);
    ceol();
    curxy(CURSOR, curline);
    blurb = 0;
}

curxy(col, lin)
{
    char   *cmstr = tgoto(CM, col, lin);
    putpad(cmstr);
}

short column;
extern char *fgets();

/*
 * Modified to type help file & others. fjl 5/81
 * This function is rather a mess right now.
 */
type(filestr, waitflg)
char *filestr;
{
    int	    helpfd = 5;
    FILE    *fd = stdin;
    register int     i;
    register int     cc = 0;
    int	    cur_scrl;		/* current screen length */
    char    helpbuf[BUFSIZ];
    
    if (splitflg && waitflg)
    	fd = fopen(filestr, "r");
    else
    	helpfd = open(filestr, 0);
    if (helpfd < 0 || fd == NULL) {
    	telluser("?Unable to open %s", filestr);
	return FALSE;
    }
    
    (void) signal(SIGINT, catchint);
    sigint = 0;
    
    if (splitflg && waitflg) {
	setbuf(stdout, bufout);		/* to speed it up */
	cur_scrl = totfiles - topfile;	/* topfile starts at 0 */
	Worktop = ((cur_scrl < scrlen) ? cur_scrl : scrlen) + 1;
	do {
	    curxy(0, Worktop);
	    ceod();
	    for (i = Worktop; (i < Tscrlen) && !sigint && (cc != EOF); i++) {
		column = 0;
		while ((cc = getc(fd)) != EOF) {
		    if (typec(cc, fd))
		        break;
	        }
	    }
	    (void) fflush(stdout);
	} while (!sigint && (cc != EOF) && waitchk(waitflg));
	
	if (feof(fd))	
	    printf("===== End-of-File =====\n");
	(void) fflush(stdout);
	setbuf(stdout, (char *) 0);
	(void) fclose(fd);
	return FALSE;		/* means needs no re-display */
    }
    else {			/* full screen typeout */
    	blank();
	unsetdpy();		/* so page mode works if we've got it */
	stopdefault();			/* ??? */
    	while ((i = read (helpfd, helpbuf, sizeof helpbuf)) > 0 && !sigint)
	    (void) write(1, helpbuf, i);
	(void) close(helpfd);
	stopcatch();
	setdpy();
    	if (!sigint) {
	    curxy(0, Tscrlen);
	    printf("CR to return...");
    	    (void) getchar();
    	}
	return TRUE;
    }
}

/* Used by "type" to put out a char in other window, doing line-folding,
 * tab expansion, ^L interpreting, etc.  Returns non-0 iff put out a \n
 * during it.
 */
typec(cc, fd)
register int cc;
FILE *fd;
{
	register int i;
	register int newline = 0;
chklen:
	if (cc == '\t') {
	    if (!hardtabs) {
		for (i = 7; i; i--)
		    newline += typec(' ', fd);
		cc = ' ';
	    }
	    else			/* 8 should not be hardwired here */
		column = (column + 8) & ~07;  /* round down to multiple of 8 */
	}
	/* If am then can''t use whole line cause it will scroll */
	if (++column > (scrwid + (!autowrap))) {
	    if (cc != '\t' && cc != '\n')
		(void) ungetc(cc, fd);
	    cc = '\n';
	}
	if (cc == CTRL('l')) {
	    (void) putchar('^');
	    cc = 'L';
	    goto chklen;
	}
	if (putchar(cc) == '\n')
	    newline++;
	return newline;
}
	    
waitchk(waitflg)
{
	register int c;

	if (!waitflg)
	    return 1;
	(void) fflush(stdout);
	setbuf(stdout, (char *) 0);
prompt:
	curxy(0, Tscrlen);
	printf ("---Continue---");
	ceol();
	curxy(0, Tscrlen);
	/* need to handle syscall restarting */
	if ((c = getchar()) == 'q')
	    sigint = 1;		/* simulate interrupt */
#ifdef SIGTSTP
	else if (c == oltc.t_suspc) {
	    onstop();
	    goto prompt;
	}
#endif
	ceol();
	setbuf(stdout, bufout);
	if (sigint)
	    return 0;		/* avoids clear of screen */
	return 1;
}

setdpy()
{
    static int dpyinit;
#ifdef USG
    static struct termio newb;
#else    
    static struct sgttyb newb;
#endif USG
#ifdef TIOCGLTC		/* this better be defined if SIGTSTP is */
    static struct ltchars nltc;

    if (!dpyinit) {
	(void) ioctl(0, TIOCGLTC, (char *) &oltc);
	nltc = oltc;
	nltc.t_suspc = '\377';		/* ^Z */
	nltc.t_dsuspc = '\377';		/* ^Y */
	nltc.t_lnextc = '\377';		/* ^V */
    }
    (void) ioctl(0, TIOCSLTC, (char *) &nltc);
#endif TIOCGLTC

#ifdef USG
    if (!dpyinit) {
	dpyinit++;
	(void) ioctl(0, TCGETA, (char *) &ioctlb);
	newb = ioctlb;
	newb.c_lflag &= ~ICANON;
	newb.c_lflag &= ~ECHO;
	newb.c_cc[VMIN] = 1;
	newb.c_cc[VTIME] = 1;
    }
    (void) ioctl(0, TCSETA, (char *) &newb);

#else
    if (!dpyinit) {
	dpyinit++;
	(void) ioctl(0, TIOCGETP, (char *) &ioctlb);
	newb = ioctlb;
	newb.sg_flags |= CBREAK;
	newb.sg_flags &= ~(ECHO|XTABS);
    }
    (void) ioctl(0, TIOCSETP, (char *) &newb);
# ifdef LPASS8
    (void) ioctl(0, TIOCLBIS, (char *) &lpass8);
# endif
#endif USG

    dpyflg = TRUE;
    putpad(VS);
}

unsetdpy()
{
#ifdef USG
    (void) ioctl(0, TCSETA, (char *) &ioctlb);
#else
    (void) ioctl(0, TIOCSETP, (char *) &ioctlb);
#endif USG
#ifdef TIOCGLTC
    (void) ioctl(0, TIOCSLTC, (char *) &oltc);
#endif
# ifdef LPASS8
    (void) ioctl(0, TIOCLBIC, (char *) &lpass8);
# endif
    dpyflg = FALSE;
    putpad(VE);
}

getcap()
{
    static char tcapbuf[256];
    char tbuf[1024];
    char   *ap;
    char   *term;
    char   *xPC;

    term = getenv("TERM");
    if (term == 0) {
	fprintf(stderr, "No TERM in environment\n");
	exit(1);
    }

    switch (tgetent(tbuf, term)) {
	case -1: 
	    fprintf(stderr, "Cannot open termcap file\n");
	    exit(2);
	case 0: 
	    fprintf(stderr, "%s: unknown terminal", term);
	    exit(3);
    }

    ap = tcapbuf;

    Tscrlen = tgetnum("li") - 1;
    scrwid = tgetnum("co") - 1;	    /* lose 1 so won`t scroll in last line */

    UP = tgetstr("up", &ap);
    DO = tgetstr("do", &ap);
    CD = tgetstr("cd", &ap);
    CE = tgetstr("ce", &ap);
    HO = tgetstr("ho", &ap);		/* home, optional */
    CL = tgetstr("cl", &ap);
    CM = tgetstr("cm", &ap);
    AL = tgetstr("al", &ap);		/* insert line, optional */
    VS = tgetstr("vs", &ap);		/* visual start */
    VE = tgetstr("ve", &ap);
    autowrap = tgetflag("am");		/* use for smarts in "type" */
    hardtabs = tgetflag("pt");		/* same */

    xPC = tgetstr("pc", &ap);
    if (xPC)
	PC = *xPC;

    if ((CM == 0) || (CL == 0) || (UP == 0)) {
	fprintf(stderr, "Tty must have cursor addr, clear, and 4 cursor motions.\n");
	exit(1);
    }
    if (Tscrlen <= 0 || scrwid <= 0) {
	fprintf(stderr, "Must know the screen size\n");
	exit(1);
    }
}

catchint()
{
	(void) signal(SIGINT, SIG_IGN);	/* reset it */
	sigint = 1;
}

/*VARARGS2*/
char *
bldnam(str, filidx, lp)
register char *str;
int filidx;
struct lbuf *lp;
{
	if (thisdir) {
		(void) strcat(str, thisdir);
		(void) strcat(str, "/");
	}
	if (filidx < 0)			/* XXX */
	    (void) strcat(str, lp->namep);
	else
	    (void) strcat(str, file[filidx].namep);
	return str;
}

promptwin(str, otherwin)
char *str;
{
    if (otherwin) {		/* do in lower window */
	int cur_scrl = totfiles - topfile;
	Worktop = ((cur_scrl < scrlen) ? cur_scrl : scrlen) +1;
	curxy(0, Worktop);
	ceod();
    }
    else {
	curxy(0, Tscrlen);
	ceol();
	blurb++;
    }
    unsetdpy();
    printf(str);
}
		
/* 
 * fix up display after numout lines wrtten in cooked mode.
 *
 * Since we were in cooked mode, the CR ending the gets is echoed and
 * we lost the first line of the display.  So we re-insert it.
 */
fixup(otherwin, numout)
{
    setdpy();
    if (!otherwin) {
	if (AL == 0 || numout > 1)	/* no insert line capability */
	    showscreen(FALSE);
	else {				/* be a little sneakier */
	    curxy(0, Tscrlen-1);	/* go back to where prompt */
	    ceol();		  	/*  is now, and blank it */
	    home();
	    insline();			/* make some room */
	    pentry(topfile);
	    (void) putchar('\n');
	}
    }
    curxy(CURSOR, curline);
}		

#ifdef SIGTSTP
onstop()
{
    curxy(0, Tscrlen - 2);	/* so top line stays at top when stopped */
    ceod();
    unsetdpy();
    stopdefault();
    (void) kill(0, SIGTSTP);		/* hold it right there! */
    stopcatch();
    setdpy();
    showscreen(FALSE);		/* false ==> don`t restat.  Arguable. */
    curxy(CURSOR, curline);
}

stopdefault()
{
    (void) signal(SIGTSTP, SIG_DFL);
#ifdef BSD42
    (void) sigsetmask(sigblock(0) &~ mask(SIGTSTP));
#endif
}

stopcatch()
{
    (void) signal(SIGTSTP, onstop);
}
#endif SIGTSTP

static int
fileselect(dp)
    struct direct *dp;
{
    register char *p = dp->d_name;
    register int len;
    register char *q, *sq;
    char buf[56];

    if (*p == '.') {
        switch (*(p+1)) {
	  case '\0':			/* "." */
	    return 0;
	  case '.':
	    if (*(p+2) == '\0')		/* ".." */
	        return 0;
	}
    }
    len = dp->d_namlen;
    /* This is written so that a null suffixes string will always return 1 */
    p = p + len;			/* p now points at ending null */
    (void) strcpy(buf, xsuff);
    sq = buf;
    (void) strcat(sq, "|");
    for (q = sq; *q; q++)
	if (*q == '|') {
	    if ((len = q - sq) && !strncmp(p - len, sq, len))
		return 0;
	    sq = q + 1;			/* skip over the | */
	}
    return 1;
}

/* stat's all the files if it hasn't been done yet */
statall()
{
    register int i;
    register struct lbuf *lp;
    struct lbuf *endbuf = file + totfiles;	/* one past end */
    char buf[MAXPATHLEN];

    if (allstatflg)
	return;
    for (lp = file; lp < endbuf; lp++) {
	if (!lp->flg&STATDONE) {
	    i = lp - file;
	    *buf = '\0';
	    (void) gstat(bldnam(buf, i), i);
	}
    }
    allstatflg++;
}

/* The following routines are more or less from Charles Hill, philabs!crh */

/* move to the indicated file number */
displayfile(fnum)
int fnum;
{
 
  /* redraw if file not already on screen */
  if ( (fnum < topfile) || (fnum >= topfile + scrlen) ) {
     topfile = fnum/(scrlen - 1) * (scrlen - 1);
     /* get rid of 1 file final windows unless necessary */
     if ( (topfile == totfiles - 1) && (topfile - scrlen + 1 >= 0) )
       topfile = topfile - scrlen + 1;
     showscreen(FALSE);
  }
  curfile = fnum;
  curline = fnum - topfile;
  curxy(CURSOR, curline);
}
