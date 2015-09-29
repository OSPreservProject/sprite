
/*
 * @(#)boot.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../h/bootparam.h"
#include "../h/sunromvec.h"
#include "../dev/saio.h"
#include "../h/eeprom.h"
#include "../sun3/cpu.addrs.h"
#ifdef	SIRIUS
#include "../h/enable.h"
#include "../h/globram.h"
#include "../sun3/cpu.map.h"
#include "../h/cache.h"
#endif	SIRIUS

/*
 * nullsys is used as the probe routine address if the device is
 * not to be booted by default.
 */
int	nullsys();

extern struct boottab xddriver;         /* VME Xylogics disk driver */
extern struct boottab xydriver;		/* Xylogics disk driver */

#ifdef DDBOOT
extern struct boottab dddriver;
#endif

#ifdef IPBOOT
extern struct boottab ipdriver;
#endif

#ifdef ECBOOT
extern struct boottab ecdriver;
#endif

extern struct boottab ledriver;   	/* AMD Ethernet driver */
extern struct boottab iedriver;		/* Intel Ethernet driver */
extern struct boottab mtdriver;		/* TapeMaster tape driver */

#ifdef ARBOOT
extern struct boottab ardriver;
#endif

extern struct boottab sddriver;		/* SCSI disk driver */
extern struct boottab stdriver;		/* SCSI tape driver */
extern struct boottab xtdriver;         /* Xylogics tape driver */

#ifdef RSBOOT
extern struct boottab radriver, rbdriver;
#endif 

struct boottab *(boottab[]) = {
#ifndef M25
        &xddriver, 
	&xydriver,
#endif 
#ifdef DDBOOT
	&dddriver,
#endif
#ifdef IPBOOT
	&ipdriver,
#endif
	&sddriver,
#ifdef M25
	&ledriver,
#else M25
	&iedriver,
#endif 
#ifdef ECBOOT
	&ecdriver,
#endif
#ifndef M25
	&mtdriver,
	&xtdriver,
#endif M25
	&stdriver,
#ifdef ARBOOT
	&ardriver,
#endif
#ifdef RSBOOT
	&radriver,
	&rbdriver,
#endif
	0,
};

#define	skipblank(p)	{ while (*(p) == ' ') (p)++; }

