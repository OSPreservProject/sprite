#ifndef lint
static	char sccsid[] = "@(#)inet.c	1.12 88/02/08	Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Standalone IP send and receive - specific to Ethernet
 * Includes ARP and Reverse ARP
 */
#include "boot.h"
#include "sainet.h"
#include "idprom.h"

Net_EtherAddress etherbroadcastaddr = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};


#define WAITCNT	2	/* 4 seconds before bitching about arp/revarp */

/*
 * Fetch our Ethernet address from the ID prom
 */
myetheraddr(ea)
	Net_EtherAddress *ea;
{
	struct idprom id;

	if (idprom(IDFORM_1, &id) != IDFORM_1) {
		printf("ERROR: missing or invalid ID prom\n");
		return;
	}
	*ea = *(Net_EtherAddress *)id.id_ether;
}

/*
 * Initialize IP state
 * Find out our Ethernet address and call Reverse ARP
 * to find out our Internet address
 * Set the ARP cache to the broadcast host
 */
inet_init(fileId, sain, tmpbuf)
	register void *fileId;
	register struct sainet *sain;
	char *tmpbuf;
{
	myetheraddr(&sain->myEther);
	bzero((caddr_t)&sain->myAddr, sizeof(Net_InetAddress));
	bzero((caddr_t)&sain->hisAddr, sizeof(Net_InetAddress));
	sain->hisEther = etherbroadcastaddr;
	revarp(fileId, sain, tmpbuf);
}


/*
 * Output an IP packet
 * Cause ARP to be invoked if necessary
 */
ip_output(fileId, buf, len, sain, tmpbuf)
	register void *fileId;
	caddr_t buf, tmpbuf;
	short len;
	register struct sainet *sain;
{
	register Net_EtherHdr *eh;
	register Net_IPHeader *ip;

	eh = (Net_EtherHdr *)buf;
	ip = (Net_IPHeader *)(buf + sizeof(Net_EtherHdr));
	if (!inet_cmp(&ip->dest, &sain->hisAddr)) {
		inet_copy(&sain->hisAddr, &ip->dest);
		arp(fileId, sain, tmpbuf);
	}
	eh->type = NET_ETHER_IP;
	eh->source = sain->myEther;
	eh->destination = sain->hisEther;
	/* checksum the packet */
	ip->checksum = 0;
	ip->checksum = ipcksum((caddr_t)ip, sizeof (Net_IPHeader));
	if (len < NET_ETHER_MIN_BYTES) {
		len = NET_ETHER_MIN_BYTES;
	}
	return !xmit_packet(fileId, buf, len);
}

/*
 * Check incoming packets for IP packets
 * addressed to us. Also, respond to ARP packets
 * that wish to know about us.
 * Returns a length for any IP packet addressed to us, 0 otherwise.
 */
ip_input(fileId, buf, sain)
	register void *fileId;
	caddr_t buf;
	register struct sainet *sain;
{
	register short len;
	register Net_EtherHdr *eh;
	register Net_IPHeader *ip;
	register Net_ArpPacket *ea;

	len = poll_packet(fileId, buf);
	eh = (Net_EtherHdr *)buf;
	if (eh->type == NET_ETHER_IP &&
	    len >= sizeof(Net_EtherHdr)+sizeof(Net_IPHeader)) {
		ip = (Net_IPHeader *)(buf + sizeof(Net_EtherHdr));
#ifdef NOREVARP
		if ((sain->hisAddr.s_addr & 0xFF000000) == 0 &&
		    NET_ETHER_COMPARE(etherbroadcastaddr,
			eh->destination) == 0 &&
		    (in_broadaddr(sain->hisAddr) ||
		    in_lnaof(ip->source) == in_lnaof(sain->hisAddr))) {
			sain->myAddr = ip->dest;
			sain->hisAddr = ip->source;
			sain->hisEther = eh->source;
		}
#endif
		if (!inet_cmp(&ip->dest, &sain->myAddr))
			return (0);
		return (len);
	}
	if (eh->type == NET_ETHER_ARP &&
	    len >= sizeof(Net_EtherHdr) + sizeof(Net_ArpPacket)) {
		ea = (Net_ArpPacket *)buf;
		if (ea->protocolType != NET_ETHER_IP)
			return (0);
		if (inet_cmp(ea->senderProtAddr, &sain->hisAddr)) {
			sain->hisEther = ea->senderEtherAddr;
		}
		if (ea->opcode == NET_ARP_REQUEST &&
		    inet_cmp(ea->targetProtAddr, &sain->myAddr)) {
			ea->opcode = NET_ARP_REPLY;
			eh->destination = ea->senderEtherAddr;
			eh->source = sain->myEther;
			ea->targetEtherAddr = ea->senderEtherAddr;
			inet_copy(ea->targetProtAddr, ea->senderProtAddr);
			ea->senderEtherAddr = sain->myEther;
			inet_copy(ea->senderProtAddr, &sain->myAddr);
			(void) xmit_packet(fileId, buf, 
			    sizeof(Net_ArpPacket));
		}
		return (0);
	}
	return (0);
}

