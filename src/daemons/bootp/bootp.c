/*
 * bootp.c --
 *
 */

#ifndef lint
static char sccsid[] = "@(#)bootp.c	1.1 (Stanford) 1/22/86";
#endif

/*
 * BOOTP (bootstrap protocol) server daemon.
 *
 * Answers BOOTP request packets from booting client machines.
 * See [SRI-NIC]<RFC>RFC951.TXT for a description of the protocol.
 */

/*
 * history
 * 01/22/86	Croft	created.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <net/if.h>
#include <netinet/in.h>
#define	iaddr_t struct in_addr
#include "bootp.h"

#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <varargs.h>
#include <time.h>

static int	debug;
extern	int errno;
static struct	sockaddr_in sin = { AF_INET };
static int	s;		/* socket fd */
static struct	sockaddr_in from;
static int	fromlen;
static u_char	buf[1024];	/* receive packet buffer */
extern long	time();	        /* time of day */
static struct	ifreq ifreq[10]; /* holds interface configuration */
static struct	ifconf ifconf;	/* int. config. ioctl block (points to ifreq) */
static struct	arpreq arpreq;	/* arp request ioctl block */

/*
 * Globals below are associated with the bootp database file (bootptab).
 */

static char	*bootptab = "/etc/spritehosts";
static char	*bootplog = "/sprite/admin/bootplog";
static FILE	*fp;
static int	f;
static char	line[256];	/* line buffer for reading bootptab */
static char	*linep;		/* pointer to 'line' */
static int	linenum;	/* current ilne number in bootptab */

/* bootfile homedirectory */
static char	homedir[] = "/sprite/boot";
static char	defaultboot[] = "ds3100"; /* default file to boot */

#define	MHOSTS	512	/* max number of 'hosts' structs */

static struct hosts {
	char	host[31];	/* host name (and suffix) */
	u_char	htype;		/* hardware type */
	u_char	haddr[6];	/* hardware address */
	iaddr_t	iaddr;		/* internet address */
	char	bootfile[32];	/* default boot file name */
} hosts[MHOSTS];

static int	nhosts;		/* current number of hosts */
static long	modtime;	/* last modification time of bootptab */

static struct in_addr myAddr;

static void log();
static void request();
static void reply();
static void sendreply();
static int nmatch();
static void setarp();
static void getfield();
static void readtab();

unsigned char ginger[4] = {128,32,150,28};

void
main(argc, argv)
	char *argv[];
{
	register struct bootp *bp;
	register int n;
	char hostname[100];
	struct hostent *hostentPtr;

	if (gethostname(hostname, 100) < 0) {
	    perror("gethostname");
	    exit(1);
	}

	hostentPtr = gethostbyname(hostname);
	if (hostentPtr == (struct hostent *)NULL) {
	    perror("gethostbyname");
	    exit(2);
	}
	myAddr = *(struct in_addr *)hostentPtr->h_addr_list[0];
	{
	    unsigned char *addrPtr;

	    addrPtr = (unsigned char *)&myAddr;
	    log("My name and addr: %s %d:%d:%d:%d\n", hostname,
			    addrPtr[0], addrPtr[1], addrPtr[2], addrPtr[3]);
	}

	for (argc--, argv++ ; argc > 0 ; argc--, argv++) {
		if (argv[0][0] == '-') {
			switch (argv[0][1]) {
			case 'd':
				debug++;
				break;
			}
		}
	}
	
	if (debug == 0) {
		int t;
		if (fork())
			exit(0);
		for (f = 0; f < 10; f++)
			(void) close(f);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		t = open("/dev/tty", 2);	
		if (t >= 0) {
			ioctl(t, TIOCNOTTY, (char *)0);
			(void) close(t);
		}
	}

	log("BOOTP server starting up.");

reopenSocket:

	while ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log("socket call failed");
		sleep(5);
	}
#ifdef notdef
	ifconf.ifc_len = sizeof ifreq;
	ifconf.ifc_req = ifreq;
	if (ioctl(s, SIOCGIFCONF, (caddr_t)&ifconf) < 0
	    || ifconf.ifc_len <= 0) {
		log("'get interface config' ioctl failed");
		exit(1);
	}
