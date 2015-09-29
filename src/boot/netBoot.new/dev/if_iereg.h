
/*	@(#)if_iereg.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Control block definitions for Intel 82586 (Ethernet) chip
 * All fields are byte-swapped because the damn chip wants bytes
 * in Intel byte order only, so we swap everything going to it.
 */

/* byte-swapped data types */
typedef u_short ieoff_t;	/* control block offsets from iscp cbbase */
#define	IENORBD	0xffff		/* null pointer for rbd */
typedef short ieint_t;		/* 16 bit integers */
typedef	long ieaddr_t;		/* data (24-bit) addresses */

/*
 * System Configuration Pointer
 * Must be at 0xFFFFF6 in chip's address space
 */
#define IESCPADDR 0xFFFFF6
struct iescp {
	char	ie_sysbus;	/* bus width: 0 => 16, 1 => 8 */
	char	ie_junk1[5];	/* unused */
	ieaddr_t ie_iscp;	/* address of iscp */
};

/*
 * Intermediate System Configuration Pointer
 * Specifies base of all other control blocks and the offset of the SCB
 */
struct ieiscp {
	char	ie_busy;	/* 1 => initialization in progress */
	char	ie_junk2;	/* unused */
	ieoff_t	ie_scb;		/* offset of SCB */
	ieaddr_t ie_cbbase;	/* base of all control blocks */
};

/* 
 * System Control Block - the focus of communication
 */
struct iescb {
	u_char		: 1;	/* mbz */
	u_char	ie_rus	: 3;	/* receive unit status */
	u_char		: 4;	/* mbz */
	u_char	ie_cx	: 1;	/* command done (interrupt) */
	u_char	ie_fr	: 1;	/* frame received (interrupt) */
	u_char	ie_cnr	: 1;	/* command unit not ready */
	u_char	ie_rnr	: 1;	/* receive unit not ready */
	u_char		: 1;	/* mbz */
	u_char	ie_cus	: 3;	/* command unit status */
	short	ie_cmd;		/* command word */
	ieoff_t	ie_cbl;		/* command list */
	ieoff_t	ie_rfa;		/* receive frame area */
	ieint_t	ie_crcerrs;	/* count of CRC errors */
	ieint_t	ie_alnerrs;	/* count of alignment errors */
	ieint_t	ie_rscerrs;	/* count of discarded packets */
	ieint_t	ie_ovrnerrs;	/* count of overrun packets */
};

/* ie_rus */
#define	IERUS_IDLE	0
#define	IERUS_SUSPENDED	1
#define	IERUS_NORESOURCE 2
#define	IERUS_READY	4

/* ie_cus */
#define	IECUS_IDLE	0
#define	IECUS_SUSPENDED	1
#define	IECUS_READY	2

/* ie_cmd */
#define	IECMD_RESET	0x8000	/* reset chip */
#define	IECMD_RU_START	(1<<12)	/* start receiver unit */
#define	IECMD_RU_RESUME	(2<<12)	/* resume receiver unit */
#define	IECMD_RU_SUSPEND (3<<12) /* suspend receiver unit */
#define	IECMD_RU_ABORT	(4<<12)	/* abort receiver unit */
#define	IECMD_ACK_CX	0x80	/* ack command executed */
#define	IECMD_ACK_FR	0x40	/* ack frame received */
#define	IECMD_ACK_CNR	0x20	/* ack CU not ready */
#define	IECMD_ACK_RNR	0x10	/* ack RU not ready */
#define	IECMD_CU_START	1	/* start receiver unit */
#define	IECMD_CU_RESUME	2	/* resume receiver unit */
#define	IECMD_CU_SUSPEND 3	/* suspend receiver unit */
#define	IECMD_CU_ABORT	4	/* abort receiver unit */

/*
 * Generic command block
 */