/*
 * arp
 * Broadcasts to determine Ethernet address given IP address
 * See RFC 826
 */
arp(fileId, sain, tmpbuf)
	register void *fileId;
	register struct sainet *sain;
	char *tmpbuf;
{
	Net_ArpPacket out;

	if (in_broadaddr(sain->hisAddr)
#ifdef NOREVARP
	    || (sain->hisAddr.s_addr & 0xFF000000) == 0
#endif
	    ) {
		sain->hisEther = etherbroadcastaddr;
		return;
	}
	out.header.type = NET_ETHER_ARP;
	out.opcode = NET_ARP_REQUEST;
	out.targetEtherAddr = etherbroadcastaddr;	/* what we want */
	inet_copy(out.targetProtAddr, &sain->hisAddr);
	comarp(fileId, sain, &out, tmpbuf);
}

/*
 * Reverse ARP client side
 * Determine our Internet address given our Ethernet address
 * See RFC 903
 */
revarp(fileId, sain, tmpbuf)
	register void *fileId;
	register struct sainet *sain;
	char *tmpbuf;
{
	Net_ArpPacket out;

#ifdef NOREVARP
	bzero((caddr_t)&sain->myAddr, sizeof(Net_InetAddress));
	bcopy((caddr_t)&sain->myEther.ether_addr_octet[3],
		(caddr_t)(&sain->myAddr)+1, 3);
#else
	out.header.type = NET_ETHER_REVARP;
	out.opcode = NET_RARP_REQUEST;
	out.targetEtherAddr = sain->myEther;
	/* What we want to find out... */
	bzero((caddr_t)out.targetProtAddr, sizeof(Net_InetAddress));
	comarp(fileId, sain, &out, tmpbuf);
#endif
}

/*
 * Common ARP code 
 * Broadcast the packet and wait for the right response.
 * Fills in *sain with the results
 */
