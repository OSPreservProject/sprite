/* $Header: ded.h,v 1.2 84/04/24 22:51:47 lepreau Exp $ */

#ifdef USG
# define index strchr
#endif

#ifndef S_IFLNK
# define lstat stat
#endif

#ifdef pdp11				/* should work */
# define void int
#endif

#define	FALSE		0
#define	TRUE		1

#define max(a,b)	((a) > (b) ? (a):(b))
#define min(a,b)	((a) < (b) ? (a):(b))

#define	ALLOC(type, n)	    (type *) malloc((unsigned) ((n) * sizeof(type)))
#define	RALLOC(type, p, n)  (type *) realloc((char *) p, (unsigned) ((n) * sizeof(type)))
extern char *malloc(), *realloc();

struct lbuf {
    char    *namep;			/* filename */
#ifdef S_IFLNK
    struct target {
	int   targlen;
	char  targname[1];		/* actually variable length */
    } *linkto;				/* target of symlink */
#define linklen(p)  (*p->linkto).targlen
#define linkname(p) (*p->linkto).targname
#endif
    char    flg;			/* local flags */
    char    ltype;			/* printable type of file */
    short   namlen;			/* strlen(namep) */
    short   lino;			/* inode */
    short   lmode;			/* st_mode&S_IFMT */
    short   lnl;			/* number of links */
    unsigned short   luid;
    unsigned short   lgid;
    long    lsize;
    time_t  latime;			/* accessed */
    time_t  lmtime;			/* modified */
};

/* lbuf.flg */
#define STATDONE    001			/* we have stat`ed this file */
#define DELETED	    002			/* marked for deletion */
#define RDLINKDONE  004			/* we have done the readlink() */
