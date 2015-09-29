
/*	@(#)promether.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define ETHERMEM	0x080000	/* must be top 16 MB of memory */	
#define BCLEAR(x)	bzero((x), sizeof(*(x)))

/*
 * Register definitions for the Sun-2 On-board version of the
 * Intel EDLC based Ethernet interface
 */

struct ereg {
	u_char	obie_noreset	: 1;	/* R/W: Ethernet chips reset */
	u_char	obie_noloop	: 1;	/* R/W: loopback */
	u_char	obie_ca		: 1;	/* R/W: channel attention */
	u_char	obie_ie		: 1;	/* R/W: interrupt enable */
	u_char			: 1;	/* R/O: unused */
	u_char	obie_level2	: 1;	/* R/O: 0=Level 1 xcvr, 1=Level 2 */
	u_char	obie_buserr	: 1;	/* R/O: Ether DMA got bus error */
	u_char	obie_intr	: 1;	/* R/O: interrupt request */
};

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *	blame kevin sheehan Thu Dec 29 17:46:12 PST 1983
 *	Then blame John Gilmore for leaving it this way!
 *	then blame kevin for the prom version?
 *
 */

#define ETHERBUFSIZE		0x2000
#define	MEMPAGESIZE		0x2000	
#define NTXBUF			15
#define NRXBUF			NTXBUF
#define DUMPSCB			1			/* for etherdump */
#define DUMPCB			2
#define DUMPTDR			4
#define DUMPRFD			8
#define DUMPRBD			0x10
#define DUMPSTATS		0x20
#define DUMPTBD			0x40
#define DUMPRXBUF		0x80
#define DUMPALL			-1

/*
 * Data types peculiar to the Intel Ethernet chip
 * These types of data MUST be manipulated by calls to to_iexxx
 * and from_iexxx, since their representation changes from the
 * (non-byte-swapped) Multibus board to the (byte-swapped) Model 50 board.
 */
typedef unsigned short mieoff_t;		/* offsets from iscp_base */
typedef unsigned short mieint_t;		/* 16 bit integers */
typedef unsigned long  mieaddr_t;		/* 24-bit addresses */

mieoff_t to_mieoff();
mieaddr_t to_mieaddr();
caddr_t from_mieoff();
caddr_t from_mieaddr();

mieint_t to_ieint();
#define	from_ieint	(unsigned short) to_ieint


/*
 * Now for the control structures in shared memory
 */

/*
 * System Configuration Pointer
 * This is at 0xFFFFF6 in the EDLC chip's address space.
 */
struct	escp {
	u_short		scp_sysbus;	/* one byte or the other 
					   determines bus width */
	u_long		dummy1;
	mieaddr_t	iscp_addr;	/* bizzare address of iscp */
};

/*
 * Intermediate System Configuration Pointer
 * Specifies base of all other control blocks and the offset of the SCB.
 */
struct	eiscp {	
#ifdef NOSWAB
	u_char		dummy2;
	u_char		iscp_busy;	/* 1=> still initializing */
#else  NOSWAB
	u_char		iscp_busy;	/* 1=> still initializing */
	u_char		dummy2;
#endif NOSWAB
	mieoff_t		iscb_offset;	/* offset of scb */
	mieaddr_t	iscb_base;	/* base for all offsets */
};


/*
 * System Control Block -- the focus of communication with the EDLC
 * Head of command blocks, and holder of all sacred.
 */
struct	escb {
#ifdef NOSWAB
	unsigned	scb_cx:1;	/* cmd with I-bit set has been done */
	unsigned	scb_fr:1;	/* frame received */
	unsigned	scb_cnr:1;	/* command unit became not ready */
	unsigned	scb_rnr:1;	/* receive unit became not ready */
	unsigned	scb_zero0:1;	/* must be zero */
	unsigned	scb_cus:3;	/* command unit status */

	unsigned	scb_zero1:1;	/* must be zero */
	unsigned	scb_rus:3;	/* receive unit status */
	unsigned	scb_zero2:4;	/* must be zero */