#endif
	sin.sin_port = htons(IPPORT_BOOTPS);
	while (bind(s, (caddr_t)&sin, sizeof (sin), 0) < 0) {
		perror("bind");
		log("bind call failed");
		sleep(5);
	}
	for (;;) {
		fromlen = sizeof (from);
		n = recvfrom(s, buf, sizeof buf, 0, (caddr_t)&from, &fromlen);
		if (n <= 0) {
		    log("recvfrom failed: %s\n", strerror(errno));
		    if (errno==ESTALE || errno==EIO) {
			close(s);
			log("restarting socket");
			goto reopenSocket;
		    }
		    sleep(10);
		    continue;
		}
		bp = (struct bootp *) buf;
		if (n < sizeof *bp) {
			continue;
		}
		readtab();	/* (re)read bootptab */
		switch (bp->bp_op) {
		case BOOTREQUEST:
			request();
			break;

		case BOOTREPLY:
			reply();
			break;
		}
	}
}


/*
 * Process BOOTREQUEST packet.
 *
 * (Note, this version of the bootp.c server never forwards 
 * the request to another server.  In our environment the 
 * stand-alone gateways perform that function.)
 *
 * (Also this version does not interpret the hostname field of
 * the request packet;  it COULD do a name->address lookup and
 * forward the request there.)
 */
static void
request()
{
	register struct bootp *rq = (struct bootp *)buf;
	struct bootp rp;
	char *strPtr;
	char path[64], file[64];
	register struct hosts *hp;
	register n;

	rp = *rq;	/* copy request into reply */
	rp.bp_op = BOOTREPLY;
	if (rq->bp_ciaddr.s_addr == 0) { 
		/*
		 * client doesnt know his IP address, 
		 * search by hardware address.
		 */
		for (hp = &hosts[0], n = 0 ; n < nhosts ; n++,hp++)
			if (rq->bp_htype == hp->htype
			   && bcmp(rq->bp_chaddr, hp->haddr, 6) == 0)
				break;
		if (n == nhosts)
			return;	/* not found */
		rp.bp_yiaddr = hp->iaddr;
	} else {
		/* search by IP address */
		for (hp = &hosts[0], n = 0 ; n < nhosts ; n++,hp++)
			if (rq->bp_ciaddr.s_addr == hp->iaddr.s_addr)
				break;
		if (n == nhosts)
			return;
	}
	if (strcmp(rq->bp_file, "sunboot14") == 0)
		rq->bp_file[0] = 0;	/* pretend it's null */
	strPtr = strchr(rq->bp_file,' ');
	if (strPtr != (char *)NULL) {
	    *strPtr = 0;
	}
	log("request from %s for '%s'", hp->host, rq->bp_file);
	strcpy(path, homedir);
	strcat(path, "/");
	strcat(path, hp->bootfile);
	strcat(path, ".md/");
	if (rq->bp_file[0] == 0) { /* if client didnt specify file */
		if (hp->bootfile[0] == 0)
			strcpy(file, defaultboot);
		else
			strcpy(file, hp->bootfile);
	} else {
		/* client did specify file */

	    /*
	     * Test for boot from ginger.
	     */
	    if (!strcmp(rq->bp_file,"/xyzzy")) {
		strcpy(rp.bp_file,"/tmp/bar");
		/*
		strcpy(rp.bp_file,"/home/ginger/sprite/kernels/ds3100.1.075");
		*/
		bcopy(ginger, &rp.bp_siaddr,4);
		log("Ginger boot %s",rp.bp_file);
		sendreply(&rp, 1);
		return;
	    }
	    /*
	     * For now the ds5000 always returns an extra '/' at the beginning
	     * of the path name. 
	     */
	     if (!strcmp(hp->bootfile, "ds5000")) {
		 strcpy(file, &rq->bp_file[1]);
	     } else {
		strcpy(file, rq->bp_file);
	     }
	}
	if (file[0] == '/')	/* if absolute pathname */
		strcpy(path, file);
	else
		strcat(path, file);
	/* try first to find the file with a ".host" suffix */
	n = strlen(path);
	strcat(path, ".");
	strcat(path, hp->host);
	if (access(path, R_OK) < 0) {
		path[n] = 0;	/* try it without the suffix */
		if (access(path, R_OK) < 0) {
			if (rq->bp_file[0])  /* client wanted specific file */
				return;		/* and we didnt have it */
			log("boot file %s* missing?", path);
		}
	}
	log("replyfile %s", path);
	strcpy(rp.bp_file, path);
	sendreply(&rp, 0);
	return;
}


