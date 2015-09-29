/*
 * tftp.c
 *
 * @(#)tftp.c 1.9 88/02/08 Copyr 1986 Sun Micro
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * Standalone network boot via TFTP
 */
#include "boot.h"
#include "sainet.h"
#include <arpa/tftp.h>
#include <sys/exec.h>

#ifndef	IPPORT_TFTP
#define IPPORT_TFTP	69
#endif

char	*tftp_errs[] = {
	"not defined",
	"file not found",
	"access violation",
	"disk full or allocation exceeded",
	"illegal TFTP operation",
	"unknown transfer ID",
	"file already exists",
	"no such user"
};

struct tftp_pack {
	Net_IPHeader tf_ip;		/* IP header */
	Net_UDPHeader tf_udp;		/* UDP header */
	struct tftphdr tf_tftp;		/* TFTP header */
	char tftp_data[SEGSIZE];	/* TFTP data beyond header */
};

typedef union {
    char buf[NET_ETHER_MAX_BYTES + 4];
    struct {
	Net_EtherHdr header;
	Net_IPHeader ip;		/* IP header */
    } aligned;
} PacketBuffer;
					/* TFTP packet */
/*
 * Size of Headers in TFTP DATA packet
 */
#define TFTPHDRLEN	(sizeof (Net_EtherHdr) + sizeof (Net_IPHeader) + \
			 sizeof (Net_UDPHeader) + 4)

struct tftpglob {
	PacketBuffer	tf_out;		/* outgoing TFTP packet */
	PacketBuffer	tf_tmpbuf;	/* tmp for incoming packets */
	struct sainet tf_inet;		/* Internet state */
	int	tf_block;		/* current block number */
	char	*tf_data;		/* current load pointer */
};

static struct tftpglob tfbuf;

#define	REXMIT_MSEC	4000		/* 4 seconds between retransmits */

/*
 * Description: Entry point for initializing the ethernet during boot.
 *
 * Synopsis:	status = etheropen(fileId)
 *		status	:(int) 0 command complete
 *		fileId	:(void *) opaque pointer to PROM device
 *
 * Routines:	bzero, inet_init
 */
etheropen(fileId)
	register void *fileId;
{
	register struct tftpglob *tf = &tfbuf;

	bzero((caddr_t)tf, sizeof(*tf));	/* clear tftp work space */

	/* get internet address */
	inet_init(fileId, &tf->tf_inet, &tf->tf_tmpbuf);
	return (0);
}


/*
 * Description: Loads the boot routine across the ethernet
 *
 * Synopsis:	status = tftpload(fileId, fileName, unitNum)
 *		status	:(int)    load address
 *				  -1 error
 *		fileId	:(void *) opaque pointer to PROM device (sun4c)
 *				  or pointer to saioreq structure (sun4)
 *		fileName:(char *) remote file to transfer
 *		unitNum	:(int)    autoboot unit number
 *
 * Routines:	bzero
 *
 * Variables:	locked	:(int) lock in host and server
 *		firsttry:(int) first try flag for autoboot
 */
