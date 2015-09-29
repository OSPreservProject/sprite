/*
 * tftp.c
 *
 * @(#)tftp.c 1.9 88/02/08 Copyr 1986 Sun Micro
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * Standalone network boot via TFTP
 */
#ifdef sun4
#define	SUN4
#endif
#ifdef sun3
#define	SUN3
#endif
#include "machparam.h"
#include "boot.h"
#include "saio.h"
#include "socket.h"
#include "if.h"
#include "in.h"
#include "if_ether.h"
#include "in_systm.h"
#include "ip.h"
#include "udp.h"
#include "sainet.h"
#include "sunromvec.h"
#include "cpu.addrs.h"
#include    <sys/exec.h>

#undef DEV_BSIZE
#undef MAX
#include "tftp.h"

#ifdef notdef
#include "globram.h"
#endif

#ifdef notdef
#define millitime() (gp->g_nmiclock)
#else
#define millitime() (*romp->v_nmiclock)
#endif
					/* tftp error messages */
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

					/* TFTP packet */
struct tftp_pack {
	/* struct ether_header tf_ether; */	/* Ethernet header */
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
	struct ether_header in_etherheader; 
	char	tf_tmpbuf[1600];	/* tmp for incoming packets */
	int	tf_block;		/* current block number */
	char	*tf_data;		/* current load pointer */
};

#if defined(SUN4) && defined(CACHE)		    /* for loading into cache */
#define LOADADDR 0x20000
#define TFTPBASE ((struct tftpglob *) 0xFFDC0300)
#else CACHE
#define LOADADDR 0x4000
#define TFTPBASE	((struct tftpglob *)0x3000)
#endif SUN4 && CACHE

#define	REXMIT_MSEC	4000		/* 4 seconds between retransmits */

/*
 * Description: Entry point for initializing the ethernet during boot.
 *
 * Synopsis:	status = etheropen(sip)
 *		status	:(int) 0 command complete
 *		sip	:(char *) pointer to saioreq structure
 *
 * Routines:	bzero, inet_init
 */
etheropen(sip)
	register struct saioreq *sip;
{
	register struct tftpglob *tf = TFTPBASE;

	bzero((caddr_t)tf, sizeof(*tf));	/* clear tftp work space */

	inet_init(sip, &tf->tf_inet, tf->tf_tmpbuf); /* get internet address */
	return (0);
}

#ifdef SUN2
etherstrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	printf("tftp: random access attempted - code error.\n");
	return(-1);
}
#endif SUN2

/*
 * Description: Loads the boot routine across the ethernet
 *
 * Synopsis:	status = tftpload(sip)
 *		status	:(int)    load address
 *				  -1 error
 *		sip	:(char *) pointer to saioreq structure
 *
 * Routines:	bzero
 *
 * Variables:	locked	:(int) lock in host and server
 *		firsttry:(int) first try flag for autoboot
 */
tftpload(sip, bp)
	register struct saioreq *sip;
	    struct bootparam   *bp ;
{
	register struct tftpglob    *tf = TFTPBASE;
	register struct tftp_pack   *out = &tf->tf_out;
	register struct tftp_pack   *in = (struct tftp_pack *)tf->tf_tmpbuf;
	register char		    *p, *q, *x;
	register short		    i, len;
	int			    autoboot = 0;
	int			    firsttry = 0;
	int			    feedback = 0;
	int finished = 0;
	int			    delay = REXMIT_MSEC;
	int			    time, xcount, locked, retry;
	char			    *ind = "-=-=";
#ifdef SUN4
        u_long			    temp=0;
#endif SUN4
	struct exec *header;
						/* if unit # is 0, this is
						 * an autoboot */
	if (sip->si_unit == 0)
                autoboot = 1;
top:
						/* Initialize IP header */
	out->tf_ip.ip_v = IPVERSION;		/* IP version number */
	out->tf_ip.ip_hl = sizeof(struct ip) / 4;/* header length */
	out->tf_ip.ip_ttl = MAXTTL;		/* time to live */
	out->tf_ip.ip_p = IPPROTO_UDP;		/* type of protocol */

						/* set source address */

	bcopy(&tf->tf_inet.sain_myaddr, &out->tf_ip.ip_src, sizeof(out->tf_ip.ip_src));
	 
						/* set destination address,
						 * Dst host is argument with
						 * our net number plugged in */
	if (autoboot && firsttry == 0) {
						/* send to host from revarp */

#ifdef SUN4
        	out->tf_ip.ip_dst.S_un.S_un_b.s_b1=
                  tf->tf_inet.sain_hisaddr.S_un.S_un_b.s_b1;
        	out->tf_ip.ip_dst.S_un.S_un_b.s_b2=
                  tf->tf_inet.sain_hisaddr.S_un.S_un_b.s_b2;
        	out->tf_ip.ip_dst.S_un.S_un_b.s_b3=
                  tf->tf_inet.sain_hisaddr.S_un.S_un_b.s_b3;
        	out->tf_ip.ip_dst.S_un.S_un_b.s_b4=
                  tf->tf_inet.sain_hisaddr.S_un.S_un_b.s_b4;
#else SUN4
		out->tf_ip.ip_dst.s_addr = tf->tf_inet.sain_hisaddr.s_addr;
#endif SUN4
		firsttry = 1;
	} else if (autoboot && firsttry > 0) {
						/* broadcast? */
#ifdef SUN4
		out->tf_ip.ip_dst.S_un.S_un_b.s_b1=(-1);
		out->tf_ip.ip_dst.S_un.S_un_b.s_b2=(-1);
		out->tf_ip.ip_dst.S_un.S_un_b.s_b3=(-1);
		out->tf_ip.ip_dst.S_un.S_un_b.s_b4=(-1);
#else SUN4
		out->tf_ip.ip_dst.s_addr = -1;
#endif SUN4
	} else {
						/* unit specified */

#ifdef SUN4
		struct in_addr  in;
		bcopy(&out->tf_ip.ip_src.s_addr, &in, sizeof(in));
		temp = in.s_addr;
		temp -= in_lnaof(in);
		temp += sip->si_unit;
		bcopy(&temp, &out->tf_ip.ip_dst.s_addr, sizeof(temp));
#else SUN4
		out->tf_ip.ip_dst.s_addr = out->tf_ip.ip_src.s_addr +
				sip->si_unit  - in_lnaof(out->tf_ip.ip_src);
#endif SUN4
	}

