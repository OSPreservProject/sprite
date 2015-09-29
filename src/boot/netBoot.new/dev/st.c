
/*
 * @(#)st.c 1.1 86/09/27 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "../dev/saio.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../dev/dklabel.h"
#include "../dev/dkio.h"
#include "../dev/screg.h"
#include "../dev/sireg.h"
#include "../dev/scsi.h"
#include "../dev/streg.h"
#include "../h/idprom.h"

/*
 * Driver for Sysgen SC4000 and EMULEX MT-02 SCSI tape controllers.
 * Supports qic11 format only.
 */
extern int scdoit();
#ifdef SUN3
extern int sidoit();
#endif

#define min(a,b)	((a)<(b)? (a): (b))

#define NSD 1
unsigned long staddrs[NSD] = { 0x0, };

extern int xxprobe(), ttboot();
int stopen(), stclose(), ststrategy();


struct stparam {
	int		st_target;
	int		st_unit;
	int		st_eof;
	int		st_lastcmd;
	int		st_ctype;
#ifdef SUN3
	int		st_ha_type;
#endif
	struct 		saioreq subsip[1];	/* sip for host adapter */
};

/*
 * DMA-able buffers
 */
#ifdef SUN2
/* virtual addresses are precious on Sun-2 */
#define	MAXSTBSIZE	(20*1024)	
#endif
#ifdef SUN3
#define	MAXSTBSIZE	(127*512)	
#endif
struct stdma {
	char	sbuffer[SENSE_LENGTH];
	char	databuffer[MAXSTBSIZE];
};

#define STSBUF		(((struct stdma *)sip->si_dmaaddr)->sbuffer)
#define STBUF		(((struct stdma *)sip->si_dmaaddr)->databuffer)

#define ISAEMULEX(stp)	(stp->st_ctype == ST_TYPE_EMULEX ? 1 : 0)

/*
 * What resources we need to run
 */
struct devinfo stinfo = {
	0,				/* No device to map in */
	sizeof (struct stdma),
	sizeof (struct stparam),
	NSD,				/* Dummy devices */
	staddrs,			/* Dummy devices */
	MAP_MAINMEM,
	MAXSTBSIZE,			/* transfer size */
};

struct boottab stdriver = {
	"st",	xxprobe, ttboot,
	stopen, stclose, ststrategy,
	"st: SCSI tape", &stinfo
};

#define	TAPE_TARGET		4	/* default SCSI target # for tape */

#define SENSELOC	4	/* sysgen returns sense at this offset */

#define	ROUNDUP(x)	((x + DEV_BSIZE - 1) & ~(DEV_BSIZE - 1))

#ifdef SUN3
/*
 * Determine type of host adaptor interface, si or sc.
 * Returns 1 if si host adaptor and 0 if sc host adaptor.
 */
st_probe(sip)
	struct saioreq *sip;
{
	if (siprobe(sip)) {
		return (1);
	} else {
		return (0);
	}
}
#endif

/*
 * Open the SCSI Tape controller
 */
