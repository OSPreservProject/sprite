/*
 * Interface definitions for the Array
 *
 * $Header$
 */

#ifndef _IOPB
#define _IOPB

struct iopb {
	char	command;	/* basic operation to be performed */
	char	option;		/* modifies 'command' in some cases */
	char	priority;
	char    reserved1;      /* unused */
	char	flags;
#define			CHECKSUM2	0x01	/* 2-byte checksums */
#define			CHECKSUM4	0x02	/* 4-byte checksums */
#define			RESTARTED_IOPB	0x04	/* from dead controller */
#define			WAIT_FOR_EVENT	0x08	/* applies to some cmds */

	u_int   log_array:2;      /* the logical array number */
	u_int   reserved2:6;    /* currently unused */

	u_short	id;		/* logical volume, redundancy group, */
				/* logical disk, or controller/scsi_id */
				/* depending on the command */

	u_long	logical_block;	/* block to be read/written */
	u_long	byte_count;	/* # of bytes to be transferred */
	char	*buffer;	/* points to data (or scatter/gather list) */
	char	buf_addr_mod;	/* address modifier for buffer */

	char	intr_level;	/* 0 => no int. (if possible) */
	char	intr_vector;

	char	auxbuf_mod;	/* address modifier for the auxiliary buffer */
	u_short	sg_entries;	/* # of normal scatter/gather entries */
	u_short	auxbuf_size;	/* size of the buffer area */
	char	*aux_buffer;	/* auxiliary buf: checksums, pass-thru, etc */
};

/* The following macro forms an 'id' value from a controller and scsi id */
#define	DISK_ID(ctrlr, scsi_id)	(((ctrlr) << 8) | ((scsi_id) & 0xff))

/*
 * IOPB command and option values
 */
#define	CMD_READ	0	/* read blocks from a logical volume */
#define	CMD_WRITE	1	/* write blocks to a logical volume */
#define	CMD_INIT_GROUP	2	/* initialize a redundancy group */
#define	CMD_READ_ENV	3	/* read the environment registers */
#define	CMD_WRITE_ENV	4	/* write the environment registers */

#define	CMD_READ_CONF	5	/* read the system configuration */
#define	CMD_WRITE_CONF	6	/* write the system configuration */
		/* options for read/write configuration commands */
#define		OPT_SYSTEM_INFO	0	/* system hardware information */
#define         OPT_DEVICE_INFO 1       /* device information */
#define		OPT_SYSVARS	2	/* system variables */
#define		OPT_DISK_CONFIG	3	/* layout of disks in the array */
#define         OPT_ADD_DISKS   4       /* add disks to the config */
#define		OPT_SW_LOAD1	5	/* primary software load area */
#define		OPT_SW_LOAD2	6	/* secondary sw load area */
#define         OPT_SW_DIAG     7       /* diagnostic sw load area */
#define		OPT_SCRATCHPAD	8	/* host scratchpad area */

#define	CMD_PASS_THRU	7	/* CDB pass-through command */

#define	CMD_READ_STATS	8	/* read performance stats */
#define		OPT_NO_CLEAR	0	/* don't clear the stats structure */
#define		OPT_CLEAR	1	/* clear the stats after reading */

#define	CMD_READ_ELOG	9	/* read an error log entry */
#define		OPT_DESTRUCT	0	/* destructive read (inc. read ptr.) */
#define		OPT_NONDESTRUCT	1	/* non-destructive read */

#define	CMD_SET_LEVEL	11	/* set the Array's run level */
#define		OPT_BOOT_PROF	0	/* boot production code */
#define		OPT_BOOT_DIAG	1	/* boot diagnostic code */
#define		OPT_FINISH_BOOT	2	/* begin normal Array operation */
#define         OPT_IDLE        3       /* idle array for config changes */
#define		OPT_HALT        4       /* halt the Array */
#define		OPT_REBOOT	5	/* reboot the Array */