/*
 * Process BOOTREPLY packet (something is using us as a gateway).
 */
static void
reply()
{
	struct bootp *bp = (struct bootp *)buf;

	sendreply(bp, 1);
	return;
}


/*
 * Send a reply packet to the client.  'forward' flag is set if we are
 * not the originator of this reply packet.
 */
static void
sendreply(bp, forward)
	register struct bootp *bp;
{
	iaddr_t dst;
	struct sockaddr_in to;

	to = sin;
	to.sin_port = htons(IPPORT_BOOTPC);
	/*
	 * If the client IP address is specified, use that
	 * else if gateway IP address is specified, use that
	 * else make a temporary arp cache entry for the client's NEW 
	 * IP/hardware address and use that.
	 */
	if (bp->bp_ciaddr.s_addr) {
		dst = bp->bp_ciaddr;
		if (debug) log("reply ciaddr");
	} else if (bp->bp_giaddr.s_addr && forward == 0) {
		dst = bp->bp_giaddr;
		to.sin_port = htons(IPPORT_BOOTPS);
		if (debug) log("reply giaddr");
	} else {
		dst = bp->bp_yiaddr;
		if (debug) log("reply yiaddr %x", dst.s_addr);
		setarp(&dst, bp->bp_chaddr, bp->bp_hlen);
	}

	if (forward == 0) {
		/*
		 * If we are originating this reply, we
		 * need to find our own interface address to
		 * put in the bp_siaddr field of the reply.
		 * If this server is multi-homed, pick the
		 * 'best' interface (the one on the same net
		 * as the client).
		 */
		int maxmatch = 0;
		int len, m;
		register struct ifreq *ifrp, *ifrmax;

#ifdef notdef
		ifrmax = ifrp = &ifreq[0];
		len = ifconf.ifc_len;
		for ( ; len > 0 ; len -= sizeof ifreq[0], ifrp++) {
			if ((m = nmatch((caddr_t)&dst,
			    (caddr_t)&((struct sockaddr_in *)
			     (&ifrp->ifr_addr))->sin_addr)) > maxmatch) {
				maxmatch = m;
				ifrmax = ifrp;
			}
		}
		if (bp->bp_giaddr.s_addr == 0) {
			if (maxmatch == 0) {
				log("missing gateway address");
				return;
			}
			bp->bp_giaddr = ((struct sockaddr_in *)
				(&ifrmax->ifr_addr))->sin_addr;
		}
#endif
		bp->bp_siaddr = myAddr;
	}
	to.sin_addr = dst;
	if (sendto(s, (caddr_t)bp, sizeof *bp, 0, &to, sizeof to) < 0)
		log("send failed");
	return;
}


/*
 * Return the number of leading bytes matching in the
 * internet addresses supplied.
 */
static int
nmatch(ca,cb)
	register char *ca, *cb;
{
	register n,m;

	for (m = n = 0 ; n < 4 ; n++) {
		if (*ca++ != *cb++)
			return(m);
		m++;
	}
	return(m);
}


/*
 * Setup the arp cache so that IP address 'ia' will be temporarily
 * bound to hardware address 'ha' of length 'len'.
 */
static void
setarp(ia, ha, len)
	iaddr_t *ia;
	u_char *ha;
{
#ifdef notdef
	struct sockaddr_in *si;

	arpreq.arp_pa.sa_family = AF_INET;
	si = (struct sockaddr_in *)&arpreq.arp_pa;
	si->sin_addr = *ia;
	bcopy(ha, arpreq.arp_ha.sa_data, len);
	if (ioctl(s, SIOCSARP, (caddr_t)&arpreq) < 0)
		log("set arp ioctl failed");
#endif
	return;
}

