
/*
 * @(#)tftp.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Standalone network boot via TFTP
 */
#include "../dev/saio.h"
#include "../h/socket.h"
#include "../dev/if.h"
#include "../h/in.h"
#include "../dev/if_ether.h"
#include "../h/in_systm.h"
#include "../h/ip.h"
#include "../h/udp.h"
#include "../h/sainet.h"
#include "../h/sunromvec.h"
#include "../sun3/cpu.addrs.h"
#undef DEV_BSIZE
#undef MAX
#include "../h/tftp.h"


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


#define       LOADADDR        0x4000
#define millitime() (*romp->v_nmiclock)

struct tftp_pack {	/* TFTP packet */
	struct ether_header tf_ether;	/* Ethernet header */
	struct ip tf_ip;		/* IP header */
	struct udphdr tf_udp;		/* UDP header */
	struct tftphdr tf_tftp;		/* TFTP header */
	char tftp_data[SEGSIZE];	/* TFTP data beyond header */
};

/*
 * Size of Headers in TFTP DATA packet
 */
#define TFTPHDRLEN	(sizeof (struct ether_header) + sizeof (struct ip) + \
			 sizeof (struct udphdr) + 4)

struct tftpglob {
	struct tftp_pack tf_out;	/* outgoing TFTP packet */
	struct sainet tf_inet;		/* Internet state */
	char	tf_tmpbuf[1600];	/* tmp for incoming packets */
	int	tf_block;		/* current block number */
	char	*tf_data;		/* current load pointer */
};

#define TFTPBASE	((struct tftpglob *)0x3000)

#define	REXMIT_MSEC	4000	/* 4 seconds between retransmits */

etheropen(sip)
	register struct saioreq *sip;
{
	register struct tftpglob *tf = TFTPBASE;

	bzero((caddr_t)tf, sizeof (*tf));
	inet_init(sip, &tf->tf_inet, tf->tf_tmpbuf); /* Initialize inet */
	return (0);
}

etherstrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	printf("tftp: random access attempted - code error.\n");
	return (-1);
}

tftpload(sip)
	register struct saioreq *sip;
{
	register struct tftpglob *tf = TFTPBASE;
	register struct tftp_pack *out = &tf->tf_out;
	register struct tftp_pack *in = (struct tftp_pack *)tf->tf_tmpbuf;
	register char *p, *q, *x;
	register short i, len;
	int autoboot = 0;
	int firsttry = 0;
	int feedback = 0;
	int time, xcount, locked, retry;
	char    *ind = "-\\|/";

	if (sip->si_unit == 0)
                autoboot = 1;	/* if unit # is 0, this is an autoboot */
top:
	/*
	 * Initialize IP and UDP headers
	 */
	out->tf_ip.ip_v = IPVERSION;
	out->tf_ip.ip_hl = sizeof (struct ip) / 4;
	out->tf_ip.ip_ttl = MAXTTL;
	out->tf_ip.ip_p = IPPROTO_UDP;
	out->tf_udp.uh_sport =  (millitime() & 1023) + 1024;
	out->tf_udp.uh_dport =  IPPORT_TFTP;
	out->tf_udp.uh_sum =  0;		/* no checksum */
	 
	/* 
	 * Set src and dst host addresses
	 * Dst host is argument with our net number plugged in
	 */
	out->tf_ip.ip_src = tf->tf_inet.sain_myaddr;

	if (autoboot && firsttry == 0) {
        	out->tf_ip.ip_dst.s_addr = tf->tf_inet.sain_hisaddr.s_addr;
	} else if (autoboot && firsttry > 0) {
		out->tf_ip.ip_dst.s_addr = -1;
	} else {
		out->tf_ip.ip_dst.s_addr = out->tf_ip.ip_src.s_addr +
				sip->si_unit  - in_lnaof(out->tf_ip.ip_src);
	}

	++firsttry;
	locked = 0;
	retry = 0;
	tf->tf_block = 1;
	tf->tf_data = (char *)LOADADDR;

	/*
	 * Create the TFTP Read Request packet 
	 */
	out->tf_tftp.th_opcode = RRQ;
	p = out->tf_tftp.th_stuff;
	q = (char *)&tf->tf_inet.sain_myaddr;
	x = "0123456789ABCDEF";
	for (i=0; i<4; i++) {
		*p++ = x[(*q >> 4) & 0xF];
		*p++ = x[(*q++) & 0xF];
	}
	*p++ = 0;
	q = "octet";
	while (*p++ = *q++)
		;
	out->tf_udp.uh_ulen = sizeof (struct udphdr) + 2 +
		(p - out->tf_tftp.th_stuff);
	out->tf_ip.ip_len = sizeof (struct ip) +
		out->tf_udp.uh_ulen;
	
	time = 0;
	for (xcount = 0; xcount < 5;) {
		if (millitime() - time >= REXMIT_MSEC) {
			time = millitime();
			printf("%c\b", ind[feedback++ % 4]); /* Show activity */
			if (ip_output(sip, (caddr_t)out, out->tf_ip.ip_len +
			    sizeof (struct ether_header), &tf->tf_inet,
			    tf->tf_tmpbuf))
				printf("X\b");
			if (locked == 0 || retry > 15)
				xcount++;
			else 
				retry++;
		}
		len = ip_input(sip, (caddr_t)in, &tf->tf_inet);
		if (len < TFTPHDRLEN)
			continue;
		if (in->tf_ip.ip_p != IPPROTO_UDP ||
		    in->tf_udp.uh_dport != out->tf_udp.uh_sport) 
			continue;
		if (locked &&
                    out->tf_ip.ip_dst.s_addr != in->tf_ip.ip_src.s_addr)                                continue;
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
			if (autoboot)
				goto top;
			return (-1);
		}
		if (in->tf_tftp.th_opcode != DATA ||
		    in->tf_tftp.th_block != tf->tf_block) 
			continue;
		/*
		 * Here if we have an in sequence DATA packet
		 */
		if (tf->tf_block == 1) {	/* lock on to server and port */
			out->tf_udp.uh_dport = in->tf_udp.uh_sport;
			if (autoboot) 
				out->tf_ip.ip_dst = in->tf_ip.ip_src;
			printf("Booting from tftp server at "); 
                        inet_print(out->tf_ip.ip_dst);
			locked = 1;
		}
		/*
		 * Swallow data
		 */
		len = in->tf_udp.uh_ulen - (sizeof(struct udphdr) + 4);
		if (len) {
			bcopy(in->tf_tftp.th_data, tf->tf_data, len);
			tf->tf_data += len;
		}
		/*
		 * Send ACK 
		 */
		xcount = 0;
		retry = 0;
		out->tf_tftp.th_opcode = ACK;
		out->tf_tftp.th_block = tf->tf_block++;
		out->tf_udp.uh_ulen = sizeof (struct udphdr) + 4;
		out->tf_ip.ip_len = sizeof (struct ip) + out->tf_udp.uh_ulen;
		time = millitime();
		printf("%c\b", ind[feedback++ % 4]);	/* Show activity */
		if (ip_output(sip, (caddr_t)out, out->tf_ip.ip_len +
		    sizeof (struct ether_header), &tf->tf_inet,
		    tf->tf_tmpbuf))
			printf("X\b");
		if (len < SEGSIZE) { 	/* end of file */
			printf("Downloaded %d bytes from tftp server.\n\n",
				tf->tf_data - LOADADDR);
			return (LOADADDR);
		}
	}
	printf("tftp: time-out.\n");
	if (autoboot)
		goto top;
	return (-1);
}