#define	CMD_DIAGNOSTICS	12	/* run a diagnostic test */
#define		OPT_LB_BUFFER	0	/* data loopback buffer test */
#define		OPT_LB_CHANNEL	1	/* data loopback channel test */
#define		OPT_LB_DRIVE	2	/* data loopback drive test */
#define		OPT_SURFACE_TST	3	/* drive surface analysis test */
#define		OPT_HOST_INTFCE	4	/* host interface test */
#define		OPT_P_PARITY	5	/* P-parity test */
#define		OPT_RD_REVISION	6	/* read revision */

#define CMD_VERIFY_GROUP 14	/* verify a redundancy group */
#define	CMD_ACTIVATE_SP	15	/* activate a spare drive */
#define	CMD_INSTALL_DRV	16	/* install a new drive */

#define CMD_GET_TIME 18         /* read the TOD clock */
#define CMD_SET_TIME 19         /* set the TOD clock */

/*
 * Pointed to by 'buffer' if sg_entries is non-zero.
 */
struct sg {
	char	*buffer;
	u_long	buf_addr_mod:8,
		byte_count:24;
};

/*
 * Pointed to by aux_buffer if the command is CMD_PASS_THRU
 */
struct pass_thru_block {
	u_long	timeout_value;	/* timeout (in sec.) for this command */
	u_short reserved;
	char direction;	/* data direction for this command */
#define			PASS_THRU_NO_DATA	0
#define			PASS_THRU_READ		1
#define			PASS_THRU_WRITE		2
	char cdb_len;           /* length of following CDB */
	char	cdb[12];	/* contains the SCSI CDB */
};


/*
 * The following defines relate to the operation of the FIFO.
 */
#define	MAX_IOPBS	127	/* maximum number of IOPBs that may be used */
#define	FIFO_VALID	0x800	/* set if valid data was read from the FIFO */
#define	IOPB_ERROR	0x080	/* set on a normal reply to indicate an error */
#define	INDEX_MASK	0x7f	/* mask for the bits that form the index part */

/*
 * if an IOPB fails due to DMA problems, DMA_ERROR is sent through the
 * FIFO followed by one of the next two error codes and the index of
 * the IOPB that failed.
 */
#define	DMA_ERROR	0x7f	/* pass through FIFO if array can't DMA */
#define		IOPB_STATUS_ERR	1	/* couldn't return the status */


/*
 * The following structures are used when setting up the initial
 * communication between the host and the array.
 */

/*
 * The IOPB table is an array of these...
 */
struct iopb_table {
	struct iopb 	*iopb_ptr;
	char		iopb_addr_mod;
	char		fill[3];
};

/*
 * The Status table is an array of these...
 */
struct status_table {
	struct status 	*status_ptr;
	char		status_addr_mod;
	char		fill[3];
};

/*
 * Status entries are pointed to by the entries in the status table. The
 * status structures pointed to correspond to the IOPBs of the same number.
 */
struct status {
	u_long	status;			/* primary error code */
	u_long	status2;		/* optional additional information */
	char	scsi_status;		/* SCSI completion status */
	char	fill1, fill2, fill3;
	char	request_sense[32];	/* SCSI request sense data */
};


/*
 * This structure gets pushed through the FIFO when the system boots
 * so that IOPBs can be passed around using only their index. It tells
 * the Array where to find IOPBs and status blocks in host memory, and
 * gives the Array information about how DMA should be performed.
 */
#define	TABLE_HEADER	0x7f	/* send this value ahead of the table desc. */

struct table_desc {

	struct iopb_table	*iopb_list;	/* ptr to iopb table array */
	struct status_table	*status_list;	/* ptr to status block array */

	char		iopb_list_mod;		/* address mod. for above */
	char		status_list_mod;
	char		num_iopbs;		/* # of entries for both */
	char		flags;                  /* currently unused */