int
stopen(sip)
	register struct saioreq *sip;
{
	register struct stparam *stp;
	register int r;
	register int skip;
	register int i;
	struct scsi_cdb cdb;
	struct scsi_scb scb;

	stp = (struct stparam *) sip->si_devdata;
	bzero( (char *)stp, (sizeof (struct stparam)));

	*stp->subsip = *sip;	/* initialize subsip */

#ifdef SUN3
	{
		extern struct boottab scdriver;
		extern struct boottab sidriver;

		/* FIXME, find out which scsi interface to use */
		if (st_probe(sip)) {
			stp->st_ha_type = 1;

			/* FIXME, must vector thru table */
			stp->subsip->si_boottab = &sidriver;
		} else {
			stp->st_ha_type = 0;

			/* FIXME, must vector thru table */
			stp->subsip->si_boottab = &scdriver;
		}
	}
#endif
#ifdef SUN2
	{
		extern struct boottab scdriver;

		stp->subsip->si_boottab = &scdriver;	
	}
#endif

	stp->subsip->si_unit = sip->si_unit >> 3;       /* Target number */
	if (stp->subsip->si_unit == 0) {
		stp->subsip->si_unit = TAPE_TARGET;
	}

	r = devopen(stp->subsip);
	if (r < 0) return r;

	/* Logical unit number */
	stp->st_unit = sip->si_unit & 0x07;

	/*
	 * Must wait for tape controller to become ready.
	 * This takes about 10 seconds for the Emulex.
	 */
#ifndef M25
	DELAY(10000000);
#endif
	/*
	 * Test for the controller being ready. First test will fail if
	 * the SCSI bus is permanently busy or if a previous op was 
	 * interrupted in mid-transfer. Second one should work.
	 */
#ifdef M25
	do {
		bzero((char *) &cdb, sizeof cdb);
		cdb.cmd = SC_TEST_UNIT_READY;
		cdb.count = 0;
		stp->subsip->si_cc = 0;
		stp->subsip->si_ma = 0;
		r = sidoit(&cdb, &scb, stp->subsip);
		DELAY(10000);
	} while (scb.busy);
#else M25
	for (i = 0; i < 2; i++) {
		bzero((char *) &cdb, sizeof cdb);
		cdb.cmd = SC_TEST_UNIT_READY;
		cdb.count = 0;
		stp->subsip->si_cc = 0;
		stp->subsip->si_ma = 0;
#ifdef SUN3
		if (stp->st_ha_type)
			r = sidoit(&cdb, &scb, stp->subsip);
		else
#endif
			r = scdoit(&cdb, &scb, stp->subsip);
	}
#endif M25								

	/* 
	 * To figure out what type of tape controller is out there we send 
	 * a REQUEST_SENSE command and see how much sense data comes back.  
	 */
	stp->st_ctype = ST_TYPE_EMULEX;
	cdb.cmd = SC_REQUEST_SENSE;
	stp->subsip->si_cc = cdb.count = ST_EMULEX_SENSE_LEN;
	stp->subsip->si_ma = STSBUF;
#ifdef SUN3
	if (stp->st_ha_type)
		r = sidoit(&cdb, &scb, stp->subsip);
	else
#endif
		r = scdoit(&cdb, &scb, stp->subsip);
	if (r != ST_EMULEX_SENSE_LEN) {
	        stp->st_ctype = ST_TYPE_SYSGEN;
		cdb.cmd = SC_REQUEST_SENSE;
		stp->subsip->si_cc = cdb.count = ST_SYSGEN_SENSE_LEN;
		stp->subsip->si_ma = STSBUF;
#ifdef SUN3
		if (stp->st_ha_type)
			r = sidoit(&cdb, &scb, stp->subsip);
		else
#endif
			r = scdoit(&cdb, &scb, stp->subsip);
		if (r == -1) {
			printf("stopen: cannot get sense, %d\n", r);
			return (-1);
		}
	} 

	/* 
	 * Default format mode for emulex is qic24.
	 * Needs to be qic11.
	 */
	if (ISAEMULEX(stp)) {
		sip->si_cc = EM_MS_PL_LEN;
		if (stcmd(SC_QIC11, sip, 1) == 0) {
			printf("stopen: mode select command fail");
			return (-1);
		}
	}

	/*
	 * Rewind a few times until it works.  First one will fail if
	 * the SCSI bus is permanently busy if a previous op was interrupted
	 * in mid-transfer.  Second one will fail with POR status, after
	 * the scsi bus is reset from the busy.  Third one should work.  
	 */
	sip->si_cc = 0;
	if (stcmd(SC_REWIND, sip, 0) == 0 &&
	    stcmd(SC_REWIND, sip, 0) == 0 &&
	    stcmd(SC_REWIND, sip, 1) == 0) {
		return (-1);
	}
	sip->si_cc = 512;
	sip->si_ma = STSBUF;
	if (stcmd(SC_READ, sip, 0) == -2) {
		sip->si_cc = 0;
		stcmd(SC_QIC24, sip, 1);
	}
	sip->si_cc = 0;
	stcmd(SC_REWIND, sip, 1); /* rewind again to be safe */
	skip = sip->si_boff;
	while (skip--) {
		sip->si_cc = 0;
		if (stcmd(SC_SPACE_FILE, sip, 1) == 0) {
			return (-1);
		}
	}
	stp->st_eof = 0;
	stp->st_lastcmd = 0;
	return (0);
}