struct	iecb {
	u_char		: 8;	/* part of status */
	u_char	ie_done : 1;	/* command done */
	u_char	ie_busy : 1;	/* command busy */
	u_char	ie_ok	: 1;	/* command successful */
	u_char	ie_aborted : 1;	/* command aborted */
	u_char		: 4;	/* more status */
	u_char		: 5;	/* unused */
	u_char	ie_cmd	: 3;	/* command # */
	u_char	ie_el	: 1;	/* end of list */
	u_char	ie_susp	: 1;	/* suspend when done */
	u_char	ie_intr : 1;	/* interrupt when done */
	u_char		: 5;	/* unused */
	ieoff_t	ie_next;	/* next CB */
};

/*
 * CB commands (ie_cmd)
 */
#define	IE_NOP		0
#define	IE_IADDR	1	/* individual address setup */
#define	IE_CONFIG	2	/* configure */
#define	IE_MADDR	3	/* multicast address setup */
#define	IE_TRANSMIT	4	/* transmit */
#define	IE_TDR		5	/* TDR test */
#define	IE_DUMP		6	/* dump registers */
#define	IE_DIAGNOSE	7	/* internal diagnostics */

/*
 * TDR command block 
 */
struct ietdr {
	struct iecb ietdr_cb;	/* common command block */
	u_char	ietdr_timlo: 8;	/* time */
	u_char	ietdr_ok   : 1;	/* link OK */
	u_char	ietdr_xcvr : 1;	/* transceiver bad */
	u_char	ietdr_open : 1;	/* cable open */
	u_char	ietdr_shrt : 1;	/* cable shorted */
	u_char		   : 1;
	u_char	ietdr_timhi: 3;	/* time */
};

/*
 * Individual address setup command block
 */
struct ieiaddr {
	struct iecb ieia_cb;	/* common command block */
	char	ieia_addr[6];	/* the actual address */
};

/*
 * Configure command
 */
struct ieconf {
	struct iecb ieconf_cb;	/* command command block */
	u_char		   : 4;
	u_char	ieconf_bytes : 4;	/* # of conf bytes */
	u_char		   : 4;
	u_char	ieconf_fifolim : 4;	/* fifo limit */
	u_char	ieconf_savbf : 1;	/* save bad frames */
	u_char	ieconf_srdy  : 1;	/* srdy/ardy (?) */
	u_char		   : 6;
	u_char	ieconf_extlp : 1;	/* external loopback */
	u_char	ieconf_intlp : 1;	/* external loopback */
	u_char	ieconf_pream : 2;	/* preamble length code */
	u_char	ieconf_acloc : 1;	/* addr&type fields separate */
	u_char	ieconf_alen  : 3;	/* address length */
	u_char	ieconf_bof   : 1;	/* backoff method */
	u_char	ieconf_exprio : 3;	/* exponential prio */
	u_char		   : 1;
	u_char	ieconf_linprio : 3;	/* linear prio */
	u_char	ieconf_space : 8;	/* interframe spacing */
	u_char	ieconf_slttml : 8;	/* low bits of slot time */
	u_char	ieconf_retry : 4;	/* # xmit retries */
	u_char		   : 1;
	u_char	ieconf_slttmh : 3;	/* high bits of slot time */
	u_char	ieconf_pad    : 1;	/* flag padding */
	u_char	ieconf_hdlc   : 1;	/* HDLC framing */
	u_char	ieconf_crc16  : 1;	/* CRC type */
	u_char	ieconf_nocrc  : 1;	/* disable CRC appending */
	u_char	ieconf_nocarr : 1;	/* no carrier OK */
	u_char	ieconf_manch  : 1;	/* Manchester encoding */
	u_char	ieconf_nobrd  : 1;	/* broadcast disable */
	u_char	ieconf_promisc : 1;	/* promiscuous mode */
	u_char	ieconf_cdsrc  : 1;	/* CD source */
	u_char	ieconf_cdfilt : 3;	/* CD filter bits (?) */
	u_char	ieconf_crsrc  : 1;	/* carrier source */
	u_char	ieconf_crfilt : 3;	/* carrier filter bits */
	u_char	ieconf_minfrm : 8;	/* min frame length */
	u_char		   : 8;
};

/*
 * Receive frame descriptor
 */