static void
readtab()
{
    struct stat st;
    register char *cp;
    int v;
    register i;
    char temp[64], tempcpy[64];
    register struct hosts *hp;
    char spriteID[8];
    char netType[20];

    if (fp == NULL) {
	if ((fp = fopen(bootptab, "r")) == NULL) {
            log("can't open %s", bootptab);
	    exit(1);
	}
    }
    fstat(fileno(fp), &st);
    if (st.st_mtime == modtime && st.st_nlink) {
	return;	/* hasnt been modified or deleted yet */
    }
    fclose(fp);
    if ((fp = fopen(bootptab, "r")) == NULL) {
	log("can't open %s", bootptab);
	exit(1);
    }
    fstat(fileno(fp), &st);
    log("(re)reading %s", bootptab);
    modtime = st.st_mtime;
    nhosts = 0;
    hp = &hosts[0];
    linenum = 0;

    /*
     * read and parse each line in the file.
     */
    for (;;) {
	if (fgets(line, sizeof line, fp) == NULL) {
	    break;	/* done */
	}
	if ((i = strlen(line))) {
	    line[i-1] = 0;	/* remove trailing newline */
	}
	linep = line;
	linenum++;
	/* skip leading whitespace */
	while (isspace(*linep)) {
	    ++linep;
	}
	if (*linep == '#' || *linep == '\0') {
	    continue;	/* skip comment lines */
	}
	/* fill in host table */
	/* get spriteid */
	getfield(spriteID, sizeof(spriteID));
	if (!isdigit(*spriteID)) {
	    log("bad sprite ID at line %d of %s", linenum, bootptab);
	    exit(1);
	}
	getfield(netType, sizeof(netType));
	if (debug && strcmp(netType, "ether") && strcmp(netType, "inet")) {
	    log("unrecognized network type: %s, line %d, %s\n",
		netType, linenum, bootptab);
	}
	hp->htype = 1;
	getfield(temp, sizeof temp);
	strcpy(tempcpy, temp);
	cp = tempcpy;
	/* parse hardware address */
	for (i = 0 ; i < sizeof hp->haddr ; i++) {
	    char *cpold;
	    char c;
	    cpold = cp;
	    while (*cp != '.' && *cp != ':' && *cp != 0) {
		cp++;
	    }
	    c = *cp;	/* save original terminator */
	    *cp = 0;
	    cp++;
	    if (sscanf(cpold, "%x", &v) != 1) {
		goto badhex;
	    }
	    hp->haddr[i] = v;
	    if (c == 0) {
		break;
	    }
	}
	if (i != 5) {
badhex:     log("bad hex address: %s, at line %d of bootptab", temp, linenum);
            continue;
	}
	getfield(temp, sizeof temp);
	if ((i = inet_addr(temp)) == -1 || i == 0) {
	    log("bad internet address: %s, at line %d of bootptab",
		temp, linenum);
	    continue;
	}
	hp->iaddr.s_addr = i;
	getfield(hp->bootfile, sizeof hp->bootfile);
	if (debug &&
	    strcmp(hp->bootfile, "ds3100") && strcmp(hp->bootfile, "sun3") &&
	    strcmp(hp->bootfile, "sun4") && strcmp(hp->bootfile, "sun4c") &&
	    strcmp(hp->bootfile, "spur") && strcmp(hp->bootfile, "ds5000")) {
	      log("unrecognized machine type: %s, line %d, %s\n",
		  hp->bootfile, linenum, bootptab);
	}
	if ((strcmp(hp->bootfile, "ds3100") != 0) &&
	    (strcmp(hp->bootfile, "ds5000") != 0)) {
	    /* bootp is only used for decStations */
	    continue;
	}
	getfield(hp->host, sizeof hp->host);
	if (++nhosts >= MHOSTS) {
	    log("'hosts' table length exceeded");
	    exit(1);
	}
	hp++;
    }
    return;
}


/*
 * Get next field from 'line' buffer into 'str'.  'linep' is the 
 * pointer to current position.
 */
static void
getfield(str, len)
	char *str;
{
    register char *cp = str;

    for (; *linep && (*linep == ' ' || *linep == '\t') ; linep++) {
	continue;   /* skip spaces/tabs */
    }
    if (*linep == '\0') {
	*cp = '\0';
	return;
    }
    len--;  /* save a spot for a null */
    for (; *linep && *linep != ' ' & *linep != '\t' ; linep++) {
	*cp++ = *linep;
	if (--len <= 0) {
	    *cp = '\0';
	    log("string truncated: %s, on line %d of bootptab",	str, linenum);
	    return;
	}
    }
    *cp = '\0';
    return;
}

#if 0
/*
 * Read bootptab database file.  Avoid rereading the file if the
 * write date hasnt changed since the last time we read it.
 */