	unsigned	scb_cxack:1;	/* ack cx above */
	unsigned	scb_frack:1;	/* ack frame received */
	unsigned	scb_cnrack:1;	/* ack command unit became not ready */
	unsigned	scb_rnrack:1;	/* ack receive unit became not ready */
	unsigned	scb_zero4:1;	/* must be zero */
	unsigned	scb_cuc:3;	/* command unit command */

	unsigned	scb_reset:1;	/* reset chip */
	unsigned	scb_ruc:3;	/* receive unit command */
	unsigned	scb_zero5:4;	/* must be zero */
#else  NOSWAB
	unsigned	scb_zero1:1;	/* must be zero */
	unsigned	scb_rus:3;	/* receive unit status */
	unsigned	scb_zero2:4;	/* must be zero */

	unsigned	scb_cx:1;	/* cmd with I-bit set has been done */
	unsigned	scb_fr:1;	/* frame received */
	unsigned	scb_cnr:1;	/* command unit became not ready */
	unsigned	scb_rnr:1;	/* receive unit became not ready */
	unsigned	scb_zero0:1;	/* must be zero */
	unsigned	scb_cus:3;	/* command unit status */

	unsigned	scb_reset:1;	/* reset chip */
	unsigned	scb_ruc:3;	/* receive unit command */
	unsigned	scb_zero5:4;	/* must be zero */

	unsigned	scb_cxack:1;	/* ack cx above */
	unsigned	scb_frack:1;	/* ack frame received */
	unsigned	scb_cnrack:1;	/* ack command unit became not ready */
	unsigned	scb_rnrack:1;	/* ack receive unit became not ready */
	unsigned	scb_zero4:1;	/* must be zero */
	unsigned	scb_cuc:3;	/* command unit command */
#endif NOSWAB

	mieoff_t		scb_cbloff;	/* command block list offset */
	mieoff_t		scb_rfaoff;	/* received frame aread offset */
	mieint_t		scb_crcerrs;	/* # crc errors */
	mieint_t		scb_alnerrs;	/* # mis-aligned frames w/bad CRC */
	mieint_t		scb_rscerrs;	/* # frames tossed due to resources */
	mieint_t		scb_ovrnerrs;	/* # frames tossed due to bus loss */
};

/*
 * Generic command block
 */
struct	ecb {			/* standard command block header */
#ifdef NOSWAB
	unsigned	cb_c:1;		/* command done (set 0 by us) */
	unsigned	cb_b:1;		/* busy */
	unsigned	cb_ok:1;	/* command went ok */
	unsigned	cb_abort:1;	/* command was aborted */
	unsigned	cb_stat_hi:4;	/* other status */
	unsigned	cb_stat_lo:8;	/* Part of status */

	unsigned	cb_el:1;	/* command is end of list */
	unsigned	cb_s:1;		/* suspend CU when done this command */
	unsigned	cb_i:1;		/* interrupt when done (sets scb_cx) */
	unsigned		:10;
	unsigned	cb_cmd:3;	/* command to be done */
#else  NOSWAB
	unsigned	cb_stat_lo:8;	/* Part of status */

	unsigned	cb_c:1;		/* command done (set 0 by us) */
	unsigned	cb_b:1;		/* busy */
	unsigned	cb_ok:1;	/* command went ok */
	unsigned	cb_abort:1;	/* command was aborted */
	unsigned	cb_stat_hi:4;	/* other status */

	unsigned 		:5;
	unsigned	cb_cmd:3;	/* command to be done */

	unsigned	cb_el:1;	/* command is end of list */
	unsigned	cb_s:1;		/* suspend CU when done this command */
	unsigned	cb_i:1;		/* interrupt when done (sets scb_cx) */
	unsigned		:5;
#endif NOSWAB
	mieoff_t		cb_link;	/* offset of next block */
};

/*
 * TDR command block extension
 */