comarp(fileId, sain, out, tmpbuf)
	register void *fileId;
	register struct sainet *sain;
	register Net_ArpPacket *out;
	char *tmpbuf;
{
	register Net_ArpPacket *in = (Net_ArpPacket *)tmpbuf;
	register int e, count, time, feedback,len, delay = 2;
	char    *ind = "-\\|/";

	out->header.destination = etherbroadcastaddr;
	out->header.source = sain->myEther;
	out->hardwareType =  NET_ARP_TYPE_ETHER;
	out->protocolType = NET_ETHER_IP;
	out->hardwareAddrLen = sizeof(Net_EtherAddress);
	out->protocolAddrLen = sizeof(Net_InetAddress);
	out->senderEtherAddr = sain->myEther;
	inet_copy(out->senderProtAddr, &sain->myAddr);
	feedback = 0;

	for (count=0; ; count++) {
		if (count == WAITCNT) {
			if (out->opcode == NET_ARP_REQUEST) {
				printf("\nRequesting Ethernet address for ");
				inet_print(out->targetProtAddr);
			} else {
				printf("\nRequesting Internet address for ");
				ether_print(&out->targetEtherAddr);
			}
		}
		e = xmit_packet(fileId, (caddr_t)out, sizeof *out);
		if (!e)
			printf("X\b");
		else
			printf("%c\b", ind[feedback++ % 4]); /* Show activity */

		time = millitime() + (delay * 1000);	/* broadcast delay */
		while (millitime() <= time) {
			len = poll_packet(fileId, tmpbuf);
			if (len < sizeof(Net_ArpPacket))
				continue;
			if (in->protocolType != NET_ETHER_IP)
				continue;
			if (out->opcode == NET_ARP_REQUEST) {
				if (in->header.type != NET_ETHER_ARP)
					continue;
				if (in->opcode != NET_ARP_REPLY)
					continue;
				if (!inet_cmp(in->senderProtAddr,
				    out->targetProtAddr))
					continue;
				if (count >= WAITCNT) {
					printf("Found at ");
					ether_print(&in->senderEtherAddr);
				}
				sain->hisEther = in->senderEtherAddr;
				return;
			} else {		/* Reverse ARP */
				if (in->header.type != NET_ETHER_REVARP)
					continue;
				if (in->opcode != NET_RARP_REPLY)
					continue;
				if (NET_ETHER_COMPARE(in->targetEtherAddr,
				    out->targetEtherAddr) == 0)
					continue;

				if (count >= WAITCNT) {
					printf("Internet address is ");
					inet_print(in->targetProtAddr);
				}
				inet_copy(&sain->myAddr, in->targetProtAddr);
				/*
				 * short circuit first ARP
				 */
				inet_copy(&sain->hisAddr, in->senderProtAddr);
				sain->hisEther = in->senderEtherAddr;
				return;
			}
		}

		delay = delay * 2;	/* Double the request delay */
		if (delay > 64)		/* maximum delay is 64 seconds */
			delay = 64;

		reset(fileId);
	}
	/* NOTREACHED */
}

/*
 * Return the host portion of an internet address.
 */
in_lnaof(in)
	Net_InetAddress in;
{
	if (NET_INET_CLASS_A_ADDR(in))
		return ((in)&NET_INET_CLASS_A_HOST_MASK);
	else if (NET_INET_CLASS_B_ADDR(in))
		return ((in)&NET_INET_CLASS_B_HOST_MASK);
	else
		return ((in)&NET_INET_CLASS_C_HOST_MASK);
}

/*
 * Test for broadcast IP address
 */
in_broadaddr(in)
	Net_InetAddress in;
{
	if (NET_INET_CLASS_A_ADDR(in)) {
		in &= NET_INET_CLASS_A_HOST_MASK;
		return (in == 0 || in == 0xFFFFFF);
	} else if (NET_INET_CLASS_B_ADDR(in)) {
		in &= NET_INET_CLASS_B_HOST_MASK;
		return (in == 0 || in == 0xFFFF);
	} else if (NET_INET_CLASS_C_ADDR(in)) {
		in &= NET_INET_CLASS_C_HOST_MASK;
		return (in == 0 || in == 0xFF);
	} else
		return (0);
	/*NOTREACHED*/
}

/*
 * Compute one's complement checksum
 * for IP packet headers 
 */
ipcksum(cp, count)
	caddr_t	cp;
	register unsigned short	count;
{
	register unsigned short	*sp = (unsigned short *)cp;
	register unsigned long	sum = 0;
	register unsigned long	oneword = 0x00010000;

	count >>= 1;
	while (count--) {
		sum += *sp++;
		if (sum >= oneword) {		/* Wrap carries into low bit */
			sum -= oneword;
			sum++;
		}
	}
	return (~sum);
}

inet_print(p)
	Net_InetAddress *p;
{
	Net_InetAddress s;

	inet_copy(&s, p);
	printf("%d.%d.%d.%d\n",
		(s >> 24) & 0xff,
		(s >> 16) & 0xff,
		(s >>  8) & 0xff,
		s & 0xff);
}

ether_print(ea)
	Net_EtherAddress *ea;
{
	printf("%x:%x:%x:%x:%x:%x\n",
	    NET_ETHER_ADDR_BYTE1(*ea),
	    NET_ETHER_ADDR_BYTE2(*ea),
	    NET_ETHER_ADDR_BYTE3(*ea),
	    NET_ETHER_ADDR_BYTE4(*ea),
	    NET_ETHER_ADDR_BYTE5(*ea),
	    NET_ETHER_ADDR_BYTE6(*ea));
}
