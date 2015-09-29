/*
 * 
 * Copyright (c) 1988 Regents of the University of California
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of the University of California not be used in advertising
 * or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Permission to incorporate this
 * software into commercial products can be obtained from the Campus
 * Software Office, 295 Evans Hall, University of California, Berkeley,
 * Ca., 94720 provided only that the the requestor give the University
 * of California a free licence to any derived software for educational
 * and research purposes.  The University of California makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 * 
 */

/*
 * ASACA (Metrum RSS-600) device driver. (User-level). Sends robot
 * control signals over RS-232 port and interprets results. Driver
 * provides a "slot-number" interface to the RSS-600 with slots
 * 0 ... 599 corresponding to the drum bins (which store tapes) and
 * slots 600 ... 604 corresponding to tape readers, or VLDS's.
 */

#include "asaca.h"

int as_debug = 1;

/*
 * asaca_open() is just a wrapper around the standard UNIX open system
 * call. Open the ASACA device, set termio characteristics
 */
asaca_open (path)
	char *path;
{
	int fdes;

	if ((fdes = open (path, O_RDWR|O_BLKANDSET)) < 0)
		return (-1);

	if (ioctl (fdes, TCSETA, &asaca_tio) < 0) {
		asaca_close (fdes);
		return (-1);
	}
	return (fdes);
}

/*
 * asaca_close() simply closes the device. May need to do other things
 * here prior to closing as we gain experience...
 */
asaca_close (fdes)
	int fdes;
{
	return (close (fdes));
}

/*
 * asaca_write() simply does a character-by-character write to the ASACA
 * device returned by asaca_open(). Returns #bytes actually written or
 * failure
 */
asaca_write (fdes, buf, nbytes)
	int fdes;
	caddr_t buf;
	int nbytes;
{
	register int i = 0;

	while ((*buf != '\0') && i < nbytes) {
		if (write (fdes, buf, 1) != 1)
	    		return (-1);
		buf++; i++;
	}
	return (i);
}

/*
 * asaca_read() does a character-by-character read of the ASACA device
 * until a terminating '>' is read. Returns #bytes actually read or
 * failure
 */
asaca_read (fdes, buf, nbytes)
	int fdes;
	caddr_t buf;
	int nbytes;
{
	register int i = 0;

	while (i < nbytes) {
		switch (read (fdes, buf, 1)) {
		case	1:
			i++;
			if (*buf++ == ASTERM) {
				*buf = '\0';
				return (i);
			}
			break;
		case	0:
			*buf = '\0';
			return (i);
		default:
			return (-1);
		}
	}
	return (0);
}

/*
 * asaca_ioctl() issues asaca commands over tty line. Code compatible with
 * standard UNIX ioctl kernel code with future migration to kernel-mode
 * in mind.
 */