struct tdr_ext {	/*  tdr status part of command block */
#ifdef NOSWAB
	unsigned	tdr_linkok:1;	/* was link ok ?? */
	unsigned	tdr_xcvrprob:1;	/* was there a transceiver problem? */
	unsigned	tdr_open:1;	/* was there an open on the net ?? */
	unsigned	tdr_short:1;	/* was there a short on the net ?? */
	unsigned	tdr_dummy:1;	/* NU */
	unsigned	tdr_time_hi:3;	/* time to problem */
	unsigned	tdr_time_lo:8;	/* time to problem */
#else  NOSWAB
	unsigned	tdr_time_lo:8;	/* time to problem */

	unsigned	tdr_linkok:1;	/* was link ok ?? */
	unsigned	tdr_xcvrprob:1;	/* was there a transceiver problem? */
	unsigned	tdr_open:1;	/* was there an open on the net ?? */
	unsigned	tdr_short:1;	/* was there a short on the net ?? */
	unsigned	tdr_dummy:1;	/* NU */
	unsigned	tdr_time_hi:3;	/* time to problem */
#endif NOSWAB
};

/*
 * Configure command extension.
 */

struct conf_ext {
#ifdef NOSWAB
	unsigned	:4;			/* NU */
	unsigned	conf_fifolim:4;		/* fifo limit */

	unsigned	:4;			/* NU */
	unsigned	conf_count:4;		/* length in bytes of config params */

	unsigned	conf_extloop:1;		/* external loopback */
	unsigned 	conf_intloop:1;		/* internal loopback */
	unsigned	conf_preamlen:2;	/* preamble length */
	unsigned	conf_acloc:1;		/* 1 = address and type in buffers */
	unsigned	conf_addrlen:3;		/* length of address */

	unsigned	conf_savebad:1;		/* save bad frames */
	unsigned	conf_srdy:1;		/* internal/external sync */
	unsigned		:6;

	u_char		conf_framespace;	/* interframe spacing period */

	unsigned	conf_backoff:1;		/* backoff method */
	unsigned	conf_exppri:3;		/* exponential priority */
	unsigned		:1;
	unsigned	conf_linpri:3;		/* linear priority */

	unsigned	conf_numretries:4;	/* number of retries */
	unsigned		:1;
	unsigned	conf_slottime_hi:3;	/* slot time */
	unsigned	conf_slottime_lo:8;	/* slot time */

	unsigned	conf_cdtsrc:1;		/* collision detect source */
	unsigned	conf_cdtf:3;		/* collision detect filter bits */
	unsigned	conf_crssrc:1;		/* carrier sense source */
	unsigned	conf_crsf:3;		/* carrier sense fileter bits */

	unsigned	conf_pad:1;		/* do padding ?? */
	unsigned	conf_bitstuff:1;	/* HDLC bitstuffing mode ?? */
	unsigned	conf_crctype:1;		/* crc type */
	unsigned	conf_nocrc:1;		/* crc generation inhibit */
	unsigned	conf_tonocrs:1;		/* xmit on no CRS */
	unsigned	conf_manchester:1;	/* nrz/manch encoding */
	unsigned	conf_nobroadcast:1;	/* reject broadcast frames */
	unsigned	conf_promiscuous:1;	/* mode accepts ALL frames */

	unsigned	:8;			/* NU */

	u_char		conf_minframelen;	/* minimum frame length */
#else  NOSWAB
	unsigned	:4;			/* NU */
	unsigned	conf_count:4;		/* bytes of config params */

	unsigned	:4;			/* NU */
	unsigned	conf_fifolim:4;		/* fifo limit */

	unsigned	conf_savebad:1;		/* save bad frames */
	unsigned	conf_srdy:1;		/* internal/external sync */
	unsigned	:6;			/* NU */

	unsigned	conf_extloop:1;		/* external loopback */
	unsigned 	conf_intloop:1;		/* internal loopback */
	unsigned	conf_preamlen:2;	/* preamble length */
	unsigned	conf_acloc:1;		/* 1 = address&type in bufs */
	unsigned	conf_addrlen:3;		/* length of address */