static void
readtab()
{
	struct stat st;
	register char *cp;
	int v;
	register i;
	char temp[64], tempcpy[64];
	register struct hosts *hp;
	int skiptopercent;

	if (fp == 0) {
		if ((fp = fopen(bootptab, "r")) == NULL) {
			log("can't open %s", bootptab);
			exit(1);
		}
	}
	fstat(fileno(fp), &st);
	if (st.st_mtime == modtime && st.st_nlink)
		return;	/* hasnt been modified or deleted yet */
	fclose(fp);
	if ((fp = fopen(bootptab, "r")) == NULL) {
		log("can't open %s", bootptab);
		exit(1);
	}
	fstat(fileno(fp), &st);
	log("(re)reading %s", bootptab);
	modtime = st.st_mtime;
	homedir[0] = defaultboot[0] = 0;
	nhosts = 0;
	hp = &hosts[0];
	linenum = 0;
	skiptopercent = 1;

	/*
	 * read and parse each line in the file.
	 */
	for (;;) {
		if (fgets(line, sizeof line, fp) == NULL)
			break;	/* done */
		if ((i = strlen(line)))
			line[i-1] = 0;	/* remove trailing newline */
		linep = line;
		linenum++;
		if (line[0] == '#' || line[0] == 0 || line[0] == ' ')
			continue;	/* skip comment lines */
		/* fill in fixed leading fields */
		if (homedir[0] == 0) {
			getfield(homedir, sizeof homedir);
			continue;
		}
		if (defaultboot[0] == 0) {
			getfield(defaultboot, sizeof defaultboot);
			continue;
		}
		if (skiptopercent) {	/* allow for future leading fields */
			if (line[0] != '%')
				continue;
			skiptopercent = 0;
			continue;
		}
		/* fill in host table */
		getfield(hp->host, sizeof hp->host);
		getfield(temp, sizeof temp);
		sscanf(temp, "%d", &v);
		hp->htype = v;
		getfield(temp, sizeof temp);
		strcpy(tempcpy, temp);
		cp = tempcpy;
		/* parse hardware address */
		for (i = 0 ; i < sizeof hp->haddr ; i++) {
			char *cpold;
			char c;
			cpold = cp;
			while (*cp != '.' && *cp != ':' && *cp != 0)
				cp++;
			c = *cp;	/* save original terminator */
			*cp = 0;
			cp++;
			if (sscanf(cpold, "%x", &v) != 1)
				goto badhex;
			hp->haddr[i] = v;
			if (c == 0)
				break;
		}
		if (hp->htype == 1 && i != 5) {
	badhex:		log("bad hex address: %s, at line %d of bootptab",
				temp, linenum);
			continue;
		}
		getfield(temp, sizeof temp);
		if ((i = inet_addr(temp)) == -1 || i == 0) {
			log("bad internet address: %s, at line %d of bootptab",
				temp, linenum);
			continue;
		}
		hp->iaddr.s_addr = i;
		getfield(hp->bootfile, sizeof hp->bootfile);
		if (++nhosts >= MHOSTS) {
			log("'hosts' table length exceeded");
			exit(1);
		}
		hp++;
	}
	return;
}


/*
 * Get next field from 'line' buffer into 'str'.  'linep' is the 
 * pointer to current position.
 */
static void
getfield(str, len)
	char *str;
{
	register char *cp = str;

	for ( ; *linep && (*linep == ' ' || *linep == '\t') ; linep++)
		;	/* skip spaces/tabs */
	if (*linep == 0) {
		*cp = 0;
		return;
	}
	len--;	/* save a spot for a null */
	for ( ; *linep && *linep != ' ' & *linep != '\t' ; linep++) {
		*cp++ = *linep;
		if (--len <= 0) {
			*cp = 0;
			log("string truncated: %s, on line %d of bootptab",
				str, linenum);
			return;
		}
	}
	*cp = 0;
	return;
}
#endif

/*
 * log an error message 
 *
 */
static void
log(va_alist)
    va_dcl
{
	FILE *fp;
	char *format;
	va_list args;
	time_t now;
	char *t;

	va_start(args);
	time(&now);
	t = asctime(localtime(&now));
	/* remove the newline */
	t[24] = '\0';
	if ((fp = fopen(bootplog, "a+")) == NULL)
		return;
	fprintf(fp, "[%s]: ", t);
	format = va_arg(args, char *);
	vfprintf(fp, format, args);
	putc('\n', fp);
	fclose(fp);
	return;
}