	u_short		rdma_burst_sz;		/* DMA burst size for reads */
	u_short		wdma_burst_sz;		/* DMA burst size for writes */
	u_short		vme_timeout;		/* Bus timeout (in ms) */
	u_short		reserved;
};


/*
 * The remainder of this file contains data structures used in various
 * IOPB commands.
 */

/*
 * CMD_READ_ENV
 * CMD_WRITE_ENV
 */

#if 0
struct environ_regs {
	/*
	 * Currently undefined
	 */
};
#endif


/*
 * CMD_READ_CONF
 * CMD_WRITE_CONF
 */

/*
 *	OPT_SYSTEM_INFO
 */

#define	DEV_PER_CHAN	8	/* possible drives per channel */
#define	NUM_CHAN		12	/* total # of channels available */

typedef struct {
	struct {
		struct {
			u_long	present;

			struct {
				u_long	present;
				u_long	device_type;
				u_long	num_blocks;
				u_long	block_size;
			} dev[DEV_PER_CHAN];
		} chan[NUM_CHAN];
	} scsi_info;

	struct {
		u_long	mem_size;
		u_long	icache_size;
		u_long	dcache_size;
		u_long	hw_type;
		u_long	controller_id;
		u_long	system_sn;
		u_long	controller_sn;
	} hw_info;
} SYSTEM_INFO;


/*
 *	OPT_DEVICE_INFO
 */

typedef struct {

	u_long	type;
	char	sn[16];

	u_long	state;
} DEVICE_INFO[NUM_CHAN][DEV_PER_CHAN];


/*
 *	OPT_SYSVARS
 */


/*
 *	OPT_DISK_CONFIG
 */

#define	MAX_LOG_DISKS	409	/* maximum number of logical disks allowed */

struct logical_disk {
	u_short	controller;	/* SCSI controller number */
	u_short	scsi_id;	/* scsi drive id on the controller */
	u_long	block_offset;	/* where the logical_disk starts */
	u_long	num_blocks;	/* size of the logical_disk */
	u_long	block_size;	/* block size for this drive */

	u_short	type;
#define	    DISK	0	/* configured disk */
#define	    COLD_SPARE	2
#define	    WARM_SPARE	3
#define	    BROKEN	4	/* drive has been replaced by a spare */

	u_short	status;
#define	    ONLINE	0
#define	    OFFLINE	1
#define	    REBUILDING	2
};

/*
 * The redundancy group array is a list of values with the following format:
 *
 *	flags		flags for this redundancy group
 *	p1		number of the first logical disk in the group
 *	p2		second logical disk
 *	...		more logical disks
 *	END_MARKER	end of the group
 *
 * Each entry is a 16-bit value. This sequence is repeated for each
 * redundancy group, and the list is terminated by two consecutive
 * END_MARKERs.
 *
 * The flags and end markers are defined as:
 */
#define	RGRP_SIZE	2048	/* maximum number of entries in the array */

#define	P_REDUNDANCY	0x0001
#define	Q_REDUNDANCY	0x0002	/* not supported in this release */
#define	RG_INITIALIZED	0x0004	/* true (on read) if r.g. is initialized */

#define	END_MARKER	0xffff

/*
 * The logical volume array is a list of values with the following format:
 * This IOPB uses a buffer of up to 4k in size with the following format:
 *
 *	flags		flags/depth for this logical volume
 *	p1		number of the first logical disk in the volume
 *	p2		second logical disk
 *	...		more logical disks
 *	END_MARKER	end of the volume
 *
 * Each entry is a 16-bit value. This sequence is repeated for each
 * logical volume, and the list is terminated by two consecutive
 * END_MARKERs.
 *
 * The flags word contains an 8-bit depth value in the lower 8 bits. The
 * upper 8 bits are currently unused and reserved.
 */
#define	VOL_SIZE	2048	/* maximum number of entries in the array */


/*
 * This structure is used to transfer all information relating to logical
 * disks, redundancy groups, and logical volumes. This allows an entire
 * change of the configuration to made in one atomic operation.
 */