	unsigned	conf_backoff:1;		/* backoff method */
	unsigned	conf_exppri:3;		/* exponential priority */
	unsigned	:1;			/* NU */
	unsigned	conf_linpri:3;		/* linear priority */

	u_char		conf_framespace;	/* interframe spacing period */

	unsigned	conf_slottime_lo:8;	/* slot time */

	unsigned	conf_numretries:4;	/* number of retries */
	unsigned	:1;			/* NU */
	unsigned	conf_slottime_hi:3;	/* slot time */

	unsigned	conf_pad:1;		/* do padding ?? */
	unsigned	conf_bitstuff:1;	/* HDLC bitstuffing mode ?? */
	unsigned	conf_crctype:1;		/* crc type */
	unsigned	conf_nocrc:1;		/* crc generation inhibit */
	unsigned	conf_tonocrs:1;		/* xmit on no CRS */
	unsigned	conf_manchester:1;	/* nrz/manch encoding */
	unsigned	conf_nobroadcast:1;	/* reject broadcast frames */
	unsigned	conf_promiscuous:1;	/* mode accepts ALL frames */

	unsigned	conf_cdtsrc:1;		/* collision detect source */
	unsigned	conf_cdtf:3;		/* collision det. filter bits */
	unsigned	conf_crssrc:1;		/* carrier sense source */
	unsigned	conf_crsf:3;		/* carrier sense fileter bits */

	u_char		conf_minframelen;	/* minimum frame length */

	unsigned	:8;			/* NU */
#endif NOSWAB
};

#define ADDRLEN		6

typedef	char	etheraddr[ADDRLEN];		/* could need swapping */

struct tx_ext {					/* for cb extension block */
	mieoff_t		tx_bdptr;	/* address of TBD */
	etheraddr	tx_addr;	/* destination ethernet address */
	u_short		tx_type;	/* user defined type field */
};

/*
 * Transmit control block, again???
 */
struct etxcb {					/* for linked use */
#ifdef NOSWAB
	unsigned	tx_c:1;		/* command done (set 0 by us) */
	unsigned	tx_b:1;		/* busy */
	unsigned	tx_ok:1;	/* command went ok */
	unsigned	tx_abort:1;	/* command was aborted */
	unsigned	:1;		/* not used */
	unsigned	tx_nocarrier:1;	/* carrier dropped, transmission stopped */
	unsigned	tx_nocts:1;	/* transmission stopped, no clear to send */
	unsigned	tx_underrun:1;	/* dma underrun */
	unsigned	tx_deferred:1;	/* transmission deferred */
	unsigned	tx_heartbeat:1;	/* CDT sensed in interframe period */
	unsigned	tx_bumps:1;	/* too many collisions (counter timed out) */
	unsigned	:1;		/* not used */
	unsigned	tx_numbumps:4;	/* number of collisions in this frame */
	unsigned	tx_el:1;	/* command is end of list */
	unsigned	tx_s:1;		/* suspend CU when done with this command */
	unsigned	tx_i:1;		/* interrupt when done (sets scb_cx) */
	unsigned	tx_dummy:10;	/* NU */
	unsigned	tx_cmd:3;	/* command to be done  will be CMD_TRANSMIT */
#else  NOSWAB
	unsigned	tx_deferred:1;	/* transmission deferred */
	unsigned	tx_heartbeat:1;	/* CDT sensed in interframe period */
	unsigned	tx_bumps:1;	/* too many collisions (counter timed out) */
	unsigned	:1;		/* not used */
	unsigned	tx_numbumps:4;	/* number of collisions in this frame */

