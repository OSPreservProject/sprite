
/*	@(#)ndreg.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Definitions for software state of the network disk protocol handler.
 */

#define	MAXNDBYTES	512		/* Max size ndstrategy supports */
#define	DATASIZE	NDMAXDATA	/* Size of each read from server */

struct nd {                     /* standalone RAM variables */
        int     nd_seq;         /* current sequence number */
        struct ether_header nd_xh;      /* xmit header and packet */
        struct ndpack nd_xp;
        int     nd_block;       /* starting block number of data in "cache" */
        char    nd_data[DATASIZE]; /* "cache" of receive data */
        char    nd_buf[1600];   /* temp buf for packets */
        short   nd_efound;      /* found my ether addr */
        struct ether_addr nd_eaddr;     /* my ether addr */
};