asaca_ioctl (fdes, cmd, data)
	int 	fdes;
	int	cmd;
	caddr_t	*data;
{
	register struct	ascmd *dp;
	register char *bp;
	register int len;
	char buf[BUFSIZ];

	if ((cmd&_IOC_IN) || (cmd&_IOC_OUT))
		dp = (struct ascmd *)data;

	switch (cmd) {
	case	ASIOCALRMON:
	case	ASIOCALRMOFF:
		switch (dp->as_mode) {
		case	0:
			sprintf (buf, "O%c***%c>", unittocmd(cmd), ASALRMALL);
			break;
		case	1:
			sprintf (buf, "O%c***%c>", unittocmd(cmd), ASALRM1);
			break;
		case	2:
			sprintf (buf, "O%c***%c>", unittocmd(cmd), ASALRM2);
			break;
		case	3:
			sprintf (buf, "O%c***%c>", unittocmd(cmd), ASALRM3);
			break;
		default:
			goto bad;
		}
		break;

	case	ASIOCLOAD2:
	case	ASIOCSTORE2:
		if (isbin(dp->as_bin) && isvlds(dp->as_vlds)) {
			sprintf (buf, "O%c%c%02d%cD%13s>", unittocmd(cmd),
			  unittoface(dp->as_bin), unittobin(dp->as_bin),
			  unittovlds(dp->as_vlds), dp->as_barcode);
		} else {
			goto bad;
		}
		break;

	case	ASIOCLOAD1:
	case	ASIOCSTORE1:
		if (isbin(dp->as_bin) && isvlds(dp->as_vlds)) {
			sprintf (buf, "O%c%c%02d%c>", unittocmd(cmd),
			  unittoface(dp->as_bin), unittobin(dp->as_bin),
			  unittovlds(dp->as_vlds));
		} else {
			goto bad;
		}
		break;

	case	ASIOCBCREAD2:

		/*
		 * XXX - Only two operations for single-cassette barcode
		 * reads are supported: 
		 *	- read barcode of tape in a slot
		 *	- read barcode of tape in a VLDS
		 */
		if (isbin(dp->as_src)) {
			sprintf (buf, "O%c%c%02d*>", unittocmd(cmd),
			  unittoface(dp->as_src), unittobin(dp->as_src));
		} else {
			if (isvlds(dp->as_src)) {
				sprintf (buf, "O%c30%c*>", unittocmd(cmd),
					 unittovlds(dp->as_src));
			} else {
				goto bad;
			}
		}
		break;

	case	ASIOCINIT:
	case	ASIOCEJECT:
	case	ASIOCINJECT:
	case	ASIOCDOOROPEN:
	case	ASIOCSENSE:
		sprintf (buf, "O%c****>", unittocmd(cmd));
		break;

	case	ASIOCDRUMSET:
		if (isbin(dp->as_bin)) {
			sprintf (buf, "O%c%c***>", unittocmd(cmd),
				 unittoface(dp->as_bin));
		} else {
			goto bad;
		}
		break;

	case	ASIOCFIPDISP:
		if (isvlds(dp->as_vlds)) {
			sprintf (buf, "O%c***%cB%13s>", unittocmd(cmd),
				 unittovlds(dp->as_vlds), dp->as_barcode);
		} else {
			goto bad;
		}
		break;

	case	ASIOCSIDEIND:
		if (isbin(dp->as_bin)) {
			switch (dp->as_mode) {
			case	0:
				sprintf (buf, "O%c2%02d%c>", unittocmd(cmd),
					 unittobin(dp->as_bin), ASON);
				break;
			case	1:
				sprintf (buf, "O%c2%02d%c>", unittocmd(cmd),
					 unittobin(dp->as_bin), ASOFF);
				break;
			default:
				goto bad;
			}
		} else {
			goto bad;
		}
		break;

	case	ASIOCDETECT:

		/*
		 * Metrum only supports cassette detect for the right
		 * side drum. (drum faces 'G' - 'L'). User should
		 * issue "DETECT" command only for slots 300 - 599.
		 */
		if (isbin(dp->as_bin) && (unittoface(dp->as_bin) > 'F')) {
			sprintf (buf, "O%c%c%02d*>", unittocmd(cmd),
				 unittoface(dp->as_bin),
				 dp->as_mode == 0 ? 0 :
				 unittobin(dp->as_bin));
		} else {
			goto bad;
		}
		break;

	case	ASIOCMANUAL:
		switch (dp->as_mode) {
		case	0:
			sprintf (buf, "O%c%c***>", unittocmd(cmd), ASON);
			break;
		case	1:
			sprintf (buf, "O%c%c***>", unittocmd(cmd), ASOFF);
			break;
		default:
			goto bad;
		}
		break;

	case	ASIOCDOORLOCK:
		switch (dp->as_mode) {
		case    0:
			sprintf (buf, "O%c%c***>", unittocmd(cmd), ASUNLOCK);
			break;
		case    1:
			sprintf (buf, "O%c%c***>", unittocmd(cmd), ASLOCK);
			break;
		default:
			goto bad;
		}
		break;

	case	ASIOCMOVE1:
		if (isvlds(dp->as_src) && isvlds(dp->as_dest)) {
			sprintf (buf, "O%c%c%c**>", unittocmd(cmd),
				 unittovlds(dp->as_src),
				 unittovlds(dp->as_dest));
		} else {
			goto bad;
		}
		break;

	case	ASIOCMOVE2:
		if (isbin(dp->as_src) && isbin(dp->as_dest)) {
			switch (dp->as_mode) {
			case	0:
				sprintf (buf, "O%c%c%02d%cQ%c%02d%13s>",
					 unittocmd(cmd), unittoface(dp->as_src),
					 unittobin(dp->as_src), ASBARCHK,
					 unittoface(dp->as_dest),
					 unittobin(dp->as_dest),
					 dp->as_barcode);
				break;
			case	1:
				sprintf (buf, "O%c%c%02d%cQ%c%02d%13s>",
					 unittocmd(cmd), unittoface(dp->as_src),
					 unittobin(dp->as_src), ASNOBARCHK,
					 unittoface(dp->as_dest),
					 unittobin(dp->as_dest),
					 "*************");
				break;
			default:
				goto bad;
			}
		} else {
			goto bad;
		}
		break;

	case	ASIOCMVHDLR:
	case	ASIOCMOVE3:
		if (isbin(dp->as_dest)) {
			sprintf (buf, "O%c%c%02d*>", unittocmd(ASIOCMOVE3),
				 unittoface(dp->as_dest),
				 unittobin(dp->as_dest));
		} else {
			if (isvlds(dp->as_dest)) {
				sprintf (buf, "O%c%c01*>",
					 unittocmd(ASIOCMOVE3),
				         unittovlds(dp->as_dest));
			} else {
				goto bad;
			}
		}
		break;

	case	ASIOCMVTAPE:
		/*
		 * Generic tape move command; no barcode checks. Provides
		 * cleaner, more natural interface to applications. Code is
		 * redundant as h*ll, but works...
		 */
		if ((!isbin(dp->as_src) && !isvlds(dp->as_src)) ||
		    (!isbin(dp->as_dest) && !isvlds(dp->as_dest)))
			goto bad;

		if (isbin(dp->as_src) && isbin(dp->as_dest)) {
			/*
			 * Same as MOVE2 with no barcode check...
			 */
			sprintf (buf, "O%c%c%02d%cQ%c%02d%13s>",
				 unittocmd(ASIOCMOVE2),
				 unittoface(dp->as_src),
				 unittobin(dp->as_src), ASNOBARCHK,
				 unittoface(dp->as_dest),
				 unittobin(dp->as_dest),
				 "*************");
			break;
		}

		if (isbin(dp->as_src) && isvlds(dp->as_dest)) {
			/* 
			 * Same as LOAD1...
			 */
			sprintf (buf, "O%c%c%02d%c>",
				 unittocmd(ASIOCLOAD1),
				 unittoface(dp->as_src),
				 unittobin(dp->as_src),
				 unittovlds(dp->as_dest));
			break;
		}

		if (isvlds(dp->as_src) && isbin(dp->as_dest)) {
			/*
			 * Same as STORE1...
			 */
			sprintf (buf, "O%c%c%02d%c>",
				 unittocmd(ASIOCSTORE1),
				 unittoface(dp->as_dest),
				 unittobin(dp->as_dest),
				 unittovlds(dp->as_src));
			break;
		}

		if (isvlds(dp->as_src) && isvlds(dp->as_dest)) {
			/* 
			 * Same as MOVE1...
			 */
			sprintf (buf, "O%c%c%c**>", unittocmd(ASIOCMOVE1),
				 unittovlds(dp->as_src),
				 unittovlds(dp->as_dest));
			break;
		}

	case	ASIOCMVTAPE_BC:
		/*
		 * Generic tape move with barcode checks
		 */
		if ((!isbin(dp->as_src) && !isvlds(dp->as_src)) ||
		    (!isbin(dp->as_dest) && !isvlds(dp->as_dest)))
			goto bad;

		if (isbin(dp->as_src) && isbin(dp->as_dest)) {
			/*
			 * Same as MOVE2 with barcode check...
			 */
			sprintf (buf, "O%c%c%02d%cQ%c%02d%13s>",
				 unittocmd(ASIOCMOVE2),
				 unittoface(dp->as_src),
				 unittobin(dp->as_src), ASBARCHK,
				 unittoface(dp->as_dest),
				 unittobin(dp->as_dest),
				 dp->as_barcode);
			break;
		}

		if (isbin(dp->as_src) && isvlds(dp->as_dest)) {
			/* 
			 * Same as LOAD2...
			 */
			sprintf (buf, "O%c%c%02d%cD%13s>",
				 unittocmd(ASIOCLOAD2),
				 unittoface(dp->as_src),
				 unittobin(dp->as_src),
				 unittovlds(dp->as_dest),
				 dp->as_barcode);
			break;
		}

		if (isvlds(dp->as_src) && isbin(dp->as_dest)) {
			/*
			 * Same as STORE2...
			 */
			sprintf (buf, "O%c%c%02d%cD%13s>",
				 unittocmd(ASIOCSTORE2),
				 unittoface(dp->as_dest),
				 unittobin(dp->as_dest),
				 unittovlds(dp->as_src),
				 dp->as_barcode);
			break;
		}

		if (isvlds(dp->as_src) && isvlds(dp->as_dest)) {
			/*
			 * MOVE1 doesn't support barcode checking
			 */
			goto bad;
		}
		
	case	ASIOCBCREAD1:
	case	ASIOCBCRS:
		/*
		 * XXX - Bar Code Read Command 1 (of 50 tapes) and
		 * associated Bar Code Read Stop Command not supported
		 */
	default:
		goto bad;
	}

	/*
	 * issue command to ASACA and wait for completion code.
	 */
	len = strlen(buf);
	if ((asaca_write (fdes, buf, len)) != len) {
		/* errno set by "asaca_write" */
		return (-1);
	}

	if (!(cmd&_IOC_OUT)) {
		/* No answer generated for these; return success */
		return (0);
	}

	if (cmd == ASIOCBCREAD2) {
		/* Give bar code read extra time to complete */
		sleep (1);
	}

	/*
	 * Read ASACA command response. Response will be of
	 * the form: "Oxy>", where 'x' is a command completion
	 * code flag and 'y' is zero or more additional char's,
	 * making "len" at least three in normal cases
	 */
	if ((len = asaca_read(fdes, buf, BUFSIZ)) < 3) {
		/* errno set by "asaca_read" */
		return (-1);
	}
		
	if ((buf[0] != ASHDR) || (buf[len - 1] != ASTERM)) {
		/* mal-formed response; this should never happen */
		goto bad;
	}
		
	switch (buf[1]) {
	case	'D':
	case	'F':
	case	'K':
	case	'V':
		/* Normal operation answer */
		dp->as_error = AS_ENOERR;
		break;
	case	'H':
		/* Barcode read return */
		dp->as_error = AS_ENOERR;
		bcopy (&buf[5], dp->as_barcode, 13);
		break;
	case	'S':
		/* Door sense return */
		dp->as_error = AS_ENOERR;
		bcopy (&buf[2], dp->as_sense, 3);
		break;
	case	'W':
		/* Cassette detect return */
		dp->as_error = AS_ENOERR;
		bcopy (&buf[5], dp->as_sense, 1);
		break;
	case	'B':
		/* Barcode mismatch warning */
		dp->as_error = AS_EWARN;
		bcopy (&buf[2], dp->as_barcode, 13);
		if (as_debug)
			printf ("asaca: Barcode mismatched\n");
		break;
	case	'E':
		/* Barcode read failure */
		dp->as_error = AS_EWARN;
		if (as_debug)
			printf ("asaca: Barcode read failure\n");
		break;
	case	'N':
		/* Error without operational interrupt */
		dp->as_error = AS_EWARN;
		if (as_debug) {
			switch (buf[2]) {
			case	'1':
				printf ("asaca: Volume was not grabbed\n");
				break;
			case	'2':
				printf ("asaca: Volume was not loaded\n");
				break;
			case	'3':
				printf ("asaca: Right door is open\n");
				break;
			case	'4':
				printf ("asaca: Front door is open\n");
				break;
			case	'5':
				printf ("asaca: Volume is protruding from one of the bins\n");
				break;
			case	'6':
				printf ("asaca: I/O failure\n");
				break;
			default:
				printf ("asaca: Unknown warning\n");
			}
		}
		break;

	case	'G':
		/* Error with operational Interrupt */
		dp->as_error = AS_EFATAL;
		switch (buf[2]) {
		case	'1':
			printf ("asaca: Right drum failure\n");
			break;
		case	'2':
			printf ("asaca: Left drum failure\n");
			break;
		case	'3':
			printf ("asaca: Elevator failure in the x axis\n");
			break;
		case	'4':
			printf ("asaca: Handler failure in the y axis\n");
			break;
		case	'5':
			printf ("asaca: Handler failure in the z axis\n");
			break;
		case	'6':
			printf ("asaca: Hardware error\n");
			break;
		case	'7':
			printf ("asaca: Command error, RTS 2 second failure\n");
			break;
		default:
			printf ("asaca: Unknown fatal error\n");
		}
	}

done:
	return (0);
bad:
	errno = EINVAL;
	return (-1);
}