tftpload(fileId, fileName, unitNum)
	register void *fileId;
	char *fileName;
	int unitNum;
{
	register struct tftpglob    *tf = &tfbuf;
	register struct tftp_pack   *in =
	    (struct tftp_pack *)((caddr_t)&tf->tf_tmpbuf.aligned.ip);
	register struct tftp_pack   *out =
	    (struct tftp_pack *)((caddr_t)&tf->tf_out.aligned.ip);
	register char		    *p, *q, *x;
	register short		    i, len;
	int			    autoboot = 0;
	int			    firsttry = 0;
	int			    feedback = 0;
	int finished = 0;
	int			    delay = REXMIT_MSEC;
	int			    time, xcount, locked, retry;
	char			    *ind = ".oOo";
	int			    feedbackCount = 0;
	struct exec *header;
						/* if unit # is 0, this is
						 * an autoboot */
	if (unitNum == 0)
                autoboot = 1;
top:
						/* Initialize IP header */
	out->tf_ip.version = NET_IP_VERSION;	/* IP version number */
	out->tf_ip.headerLen = sizeof(Net_IPHeader) / 4;/* header length */
	out->tf_ip.timeToLive = NET_IP_MAX_TTL;		/* time to live */
	out->tf_ip.protocol = NET_IP_PROTOCOL_UDP;	/* type of protocol */

						/* set source address */

	bcopy(&tf->tf_inet.myAddr, &out->tf_ip.source, sizeof(out->tf_ip.source));
	 
						/* set destination address,
						 * Dst host is argument with
						 * our net number plugged in */
	if (autoboot && firsttry == 0) {
						/* send to host from revarp */

		inet_copy(&out->tf_ip.dest, &tf->tf_inet.hisAddr);
		firsttry = 1;
	} else if (autoboot && firsttry > 0) {
		Net_InetAddress tmpBroadcast = NET_INET_BROADCAST_ADDR;
						/* broadcast? */
		inet_copy(&out->tf_ip.dest, &tmpBroadcast);
	} else {
						/* unit specified */

		out->tf_ip.dest = out->tf_ip.source +
				unitNum  - in_lnaof(out->tf_ip.source);
	}

						/* initialize UDP header */

	out->tf_udp.srcPort =  (millitime() & 1023) + 1024;/* source post */
	out->tf_udp.destPort =  IPPORT_TFTP;	/* destination port */
	out->tf_udp.checksum =  0;		/* no checksum */

						/* set tftpglob structure */
	tf->tf_block = 1;
	tf->tf_data = (char *)KERNEL_START;
						/* Create the TFTP Read Request
						 * packet */
	out->tf_tftp.th_opcode = RRQ;
						/* load internet address */
	q = fileName;
	p = out->tf_tftp.th_stuff;
	while (*q && *q != ' ') {
		*(p++) = *(q++);
	}
	*p++ = 0;
        q = "octet";
        while (*p++ = *q++)
                ;
                                                /* fill UDP packet */
	out->tf_udp.len = sizeof (Net_UDPHeader) + 2 + 
			(p - out->tf_tftp.th_stuff);
						/* fill ip packet */

	out->tf_ip.totalLen = sizeof (Net_IPHeader) + out->tf_udp.len;

						/* init. transmit status */
	locked = 0;
	retry = 0;
	time = millitime();
						/* transmit loop */
	for (xcount = 0; xcount < 5;) {		/* try to xmit 5 times */
		if (millitime() >= time) {
			time = millitime() + delay;

						/* limit delay to 64 sec */
			delay = delay < 64000 ? delay * 2 : 64000;

						/* show activity */
			if ((feedbackCount++ % 8) == 0) 
				printf("%c\b", ind[feedback++ % 4]);

						/* transmit */

			if (ip_output(fileId, 
				(caddr_t)out - sizeof(Net_EtherHdr),
				out->tf_ip.totalLen + sizeof (Net_EtherHdr),
				&tf->tf_inet, (caddr_t)&tf->tf_tmpbuf))
				printf("X\b");
						/* 5 times if not locked */
			if (locked == 0 || retry > 15)
				xcount++;
			else 
				retry++;
		}
						/* get input IP packet */

		len = ip_input(fileId, (caddr_t)in - sizeof(Net_EtherHdr),
		    &tf->tf_inet);

						/* check length of packet */
		if (len < TFTPHDRLEN) {
#ifdef debugjl
/*printf(" not tftp packet\n");*/
#endif debugjl
			continue;
		}
						/* check packet type */

		if (in->tf_ip.protocol != NET_IP_PROTOCOL_UDP ||
		    in->tf_udp.destPort != out->tf_udp.srcPort)  {
#ifdef debugjl
printf(" wrong packet type = %x\n",in->tf_ip.protocol);
#endif debugjl
			continue;
		    }
						/* dst has been locked in */
		if (locked &&
                    out->tf_ip.dest != in->tf_ip.source)
#ifdef debugjl
		    { 
			printf(" not locked address = %x%x%x%x\n",
			out->tf_ip.dest.S_un.S_un_b.s_b1,
			out->tf_ip.dest.S_un.S_un_b.s_b2,
			out->tf_ip.dest.S_un.S_un_b.s_b3,
			out->tf_ip.dest.S_un.S_un_b.s_b4);
			continue;
		    }
#else
			continue;
#endif debugjl
						/* error */

		if (in->tf_tftp.th_opcode == ERROR) {
			if (autoboot && tf->tf_block == 1)
				continue;
			if (in->tf_tftp.th_code < 0 ||
			    in->tf_tftp.th_code > sizeof(tftp_errs)/sizeof(char *)){
				printf("tftp: Unknown error 0x%x\n",
					in->tf_tftp.th_code);
			} else {
				printf("tftp: %s @ block %d\n",
					tftp_errs[in->tf_tftp.th_code], tf->tf_block);
			}
						/* for autoboot, keep looping */
			if (autoboot)
				goto top;
			return (-1);
		}
						/* we are looking for data */

		if (in->tf_tftp.th_opcode != DATA ||
		    in->tf_tftp.th_block  != tf->tf_block) 
#ifdef debugjl
		    { 
			printf(" not data packet");
			continue;
		    }
#else
			continue;
#endif debugjl
						/* in sequence DATA packet */
		if (tf->tf_block == 1) {	
						/* lock onto server port */

			out->tf_udp.destPort = in->tf_udp.srcPort;

						/* for autoboot, get address */
			if (autoboot) 
				bcopy(&(in->tf_ip.source),&(out->tf_ip.dest),
					sizeof(in->tf_ip.source));

						/* print server found */

			printf("Booting from tftp server at "); 
                        inet_print(&out->tf_ip.dest);
			locked = 1;
		}
						/* calc. data length */

		len = in->tf_udp.len - (sizeof(Net_UDPHeader) + 4);

						/* copy data to load point */
		if (len) {
			bcopy(in->tf_tftp.th_data, tf->tf_data, len);
			if (tf->tf_block == 1) {
			    header = (struct exec *)tf->tf_data;
			    printf("Size: %d", header->a_text);
			}
			tf->tf_data += len;
			if (header->a_text) {
			    header->a_text -= len;
			    if (tf->tf_block == 1) {
				/*
				 * If on first block, don't count header size against the
				 * text size.
				 */
				header->a_text += sizeof(struct exec);
			    }
			    if ((int)header->a_text <= 0) {
				printf("+%d", header->a_data);
				header->a_data += header->a_text;
				header->a_text = 0;
			    }
			} else {
			    header->a_data -= len;
			    if ((int)header->a_data <= 0) {
				printf("+%d\n", header->a_bss);
				finished = 1;
			    }
			}
		}
						/* send ACK (acknowledge) */
		out->tf_tftp.th_opcode = ACK;
		out->tf_tftp.th_block = tf->tf_block++;
		out->tf_udp.len = sizeof (Net_UDPHeader) + 4;
		out->tf_ip.totalLen =
		    sizeof (Net_IPHeader) + out->tf_udp.len;

	    					/* transmit */

		if (ip_output(fileId, (caddr_t)out - sizeof(Net_EtherHdr),
		    out->tf_ip.totalLen + sizeof (Net_EtherHdr), &tf->tf_inet,
		    (caddr_t)&tf->tf_tmpbuf))
			printf("X\b");
						/* reset count and retry */
		xcount = 0;
		retry = 0;
		if ((feedbackCount++ % 8) == 0) 
			printf("%c\b", ind[feedback++ % 4]); /* Show activity */

						/* reset delay */
		delay = REXMIT_MSEC;
		time = millitime() + delay;
						/* check if end of file */
		if ((len < SEGSIZE)  || finished) {
#ifndef sun4
		    /*
		     * Zero out the uninitialized data, since Sprite
		     * doesn't do it for
		     * itself...
		     */
		    tf->tf_data += (int)header->a_data;
		    bzero(tf->tf_data, header->a_bss);
#endif	    
			printf("Downloaded %d bytes from tftp server.\n\n",
				tf->tf_data - KERNEL_START);
			return (KERNEL_START + sizeof(struct exec));
		}
	}
	printf("tftp: time-out.\n");
						/* for autoboot, loop forever */
	if (autoboot)
		goto top;
						/* error return */
	return (-1);
}