						/* initialize UDP header */

	out->tf_udp.uh_sport =  (millitime() & 1023) + 1024;/* source post */
	out->tf_udp.uh_dport =  IPPORT_TFTP;	/* destination port */
	out->tf_udp.uh_sum =  0;		/* no checksum */

						/* set tftpglob structure */
	tf->tf_block = 1;
	tf->tf_data = (char *)KERNEL_START;
						/* Create the TFTP Read Request
						 * packet */
	out->tf_tftp.th_opcode = RRQ;
						/* load internet address */
	q = bp->bp_name;
	p = out->tf_tftp.th_stuff;
	while (*q && *q != ' ') {
		*(p++) = *(q++);
	}
	*p++ = 0;
        q = "octet";
        while (*p++ = *q++)
                ;
                                                /* fill UDP packet */
	out->tf_udp.uh_ulen = sizeof (struct udphdr) + 2 + 
			(p - out->tf_tftp.th_stuff);
						/* fill ip packet */

	out->tf_ip.ip_len = sizeof (struct ip) + out->tf_udp.uh_ulen;

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
			printf("%c\b", ind[feedback++ % 4]);

						/* transmit */

			if (ip_output(sip, 
				((caddr_t)out) - sizeof (struct ether_header) ,
				out->tf_ip.ip_len + sizeof (struct ether_header), 
				&tf->tf_inet, tf->tf_tmpbuf))
				printf("X\b");
						/* 5 times if not locked */
			if (locked == 0 || retry > 15)
				xcount++;
			else 
				retry++;
		}
						/* get input IP packet */

		len = ip_input(sip,((caddr_t)in) - sizeof(struct ether_header),
			&tf->tf_inet);

						/* check length of packet */
		if (len < TFTPHDRLEN) {
#ifdef debugjl
/*printf(" not tftp packet\n");*/
#endif debugjl
			continue;
		}
						/* check packet type */

		if (in->tf_ip.ip_p != IPPROTO_UDP ||
		    in->tf_udp.uh_dport != out->tf_udp.uh_sport)  {
#ifdef debugjl
printf(" wrong packet type = %x\n",in->tf_ip.ip_p);
#endif debugjl
			continue;
		    }
						/* dst has been locked in */
#ifdef SUN4
		if ( locked &&
                    ( (out->tf_ip.ip_dst.S_un.S_un_b.s_b1 != 
                      in->tf_ip.ip_src.S_un.S_un_b.s_b1) ||      
                     (out->tf_ip.ip_dst.S_un.S_un_b.s_b2 != 
                      in->tf_ip.ip_src.S_un.S_un_b.s_b2) ||      
                     (out->tf_ip.ip_dst.S_un.S_un_b.s_b3 != 
                      in->tf_ip.ip_src.S_un.S_un_b.s_b3) ||      
                     (out->tf_ip.ip_dst.S_un.S_un_b.s_b4 != 
                      in->tf_ip.ip_src.S_un.S_un_b.s_b4)) )
#else SUN4
		if (locked &&
                    out->tf_ip.ip_dst.s_addr != in->tf_ip.ip_src.s_addr)
#endif SUN4
#ifdef debugjl
		    { 
			printf(" not locked address = %x%x%x%x\n",
			out->tf_ip.ip_dst.S_un.S_un_b.s_b1,
			out->tf_ip.ip_dst.S_un.S_un_b.s_b2,
			out->tf_ip.ip_dst.S_un.S_un_b.s_b3,
			out->tf_ip.ip_dst.S_un.S_un_b.s_b4);
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

			out->tf_udp.uh_dport = in->tf_udp.uh_sport;

						/* for autoboot, get address */
			if (autoboot) 
				bcopy(&(in->tf_ip.ip_src),&(out->tf_ip.ip_dst),
					sizeof(in->tf_ip.ip_src));

						/* print server found */

			printf("Booting from tftp server at "); 
                        inet_print(out->tf_ip.ip_dst);
			locked = 1;
		}
						/* calc. data length */

		len = in->tf_udp.uh_ulen - (sizeof(struct udphdr) + 4);

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
		out->tf_udp.uh_ulen = sizeof (struct udphdr) + 4;
		out->tf_ip.ip_len = sizeof (struct ip) + out->tf_udp.uh_ulen;

	    					/* transmit */

		if (ip_output(sip, (caddr_t)out - sizeof(struct ether_header),
			out->tf_ip.ip_len +
		    sizeof (struct ether_header), &tf->tf_inet,
		    tf->tf_tmpbuf))
			printf("X\b");
						/* reset count and retry */
		xcount = 0;
		retry = 0;
		printf("%c\b", ind[feedback++ % 4]);	/* Show activity */

						/* reset delay */
		delay = REXMIT_MSEC;
		time = millitime()+delay;
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