struct	ierfd {
	u_char	ierfd_short	: 1;	/* short frame */
	u_char	ierfd_noeof	: 1;	/* no EOF (bitstuffing mode only) */
	u_char			: 6;	/* unused */
	u_char	ierfd_done	: 1;	/* command done */
	u_char	ierfd_busy	: 1;	/* command busy */
	u_char	ierfd_ok	: 1;	/* command successful */
	u_char			: 1;	/* unused */
	u_char	ierfd_crcerr	: 1;	/* crc error */
	u_char	ierfd_align	: 1;	/* alignment error */
	u_char	ierfd_nospace	: 1;	/* out of buffer space */
	u_char	ierfd_overrun	: 1;	/* DMA overrun */
	u_char			: 8;	/* unused */
	u_char	ierfd_el	: 1;	/* end of list */
	u_char	ierfd_susp	: 1;	/* suspend when done */
	u_char			: 6;	/* unused */
	ieoff_t	ierfd_next;		/* next RFD */
	ieoff_t	ierfd_rbd;		/* pointer to buffer descriptor */
	u_char	ierfd_dhost[6];		/* destination address field */
	u_char	ierfd_shost[6];		/* source address field */
	u_short	ierfd_type;		/* Ethernet packet type field */
};

/*
 * Receive buffer descriptor
 */
struct	ierbd {
	u_char	ierbd_cntlo	: 8;	/* Low order 8 bits of count */
	u_char	ierbd_eof	: 1;	/* last buffer for this packet */
	u_char	ierbd_used	: 1;	/* EDLC sets when buffer is used */
	u_char	ierbd_cnthi	: 6;	/* High order 6 bits of count */
	ieoff_t	ierbd_next;		/* next RBD */
	ieaddr_t ierbd_buf;		/* pointer to buffer */
	u_char	ierbd_sizelo	: 8;	/* Low order 8 bits of buffer size */
	u_char	ierbd_el	: 1;	/* end-of-list if set */
	u_char			: 1;	/* unused */
	u_char	ierbd_sizehi	: 6;	/* High order 6 bits of buffer size */
#ifdef KERNEL
	struct ieipack *ierbd_iep;	/* ptr to data packet descriptor */
#endif
};

/*
 * Transmit frame descriptor ( Transmit command block )
 */
struct	ietfd {
	u_char	ietfd_defer	: 1;	/* transmission deferred */
	u_char	ietfd_heart	: 1;	/* Heartbeat */
	u_char	ietfd_xcoll	: 1;	/* Too many collisions */
	u_char			: 1;	/* unused */
	u_char	ietfd_ncoll	: 4;	/* Number of collisions */
	u_char	ietfd_done	: 1;	/* command done */
	u_char	ietfd_busy	: 1;	/* command busy */
	u_char	ietfd_ok	: 1;	/* command successful */
	u_char	ietfd_aborted	: 1;	/* command aborted */
	u_char			: 1;	/* unused */
	u_char	ietfd_nocarr	: 1;	/* No carrier sense */
	u_char	ietfd_nocts	: 1;	/* Lost Clear to Send */
	u_char	ietfd_underrun	: 1;	/* DMA underrun */
	u_char			: 5;	/* unused */
	u_char	ietfd_cmd	: 3;	/* command # */
	u_char	ietfd_el	: 1;	/* end of list */
	u_char	ietfd_susp	: 1;	/* suspend when done */
	u_char	ietfd_intr	: 1;	/* interrupt when done */
	u_char			: 5;	/* unused */
	ieoff_t	ietfd_next;		/* next RFD */
	ieoff_t	ietfd_tbd;		/* pointer to buffer descriptor */
	u_char	ietfd_dhost[6];		/* destination address field */
	u_short	ietfd_type;		/* Ethernet packet type field */
};

/*
 * Transmit buffer descriptor
 */
struct	ietbd {
	u_char	 ietbd_cntlo	: 8;	/* Low order 8 bits of count */
	u_char	 ietbd_eof	: 1;	/* last buffer for this packet */
	u_char			: 1;	/* unused */
	u_char	 ietbd_cnthi	: 6;	/* High order 6 bits of count */
	ieoff_t	 ietbd_next;		/* next TBD */
	ieaddr_t ietbd_buf;		/* pointer to buffer */
};
