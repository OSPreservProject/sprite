#ifndef _SAINET_H
#define _SAINET_H

#include <netInet.h>

/*
 * Standalone Internet Protocol State
 */

struct sainet {
	Net_InetAddress 	myAddr;		/* my host address */
	Net_EtherAddress	myEther;	/* my Ethernet address */
	Net_InetAddress		hisAddr;	/* his host address */
	Net_EtherAddress	hisEther;	/* his Ethernet address */
};
#endif /* _SAINET_H */
