/*
 * mon.c
 *
 *
 *      Profiling stuff for the decStation.
 *
 *
 */

#include "gmon.h"

extern void monitor();
extern void moncontrol();


/*
 *----------------------------------------------------------------------
 *
 * _mcount --
 *
 *	This is supposed to record each procedure call, so that gprof
 *      can print a call graph.  But it is just a null procedure because
 *      the mips compiler doesn't use a frame pointer.
 *
 *      Once we switch over to gcc, we will have a frame pointer, and
 *      we will be able to look at the stack and find out who the caller
 *      was.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
_mcount()
{

    return;
}



    /*
     *	froms is actually a bunch of unsigned shorts indexing tos
     */
extern char		profiling;

unsigned short	*froms;
struct tostruct	*tos = 0;
long		tolimit = 0;
char		*s_lowpc = 0;
char		*s_highpc = 0;
unsigned long	s_textsize = 0;
int		s_scale;
char		*minsbrk = 0;

static int	ssiz;
static int	*sbuf;

#define	MSG "No space for monitor buffer(s)\n"

monstartup(lowpc, highpc)
    char	*lowpc;
    char	*highpc;
{
    int			monsize;
    char		*buffer;
    char		*sbrk();

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */
    lowpc = (char *)
	    ROUNDDOWN((unsigned)lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_lowpc = lowpc;
    highpc = (char *)
	    ROUNDUP((unsigned)highpc, HISTFRACTION*sizeof(HISTCOUNTER));
    s_highpc = highpc;
    s_textsize = highpc - lowpc;
    monsize = (s_textsize / HISTFRACTION) + sizeof(struct phdr);
    buffer = sbrk( monsize );
    if ( buffer == (char *) -1 ) {
	write( 2 , MSG , sizeof(MSG) );
	return;
    }
    froms = (unsigned short *) sbrk( s_textsize / HASHFRACTION );
    if ( froms == (unsigned short *) -1 ) {
	write( 2 , MSG , sizeof(MSG) );
	froms = 0;
	return;
    }
    tolimit = s_textsize * ARCDENSITY / 100;
    if ( tolimit < MINARCS ) {
	tolimit = MINARCS;
    } else if ( tolimit > 65534 ) {
	tolimit = 65534;
    }
    tos = (struct tostruct *) sbrk( tolimit * sizeof( struct tostruct ) );
    if ( tos == (struct tostruct *) -1 ) {
	write( 2 , MSG , sizeof(MSG) );
	froms = 0;
	tos = 0;
	return;
    }
    minsbrk = sbrk(0);
    tos[0].link = 0;
    monitor( lowpc , highpc , buffer , monsize , tolimit );
}

_mcleanup()
{
    int			fd;
    int			fromindex;
    int			endfrom;
    char		*frompc;
    int			toindex;
    struct rawarc	rawarc;

    moncontrol(0);
    fd = creat( "gmon.out" , 0666 );
    if ( fd < 0 ) {
	perror( "mcount: gmon.out" );
	return;
    }
#   ifdef DEBUG
	fprintf( stderr , "[mcleanup] sbuf 0x%x ssiz %d\n" , sbuf , ssiz );
#   endif DEBUG
    write( fd , sbuf , ssiz );
    endfrom = s_textsize / (HASHFRACTION * sizeof(*froms));
    for ( fromindex = 0 ; fromindex < endfrom ; fromindex++ ) {
	if ( froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = s_lowpc + (fromindex * HASHFRACTION * sizeof(*froms));
	for (toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
#	    ifdef DEBUG
		fprintf( stderr ,
			"[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			frompc , tos[toindex].selfpc , tos[toindex].count );
#	    endif DEBUG
	    rawarc.raw_frompc = (unsigned long) frompc;
	    rawarc.raw_selfpc = (unsigned long) tos[toindex].selfpc;
	    rawarc.raw_count = tos[toindex].count;
	    write( fd , &rawarc , sizeof rawarc );
	}
    }
    close( fd );
}

/*VARARGS1*/
void
monitor( lowpc , highpc , buf , bufsiz , nfunc )
    char	*lowpc;
    char	*highpc;
    int		*buf, bufsiz;
    int		nfunc;	/* not used, available for compatability only */
{
    register o;

    if ( lowpc == 0 ) {
	moncontrol(0);
	_mcleanup();
	return;
    }
    sbuf = buf;
    ssiz = bufsiz;
    ( (struct phdr *) buf ) -> lpc = lowpc;
    ( (struct phdr *) buf ) -> hpc = highpc;
    ( (struct phdr *) buf ) -> ncnt = ssiz;
    o = sizeof(struct phdr);
    buf = (int *) ( ( (int) buf ) + o );
    bufsiz -= o;
    if ( bufsiz <= 0 )
	return;
    o = ( ( (char *) highpc - (char *) lowpc) );
    if( bufsiz < o )
	s_scale = ( (float) bufsiz / o ) * 65536;
    else
	s_scale = 65536;
    moncontrol(1);
}

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
void
moncontrol(mode)
    int mode;
{
    if (mode) {
	/* start */
	profil((char*) sbuf + sizeof(struct phdr), ssiz - sizeof(struct phdr),
		s_lowpc, s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
    return;
}