	unsigned	tx_c:1;		/* command done (set 0 by us) */
	unsigned	tx_b:1;		/* busy */
	unsigned	tx_ok:1;	/* command went ok */
	unsigned	tx_abort:1;	/* command was aborted */
	unsigned	:1;		/* not used */
	unsigned	tx_nocarrier:1;	/* carrier dropped, transmission stopped */
	unsigned	tx_nocts:1;	/* transmission stopped, no clear to send */
	unsigned	tx_underrun:1;	/* dma underrun */

	unsigned		:5;	/* NU */
	unsigned	tx_cmd:3;	/* command (== CMD_TRANSMIT) */

	unsigned	tx_el:1;	/* command is end of list */
	unsigned	tx_s:1;		/* suspend CU when done with this command */
	unsigned	tx_i:1;		/* interrupt when done (sets scb_cx) */
	unsigned		:5;	/* NU */
#endif NOSWAB
	mieoff_t		tx_link;	/* offset of next block */
	mieoff_t		tx_bdptr;	/* buffer descriptor offset */
	etheraddr	tx_addr;	/* destination ethernet address */
	u_short		tx_type;	/* user defined type field */
};

/*
 * Transmit Buffer Descriptor
 */
struct etbd {
#ifdef NOSWAB
	unsigned	tbd_eof:1;	/* end of this frame */
	unsigned	:1;		/* NU */
	unsigned	tbd_count_hi:6;	/* count of bytes in this buffer */
	unsigned	tbd_count_hi:8;	/* count of bytes in this buffer */
#else  NOSWAB
	unsigned	tbd_count_lo:8;	/* count of bytes in this buffer */
	unsigned	tbd_eof:1;	/* end of this frame */
	unsigned	:1;		/* NU */
	unsigned	tbd_count_hi:6;	/* count of bytes in this buffer */
#endif NOSWAB
	mieoff_t		tbd_link;	/* offset of next tbd */
	mieaddr_t	tbd_addr;	/* address of data to be sent */
};

/*
 * Receive Frame Descriptor
 */
struct erfd {
#ifdef NOSWAB
	unsigned	rfd_c:1;	/* frame reception complete */
	unsigned	rfd_b:1;	/* this frame busy */
	unsigned	rfd_ok:1;	/* frame received with no errors */
	unsigned	:1;		/* NU */
	unsigned	rfd_crc:1;	/* crc error in aligned frame */
	unsigned	rfd_align:1;	/* CRC error in misaligned frame */
	unsigned	rfd_buffer:1;	/* ran out of buffer space */
	unsigned	rfd_overrun:1;	/* DMA overrun */

	unsigned	rfd_short:1;	/* frame too short */
	unsigned	rfd_noeof:1;	/* no EOF (bitstuffing only) */
	unsigned	:6;		/* NU */

	unsigned	rfd_el:1;	/* last RFD on RDL */
	unsigned	rfd_s:1;	/* suspend after receiving this frame */
	unsigned		:6;	/* NU */

	unsigned		:8;	/* NU */
#else  NOSWAB
	unsigned	rfd_short:1;	/* frame too short */
	unsigned	rfd_noeof:1;	/* no EOF (bitstuffing only) */
	unsigned		:6;

	unsigned	rfd_c:1;	/* frame reception complete */
	unsigned	rfd_b:1;	/* this frame busy */
	unsigned	rfd_ok:1;	/* frame received with no errors */
	unsigned		:1;
	unsigned	rfd_crc:1;	/* crc error in aligned frame */
	unsigned	rfd_align:1;	/* CRC error in misaligned frame */
	unsigned	rfd_buffer:1;	/* ran out of buffer space */
	unsigned	rfd_overrun:1;	/* DMA overrun */

	unsigned		:8;

	unsigned	rfd_el:1;	/* last RFD on RDL */
	unsigned	rfd_s:1;	/* suspend after receiving this frame */
	unsigned		:6;
#endif NOSWAB
	mieoff_t		rfd_link;	/* offset of next RFD */
	mieoff_t		rfd_bdptr;	/* offset of RBD */
	etheraddr	rfd_dest;	/* destination address */
	etheraddr	rfd_source;	/* source address */
	u_short		rfd_type;	/* user defined type field */
};