boot(cmd)
	char *cmd;
{
	struct boottab **tablep;
	register struct boottab *tp;
	register struct bootparam *bp = *romp->v_bootparam;
	register char *dev = 0, *name;
	register char *p, *q;
	char *gethex(), *puthex();
	register int i,j;
	int	eeprom_boot = 0;
	struct saioreq req;

	if (*cmd == '?') 
		goto syntax ;

/* for Sirius family we want to flush cache prior to a boot */
#ifdef	SIRIUS
	if (get_enable() & ENA_CACHE) {
		vac_flush_all();
	}
#endif	SIRIUS

	for (p = cmd; *p != 0 && *p != ' ' && *p != '('; p++)
		;
	q = p;
	skipblank(p);
	if (*p == '(') {	/* device specified */
		p++;
		if (q > cmd+2)
			goto syntax;
		*q = 0;
		for (tablep = boottab; 0 != (tp = *tablep); tablep++) {
			if (cmd[0] == tp->b_dev[0] && cmd[1] == tp->b_dev[1]) {
				dev = cmd;
				break;
			}
		}
		if (dev == 0) goto syntax;
		p = gethex(p, &bp->bp_ctlr);
		p = gethex(p, &bp->bp_unit);
		p = gethex(p, &bp->bp_part);
		if (*p != 0 && *p != ')')
			goto syntax;
	} else {		/* default boot */
		p = cmd;

		for (tablep = boottab; 0 != (tp = *tablep); tablep++) {
	  	   reset_alloc();
		   bzero((char *)&req, sizeof (req));
		   req.si_boottab = tp;

		   if ((get_enable() & 0x01) == 1 && *p == 0) {
		      eeprom_boot = 1;
		      if (EEPROM->ee_diag.eed_diagdev[0] == tp->b_dev[0] &&
                          EEPROM->ee_diag.eed_diagdev[1] == tp->b_dev[1]) {
			      name = EEPROM->ee_diag.eed_diagpath; 
                              bp->bp_ctlr = EEPROM->ee_diag.eed_diagctrl;
                              dev = tp->b_dev;
                              break;
                      }
		   } else if (EEPROM->ee_diag.eed_defboot == EED_NODEFBOOT) {
		      if (EEPROM->ee_diag.eed_bootdev[0] == tp->b_dev[0] &&
		          EEPROM->ee_diag.eed_bootdev[1] == tp->b_dev[1]) {
			      bp->bp_ctlr = EEPROM->ee_diag.eed_bootctrl;
                              dev = tp->b_dev;
                              break;
		      }
                   } else if ((bp->bp_ctlr = (*tp->b_probe)(&req)) != -1) {
		      dev = tp->b_dev;
		      break;
		   }
		}

		if (eeprom_boot)
		   printf("\n\nEEPROM:  Diagnostic Auto-boot in progress...\n");

		if (dev == 0) {
			printf("No default boot devices.\n\n");
			return (-1);
		}

		if (eeprom_boot) {
			bp->bp_unit = EEPROM->ee_diag.eed_diagunit;
                        bp->bp_part = EEPROM->ee_diag.eed_diagpart;
		} else if (EEPROM->ee_diag.eed_defboot == EED_NODEFBOOT) {
			bp->bp_unit = EEPROM->ee_diag.eed_bootunit;
			bp->bp_part = EEPROM->ee_diag.eed_bootpart;
		} else {
			bp->bp_unit = 0;
			bp->bp_part = 0;
		}
	}

	if (!eeprom_boot) {
		if (*p == ')')
			p++;
		skipblank(p);
		if (*p == 0 || *p == '-') {
			name = "";	/* string for UNIX */
		} else {
			name = p;
			while (*p != 0 && *p != ' ')
			p++;
			if (*p == ' ') {
				*p = 0;
				p++;
				skipblank(p);
			}
		}
	}

	printf ("Boot: %c%c(%x,%x,%x)%s %s\n",
	    dev[0], dev[1], bp->bp_ctlr, bp->bp_unit, bp->bp_part, name, p);

	/* Put in bootparam */
	bp->bp_dev[0] = dev[0];
	bp->bp_dev[1] = dev[1];
	bp->bp_boottab = tp;
	q = bp->bp_strings;
	*q++ = dev[0];
	*q++ = dev[1];
	*q++ = '(';
	q = puthex(q, bp->bp_ctlr);
	*q++ = ',';
	q = puthex(q, bp->bp_unit);
	*q++ = ',';
	q = puthex(q, bp->bp_part);
	*q++ = ')';
	bp->bp_name = q;
	while (*q++ = *name++)
		;
	bp->bp_argv[0] = bp->bp_strings;
	for (i = 1; i < (sizeof bp->bp_argv/sizeof bp->bp_argv[0])-1;) {
		skipblank(p);
		if (*p == '\0')
			break;
		bp->bp_argv[i++] = q;
		while (*p != '\0' && *p != ' ')
			*q++ = *p++;
		/* SHOULD CHECK RANGE OF q */
		*q++ = '\0';
	}
	bp->bp_argv[i] = (char *)0;
	reset_alloc();	/* Set up to give resources to driver */
	return ((*tp->b_boot)(bp));

syntax:
	printf ("Boot syntax: b [!][dev(ctlr,unit,part)] name [options]\n\
Possible boot devices:\n");
	for (tablep = boottab; 0 != (tp = *tablep); tablep++)
		printf("  %s\n", tp->b_desc);
	return (-1);
}

char *
gethex(p, ip)
	register char *p;
	int *ip;
{
	register int acc = 0;

	skipblank(p);
	while (*p) {
		if (*p >= '0' && *p <= '9')
			acc = (acc<<4) + (*p - '0');
		else if (*p >= 'a' && *p <= 'f')
			acc = (acc<<4) + (*p - 'a' + 10);
		else if (*p >= 'A' && *p <= 'F')
			acc = (acc<<4) + (*p - 'A' + 10);
		else
			break;
		p++;
	}
	skipblank(p);
	if (*p == ',')
		p++;
	skipblank(p);
	*ip = acc;
	return (p);
}

char *
puthex(p, n)
	register char *p;
	register int n;
{
	register int a;

	if (a = ((unsigned)n >> 4))
		p = puthex(p, a);
	*p++ = "0123456789abcdef"[n & 0xF];
	return (p);
}
 
nullsys()
{
	return (-1);
}