/*
 * Close the tape drive.
 */
stclose(sip)
	register struct saioreq *sip;
{
	register struct stparam *stp;

	stp = (struct stparam *) sip->si_devdata;
	if (stp->st_lastcmd == SC_WRITE) {
		(void) stcmd(SC_WRITE_FILE_MARK, sip, 0);
	}
	(void) stcmd(SC_REWIND, sip, 0);
	if (ISAEMULEX(stp)) {
		sip->si_cc = EM_MS_PL_LEN;
		if (stcmd(SC_QIC11, sip, 1) == 0) {
			printf("stclose: mode select command fail");
			return (-1);
		}
	} else {
		sip->si_cc = 0;
		if (stcmd(SC_QIC11, sip, 1) == 0) {
			printf("stclose: mode select command fail");
			return (-1);
		}
	}
}


/*
 * Perform a read or write of the SCSI tape.
 */
int
ststrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	register struct stparam *stp;

	stp = (struct stparam *) sip->si_devdata;
	if (stp->st_eof) {
		stp->st_eof = 0;
		return (0);
	}
	return (stcmd(rw == WRITE ? SC_WRITE : SC_READ, sip, 1));
}

/*
 * Execute a scsi tape command
 */
int
stcmd(cmd, sip, errprint)
	int cmd;
	register struct saioreq *sip;
	int errprint;
{
	register struct st_emulex_mspl *mode;
	register int r, i, c;
	register char *buf;
	register struct stparam *stp;
	struct scsi_cdb cdb, scdb;
	struct scsi_scb scb, sscb;
	int count;
	int qic;
	int sense_length;
	char *cp;
	struct st_emulex_sense *ems;
	struct st_sysgen_sense *scs;

	count = sip->si_cc;
	stp = (struct stparam *)sip->si_devdata;
	buf =  sip->si_ma;

	if (cmd == SC_WRITE && buf != STBUF) {
		bcopy(buf, STBUF, (unsigned)count);
	}

	/* set up cdb */
	bzero((char *) &cdb, sizeof cdb);
	cdb.cmd = cmd;
	cdb.lun = stp->st_unit;
	c = ROUNDUP(count) / DEV_BSIZE;
	cdb.high_count = c >> 16;
	cdb.mid_count = (c >> 8) & 0xFF;
	cdb.low_count = c & 0xFF;
	stp->subsip->si_ma = STBUF;
	stp->subsip->si_cc = ROUNDUP(count);

	/* some fields in the cdb are command specific */
	switch (cmd) {

	case SC_QIC11:
	        stp->subsip->si_ma = 0;
	        stp->subsip->si_cc = 0; 
		if (ISAEMULEX(stp)) {
			qic = ST_EMULEX_QIC11;
			cdb.cmd = SC_MODE_SELECT;
			goto MODE;
		} else {
			cdb.cmd = SC_QIC02;
			cdb.high_count = ST_SYSGEN_QIC11;
			break;
		}
		/* NOTREACHED */

	case SC_QIC24:
	        stp->subsip->si_ma = 0;
	        stp->subsip->si_cc = 0; 
		if (ISAEMULEX(stp)) {
			qic = ST_EMULEX_QIC24;
			cdb.cmd = SC_MODE_SELECT;
			goto MODE;
		} else {
			cdb.cmd = SC_QIC02;
			cdb.high_count = ST_SYSGEN_QIC24;
			break;
		}
		/* NOTREACHED */
MODE:
	case SC_MODE_SELECT:
 		mode = (struct st_emulex_mspl *)STSBUF;
		bzero(mode, sizeof(*mode));
		mode->hdr.bufm = 1;
		mode->bd.density = qic;
		mode->hdr.bd_len = EM_MS_BD_LEN;
		stp->subsip->si_ma = (char *)mode;
		stp->subsip->si_cc = cdb.count = EM_MS_PL_LEN;
		break;

	case SC_SPACE_FILE:
		cdb.cmd = SC_SPACE;	/* the real SCSI cmd */
		cdb.t_code = 1;		/* space file, not rec */
		cdb.low_count = 1;	/* space 1 file, not 0 */
		stp->subsip->si_ma = 0;
		stp->subsip->si_cc = 0;
		break;

	case SC_WRITE_FILE_MARK:
		cdb.count = 1;	
		/* fall through... */

	case SC_TEST_UNIT_READY:
	case SC_REWIND:
	case SC_LOAD:
		stp->subsip->si_ma = 0;
		stp->subsip->si_cc = 0;
		break;

	case SC_READ:
	case SC_WRITE:
 		if (ISAEMULEX(stp)) {
		        cdb.t_code = 1;
		}
		break;

	default:
		if (errprint)
			printf("st: unknown command\n");
		return (0);
	}

	/* execute the command */
#ifdef SUN3
	if (stp->st_ha_type)
		r = sidoit(&cdb, &scb, stp->subsip);
	else
#endif
		r = scdoit(&cdb, &scb, stp->subsip);

	/* allow for tape blocking */
	if (r > count)
		r = count;
	/* error */
	if (r == -1) 
		return (0);

	if (cmd == SC_READ) {
		if (min(count, r))
			bcopy(STBUF, buf, (unsigned)(min(count, r)));
	}

	/* we may need to get sense data */
	if (scb.chk) {
		bzero((char *) &scdb, sizeof scdb);
		scdb.cmd = SC_REQUEST_SENSE;
		scdb.lun = stp->st_unit;
		if (ISAEMULEX(stp)) {
			sense_length = stp->subsip->si_cc =
			    scdb.count = ST_EMULEX_SENSE_LEN;
		} else {
			sense_length = stp->subsip->si_cc = 
			    scdb.count = ST_SYSGEN_SENSE_LEN;
		}
		stp->subsip->si_ma = STSBUF;
#ifdef SUN3
		if (stp->st_ha_type)
			i = sidoit(&scdb, &sscb, stp->subsip );
		else
#endif
			i = scdoit(&scdb, &sscb, stp->subsip );
		if (i != sense_length) {
			if (errprint) {
				printf("st: sense error\n");
			}
			stp->st_eof = 1;
			return (0);
		} 
		if (ISAEMULEX(stp)) {
			ems = (struct st_emulex_sense *)STSBUF;
			stp->st_eof = 1;
			if ((ems->ext_sense.key == 0x8) &&
			       (ems->error == 0x34))
				return(-2);
			if ((ems->ext_sense.fil_mk == 0) && errprint) {
				printf("st: error:\n");
				printf("\tsense key is %x", 
				    ems->ext_sense.key);
				printf("\terror is %x\n",
				    ems->error);
			}
			return (r);
		} else  {
			scs = (struct st_sysgen_sense *)STSBUF;
			stp->st_eof = 1;
			if (*(unsigned short *)&STSBUF[SENSELOC] == 0x86a8)
				return(-2);
			if ((scs->qic_sense.file_mark == 0) && errprint) {
				printf("st: error %x\n", 
					*(unsigned short *)&STSBUF[SENSELOC]);
			}
			return (r);
		}
	}
	if (r >= count) {
		return (count ? count : 1);
	} else {
		if (errprint) {
			printf("st: short transfer\n");
		}
		return (r);
	}
}