/*
 * Receive Buffer Descriptor
 */
struct erbd {
#ifdef NOSWAB
	unsigned	rbd_eof:1;	/* last buffer of this frame */
	unsigned	rbd_f:1;	/* buffer has been used */
	unsigned	rbd_count_hi:6;	/* count of meaningful bytes */
	unsigned	rbd_count_lo:8;	/* count of meaningful bytes */
	mieoff_t		rbd_bdptr;	/* offset of next rbd */
	mieaddr_t	rbd_addr;	/* address of buffer */
	unsigned	rbd_el:1;	/* buffer is last one in FBL */
	unsigned	:1;		/* NU */
	unsigned	rbd_size_hi:6;	/* size of buffer */
	unsigned	rbd_size_lo:8;	/* size of buffer */
#else  NOSWAB
	unsigned	rbd_count_lo:8;	/* count of meaningful bytes */

	unsigned	rbd_eof:1;	/* last buffer of this frame */
	unsigned	rbd_f:1;	/* buffer has been used */
	unsigned	rbd_count_hi:6;	/* count of meaningful bytes */

	mieoff_t		rbd_bdptr;	/* offset of next rbd */
	mieaddr_t	rbd_addr;	/* address of buffer */

	unsigned	rbd_size_lo:8;	/* size of buffer */

	unsigned	rbd_el:1;	/* buffer is last one in FBL */
	unsigned	:1;		/* NU */
	unsigned	rbd_size_hi:6;	/* size of buffer */
#endif NOSWAB
};

/*
 *	some handy defines, like commands and status returned.
 */

#define	CUS_IDLE	0
#define	CUS_SUSPENDED	1
#define	CUS_READY	2

#define RUS_IDLE	0
#define RUS_SUSPENDED	1
#define RUS_NORESOURCES	2
#define RUS_READY	4

#define CUC_NOP		0
#define CUC_START	1
#define CUC_RESUME	2
#define	CUC_SUSPEND	3
#define CUC_ABORT	4

#define	RUC_NOP		0
#define RUC_START	1
#define	RUC_RESUME	2
#define RUC_SUSPEND	3
#define RUC_ABORT	4

#define CMD_NOP		0
#define CMD_IASETUP	1
#define	CMD_CONFIGURE	2
#define CMD_MCSETUP	3
#define CMD_TRANSMIT	4
#define CMD_TDR		5
#define CMD_DUMP	6
#define CMD_DIAGNOSE	7

/*
 *	this stuff is the declaration of our use of the ethernet
 *	board.
 */
struct etherblock {
	u_char		ethertype;		/* =50 for Model 50 */
	u_char		verydumfill[3];		/* To long-align dummyfill */
	char dummyfill[MEMPAGESIZE - sizeof(struct escp) - 4*sizeof(u_char)];
	struct escp	scp;			/* system config pointer */
	struct eiscp	iscp;			/* intermediate sys control */
	struct escb	scb;			/* system control block */
	struct cbl {				/* command block list */
		struct ecb	cb;		/* command block */
		union cb_ext {			/* command extensions */
			struct tdr_ext	tdr;	/* time domain reflectometer */
			etheraddr	iaddr;	/* individual address setup */
			struct conf_ext	conf;	/* configuration (see &conf.h)*/
			struct tx_ext	tx;	/* transmit */
		} cb_ext;
	} cbl[NTXBUF+2];
	struct etbd	tbd[NTXBUF];		/* tx buffer descriptor */
	struct erfd	rfd[NRXBUF];		/* rx frame descriptor */
	struct erbd	rbd[NRXBUF+2];		/* rx buffer descriptor */
	u_char		txbuf[NTXBUF][ETHERBUFSIZE];	/* tx buffers */
	u_char		rxbuf[NRXBUF][ETHERBUFSIZE];	/* rx buffers */
	u_char		rxbogus[2][16];		/* for bogus b0-step actions */
};