struct disk_config {
	int	num_log_disks;
	struct	logical_disk	log_disks[MAX_LOG_DISKS];
	u_short	volumes[VOL_SIZE];
	u_short	redun_groups[RGRP_SIZE];
};


/*
 * CMD_READ_STATS
 */


/*
 * CMD_READ_ELOG
 */

/*
 * The time-of-day clock
 */

typedef struct {
  unsigned int	year:8;
  unsigned int  month:8;
  unsigned int  day:8;
  unsigned int  hour:8;
  unsigned int	min:8;
  unsigned int  sec:8;
  unsigned int  ticks:16;
} TIME;


typedef	unsigned long	CRU;


/*
 * CRU types
 */
#define	CRU_DISK	0
#define	CRU_BATTERY	1
#define	CRU_BULK	2
#define	CRU_DCC		3
#define	CRU_FAN		4
#define	CRU_DENV	5
#define	CRU_PENV	6
#define	CRU_CONTROLLER	7
#define	CRU_IPI		8
#define	CRU_VME		9
#define	CRU_SCSI	10
#define	CRU_POWER_BKPLN	11
#define	CRU_DISK_BKPLN	12
#define	CRU_SCSI_CABLE	13

#define	CRU_NONE	100


#define MK_CRU(type, bp, major_id, minor_id) \
	(((type) << 24) | ((bp) << 16) | ((major_id) << 8) | (minor_id))

/*
 * The following macro is used when no CRU identifier is applicable.
 */
#define	NO_CRU	MK_CRU(CRU_NONE, 0, 0, 0)


/*
 * Macros to get the pieces of a CRU identifier
 */

#define	CRU_TYPE(cru)		(((cru) >> 24) & 0xff)
#define	CRU_CABINET(cru)	(((cru) >> 16) & 0xff)
#define	CRU_MAJOR_ID(cru)	(((cru) >>  8) & 0xff)
#define	CRU_MINOR_ID(cru)	((cru) & 0xff)

typedef struct {
	unsigned long		iopb_status;
	unsigned long		iopb_status2;
	unsigned int		scsi_status:8,
			reserved:24;
	union {
		struct {
			unsigned long	scsi_msg_byte:8,
				sense_key:8,
				error:8,
				:8;
			unsigned long	error_info;
		} extended;

		char	request_sense[32];
	} rqst_sense_data;
} STATUS_ENTRY;


#define	MAXFILENAME	32	/* max. file name length */
#define	MAXTASKNAME	8	/* max. task name length */
#define	MAXTEXTLEN	128	/* maximum size of a text message */
#define	NUMREGS		42	/* number of saved processor registers */

typedef enum {
    ED_NONE,			/* indicates the union is empty */
    ED_CDB,
    ED_TEXT,
    ED_STATUS,
    ED_CPU
} EDATA_TYPE;

typedef union {
	char		cdb[12];		/* ed_type == ED_CDB */
	STATUS_ENTRY	status;			/* ed_type == ED_STATUS */
	char		text[MAXTEXTLEN];	/* ed_type == ED_TEXT */
	unsigned long		cpu_regs[NUMREGS];	/* ed_type == ED_CPU */
	char		pad[184];
} ERROR_DATA;


typedef struct {
    /*
     * Information that specifies the type of error
     */
    unsigned long	error_code;
    unsigned long	error_code2;		/* optional, additional information */

    CRU		cru;			/* the affected CRU, if any */

    /*
     * Information about where the error was reported
     */
    char	file[MAXFILENAME];	/* source code file name */
    unsigned long	line;			/* and line number */
    char	task[MAXTASKNAME];	/* name of the reporting task */
    unsigned long	sw_release;		/* SW release being run */

    TIME	time;			/* when the error occurred */

    EDATA_TYPE	ed_type;		/* tells what is in the union below */
    ERROR_DATA	error_data;

} ERROR_REC;

#endif /* _IOPB */
