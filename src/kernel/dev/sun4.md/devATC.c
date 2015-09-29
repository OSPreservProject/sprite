/*
 * devATC.c --
 *
 *	Driver the for ATC controller.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun4.md/devATC.c,v 9.2 92/10/23 15:04:35 elm Exp $ SPRITE (Berkeley)";
#endif /* not lint */

/*  all include files in /sprite/lib/include unless otherwise specified */

#include <sys/types.h>
#include "sprite.h"          
#include "mach.h"            /* /sprite/src/mach/sun4.md */
#include "dev.h"             /* /sprite/src/kernel/dev */
#include "devInt.h"          /* /sprite/src/kernel/dev/sun4.md */
#include "devDiskLabel.h"    /* /sprite/src/kernel/dev */
#include "dbg.h"             /* /sprite/src/kernel/dbg/sun4.md */
#include "vm.h"
#include "sys.h"
#include "sync.h"
#include "fs.h"              /* /sprite/src/kernel/fs/fs.h */
#include "vmMach.h"          /* /sprite/src/lib/include/sun4.md/vmMach.h */
#include "devQueue.h"        /* /sprite/src/kernel/dev */
#include "devBlockDevice.h"  /* /sprite/src/kernel/dev */
#include "devDiskStats.h"    /* /sprite/src/kernel/dev */
#include "stdlib.h"
#include "iopb.h"             
#include "atcreg.h"          
#include "atc.h"
#include "devATC.h"


/*
#define DEBUG
*/

static atcDebug = TRUE;
static sentTables = 0;

#ifndef	NUM_IOPBS
#define	NUM_IOPBS	MAX_IOPBS	/* # of IOPBs allocated per ctrlr */
#endif

#ifndef	RBURSTSZ
#define	RBURSTSZ	8192		/* VME DMA read burst size */
#endif

#ifndef	WBURSTSZ
#define	WBURSTSZ	8192		/* VME DMA write burst size */
#endif

#ifndef	VMETIMEOUT
#define	VMETIMEOUT	100		/* VME bus timeout (in ms.) */
#endif

#ifndef	TD_FLAGS
#define	TD_FLAGS	0	       /* primary w/o block mode, by default */
#endif

#ifndef NATC
#define NATC   4         
#endif

/*
#ifndef ATC_MAX_DISKS
#define ATC_MAX_DISKS   1
#endif
*/
#ifndef ATC_MAX_DISKS
#define ATC_MAX_DISKS   (5*7)
#endif

#define TOTAL_CYL (900 * 5)
#define HEADS_PER_CYL 14
#define SECTORS_PER_TRACK 48
/*
 * Unit = (3 bits for controller, 6 bits for volume, 4 bits for partition)
 */

#define	MAXCTRL		NATC		/* max taken from the configuration */

#define MAXVOL		64		/* up to 64 logical volumes */

#define	CTRL(m)		(((m)>>10) & 0x7)/* controller number */
#define VOL(m)		(((m)>>4) & 0x3f)/* volume number */

#define	DIAG_VOL	63		/* reserved volume for diagnostics */

#define SECTOR_SIZE	512

/*
 * Define 'AMOD', the address modifier the Array should use for its DMA
 */

#define AMOD 0x3d
#define	A24AMOD	0x3d		/* vmea24 single word */
#define A32WAMOD 0x0d
#define A32BAMOD 0x0f            /* vmea32 block mode */
/*
 * Unless I decide to include a buf structure of my own, I need to 
 * change this and identify the special bit soemwhere else!
 * We need a bit in the buffer 'flags' field to mark special commands.
 * The following determines which bit we grab. Newer versions of RISC/os
 * define B_SPECIAL in <sys/buf.h> so we have to be careful here.
 */
#define	B_SPECIAL	0x80000000

#define	IS_SPECIAL(bp)	((bp)->b_flags & B_SPECIAL)



#define GET_ETYPE(e)	(((e)>>24) & 0xff)
#define GET_ESTATUS(e)	(((e)>>16) & 0xff)
#define GET_ECMD(e)	(((e)>>8) & 0xff)
#define GET_ECODE(e)	(((e)>>0) & 0xff)


/*  Error types */

#define ET_CMD_SPEC	0x00
#define ET_CMD_GEN	0x01
#define ET_ENVIRON	0x02
#define ET_INFO		0x03
#define ET_BKD_DIAG     0x04
#define ET_ABORT        0x05
#define ET_INIT         0x06

/*  Error status */

#define ES_EMPTY	0x00
#define ES_SENSE_VALID	0x01
#define ES_NONFATAL	0x02
#define ES_EXT_VALID	0x04
#define ES_PASS_THRU    0x08

/* Error Codes */
/* Generic codes */

#define EC_CMD            0x01     /* invalid command */
#define EC_OPTION         0x02     /* invalid option */
#define EC_PRIORITY       0x03     /* invalid priority */
#define EC_LINK           0x04     /* invalid linked_iopb */
#define EC_FLAGS          0x05     /* invlaid flags */
#define EC_ID             0x06     /* invlaid id field */
#define EC_LOG_BLK        0x07     /* invlaid logical block */
#define EC_BYTE_CNT       0x08     /* invlaid byte count */
#define EC_BUFFER         0x09     /* invalid buffer address */
#define EC_BUFADDRMOD     0x0a     /* invalid buffer address modifier */
#define EC_INT_LEVEL      0x0b     /* invalid interrupt level */
#define EC_AUXBUFMOD      0x0c     /* invalid aux buffer modifier */
#define EC_SG_ENTRIES     0x0d     /* invalid scatter-gather entries */
#define EC_AUXBUFSIZE     0x0e     /* invalid aux buffer size */
#define EC_AUXBUFFER      0x0f     /* invalid aux buffer */
#define EC_SG_COUNT       0x10     /* invalid count in sg_struct */
#define EC_SG_BUFFER      0x11     /* invalid buffer in sg_struct */
#define EC_SG_AMOD        0x12     /* invalid addr mod in s.g. entry */

#define EC_BFR_AND_SIZE   0x19     /* buffer and size conflict */
#define EC_BFR_AND_SG     0x1a     /* buffer and scatter/gather conflict */
#define EC_BFR_AND_CHKSUM 0x1b     /* buffer and checksum conflict */

/* stask.b Error Codes */

#define EC_SCSI           0x29     /* a SCSI error occurred */
#define EC_DEV_OFFLINE    0x2a     /* device is offline */
#define EC_DEV_TIMEOUT    0x2b     /* SCSI REQUEST TIMED OUT */
#define EC_DEV_REBUILDING 0x2c     /* device is being rebuilt





/*
 * The ATC_RAW_READ and ATC_RAW_WRITE commands return sector header
 * information that looks like the following struct.  Again, this
 * is byte-swapped in comparison with the documentation.
 *
 * A raw read is done during boot strap to determine the drive type.
 * The label is read at the same time to determine the disk geometry,
 * and this information is passed back into the controller.
 */
typedef struct ATCSectorHeader {
    /*
     * Byte 1
     */
    char sectorHigh	:2;
    char reserved	:3;
    char cylHigh	:3;
    /*
     * Byte 0
     */
    char cylLow		:8;
    /*
     * Byte 3
     */
    char driveType	:2;
    char sectorLow	:6;
    /*
     * Byte 2
     */
    char head		:8;
} ATCSectorHeader;

typedef struct ATCDisk ATCDisk;
typedef struct Request	Request;

struct perRequestInfo {
    Request *requestPtr;          /*  pointer to request info */
    Address dmaBuffer;            /*  pointer to dma buffer */
    int dmaBufferSize;            /*  size of dma buffer */
    struct iopbhdr *IOPBPtr;      /*  pointer to IOPB structure for request */
  } perRequestInfo;


typedef struct ATCController {
    volatile ATCRegs	*regsPtr;	/* Pointer to Controller's registers */
    int			number;		/* Controller number, 0, 1 ... */
    Sync_Semaphore	mutex;		/* Mutex for queue access */
    Sync_Condition	specialCmdWait; /* Condition to wait of for special
					 * commands liked test unit ready and
					 * reading labels. */
    int			numSpecialWaiting; /* Number of processes waiting to 
					    * get access to the controller for
					    * a sync command. */
    DevCtrlQueues	ctrlQueues;	/* Queues of disk attached to the 
					 * controller. */
    ATCDisk	        *disks[ATC_MAX_DISKS]; /* Disk attached to the
						     * controller. */
    u_long	flags;		/* controller state information */
#define			F_PRESENT	0x0001	/* device exists */
#define			F_INIT		0x0002	/* device got init. block */
#define			F_NEED_IOPB	0x0004	/* someone waiting for IOPB */
#define			F_IN_USE	0x0008	/* opened for normal I/O */
#define			F_DIAGS		0x0010	/* opened for diagnostics */
#define			F_FIFO_DATA	0x0020	/* FIFO data avail (diags) */

    u_long	debug;		/* controller debug flags */
#define			D_DEBUG		0x0001	/* enable most printfs */
#define			D_SINGLE_REQ	0x0002	/* single-request mode */
#define			D_TIMEOUTS	0x0004	/* enable IOPB timeouts */
#define			D_STIMEOUTS	0x0008	/* show timeouts */

    int         numActiveIOPBs;         /* number of outstanding IOPBs */
    int         maxActive;              /* most IOPBs ever active at a time */

    int	        ilev;			/* interrupt level */
    int	        ivec;			/* interrupt vector */

    int	        fifo_data;		/* last byte read (during diags) */
    int	        diag_buf[DBUFSZ];	/* buffer for diagnostics */

    u_long	vol_sizes[MAXVOL];	/* volume sizes */
    struct dstats   ds;	                /* driver statistics */

    struct iopbhdr  *free_iopbs;	/* head of linked IOPB list */

    /*
     * IOPBs and their corresponding status blocks are dynamically
     * allocated in the following arrays. Error status for a given
     * IOPB is returned in the status structure of the same index.
     */
    struct	iopbhdr		*iopblist;
    struct	status		*statuslist;

    /*
     * The IOPB and status tables contain pointers to the IOPBs and
     * status structures in the arrays above. The host DMAs the
     * following arrays during initialization so it knows where to
     * find everything in host memory.
     */
    struct	iopb_table	*iopb_table;
    struct	status_table	*status_table;
    
    /*
     * The table descriptor contains pointers to the IOPB and status
     * tables as well as some other controller information. We shove
     * this directly through the command FIFO during initialization.
     */
    struct	table_desc	td;	/* the table descriptor */
    
    Address	diag_buf_addr;		/* DMA address of the buffer */

    /*
     * This is a pointer to the iopblist array as it is mapped into
     * DMA space.
     */
    struct	iopbhdr	*iopblistp;
    struct	status	*statuslistp;

    struct      perRequestInfo   requestInfo[NUM_IOPBS];
    
} ATCController;



struct ATCDisk {
    ATCController	        *atcPtr;	/* Back pointer to controller state */
    int				atcDriveType;	/* ATC code for disk */
    int				slaveID;	/* Drive number */
    int				numCylinders;	/* ... on the disk */
    int				numHeads;	/* ... per cylinder */
    int				numSectors;	/* ... on each track */
    DevQueue			queue;
    DevDiskMap			map[DEV_NUM_DISK_PARTS];/* partitions */
    DevDiskStats		*diskStatsPtr;
};


/*
 * The interface to ATCDisk the outside world views the disk as a 
 * partitioned disk.  
 */
typedef struct PartitionDisk {
    DevBlockDeviceHandle handle; /* Must be FIRST field. */
    int	partition;		 /* Partition number on disk. */
    ATCDisk   *diskPtr;          /* Real disk. */
} PartitionDisk;

/*
 * Format of request queued for a ATC disk. This request is 
 * built in the ctrlData area of the DevBlockDeviceRequest.
 */

struct Request {
    List_Links	queueLinks;	/* For the dev queue modole. */
    char        command;	/* One of the ATC commands */
    char        option;         /* Option qualifies command */
    char        flags;                         
    ATCDisk     *diskPtr;	/* Target disk for request. */
    int         diskAddress;    /* Starting address of request. */
    int		numSectors;	/* Number of sectors to transfer. */
    Address	buffer;		/* Memory to transfer to/from. */
    int		retries;	/* Number of retries on the command. */
    DevBlockDeviceRequest *requestPtr; /* Block device generating this 
					* request. */
};


typedef struct _atcIOCParam{
  char    option;
  char    arg;
}atcIOCParam;


/* 
 * This is the state that needs to be maintained for each request.
 */

/*
 * State for each ATC controller.
 */
static ATCController *ATC[MAXCTRL];

/*
 * This controlls the time spent busy waiting for the transfer completion
 * when not in interrupt mode.
 */
#define ATC_WAIT_LENGTH	250000

/*
 * Constants related to timeouts
 */
#define WSECS   5
#define WSTART  0        /* start timer */
#define WCONT   1        /* continue timer */
#define REQUEST_COUNT 2  /* number of clock ticks until timeout */


/*
 * SECTORS_PER_BLOCK
 */
#define SECTORS_PER_BLOCK	(FS_BLOCK_SIZE / DEV_BYTES_PER_SECTOR)

/*
 * Forward declarations.
 */
extern void		printf();

static void		ResetController();
static ReturnStatus	TestDisk();
static ReturnStatus	ReadDiskLabel();
static void		SetupIOPB();
static void 		RequestDone();
static void 		StartNextRequest();
static void		FillInDiskTransfer();


/*
 *----------------------------------------------------------------------
 *
 * alloc_iopb --
 *
 *	Start the next request of an ATC controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
struct iopbhdr *
alloc_iopb(atcPtr)
    register ATCController *atcPtr;
{
    struct iopbhdr	*iopb;

if (atcDebug){
    
    if (atcDebug) {
      printf("Entered alloc_iopb.\n");
    }

}


    if (atcPtr->free_iopbs == NULL) {
	return NULL;
    }

    iopb = atcPtr->free_iopbs;			/* take from head */
    atcPtr->free_iopbs = atcPtr->free_iopbs->next_free;/* update head ptr */
    atcPtr->numActiveIOPBs++;
    if (atcPtr->numActiveIOPBs > atcPtr->maxActive) {
      if (atcDebug){
	printf("New max IOPBs:  %d\n",atcPtr->numActiveIOPBs);
      }
    }
    return iopb;
}


/*
 *----------------------------------------------------------------------
 *
 * free_iopb --
 *
 *	Start the next request of an ATC controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
free_iopb(atcPtr, iopb)
register ATCController	*atcPtr;
register struct iopbhdr	*iopb;
{

	if (atcPtr->free_iopbs == NULL) {	/* list was empty */
		iopb->next_free = NULL;
		atcPtr->free_iopbs = iopb;
		return;
	}

	iopb->next_free = atcPtr->free_iopbs;	/* put iopb at head */

	atcPtr->free_iopbs = iopb;
	
	if (atcPtr->flags & F_NEED_IOPB) {
		atcPtr->flags &= ~F_NEED_IOPB;
	}
	atcPtr->numActiveIOPBs--;
}


/*
 *----------------------------------------------------------------------
 *
 * put_fifo --
 *
 *	Start the next request of an ATC controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
put_fifo(atcPtr, data)
register ATCController *atcPtr;
register int	data;
{
  if (atcDebug){
    printf("Entered put_fifo.  Writing 0x%x\n", data & 0xffff);
    if (atcPtr->debug & D_DEBUG)
      printf("put_fifo: writing 0x%x\n", data & 0xffff);
  }

	atcPtr->regsPtr->write_fifo = data;
}


/*
 *----------------------------------------------------------------------
 *
 * get_fifo --
 *
 *	Start the next request of an ATC controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
get_fifo(atcPtr)
register ATCController	*atcPtr;
{
  int returnVal;

  if (atcDebug){
    printf("Entered get_fifo.\n");
  }

  returnVal = (atcPtr->regsPtr->read_fifo & 0x01ff);
  if (atcDebug){
    printf("Get_fifo returns %x\n",returnVal);
  }
  return(returnVal);
}


/*
 *----------------------------------------------------------------------
 *
 * send_table --
 *
 *	Start the next request of an ATC controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
send_table(atcPtr)
register ATCController *atcPtr;
{
  register int	i;
  register char	*p = (char *) &atcPtr->td;

  if (atcDebug){
    printf("Entered send_table; about to send tables to board\n");
  }

  MASTER_LOCK(&(atcPtr->mutex));

  put_fifo(atcPtr, TABLE_HEADER);
  for (i=0; i < sizeof(struct table_desc) ;i++, p++)
    put_fifo(atcPtr, *p);

  MASTER_UNLOCK(&(atcPtr->mutex));
  
}


/*
 *----------------------------------------------------------------------
 *
 * SendCommand --
 *
 *      Lower level routine to read or write an ATC device.  Use this
 *      to transfer a contiguous set of sectors.    The interface here
 *      is in terms of a particular ATC disk and the command to be
 *      sent.  This routine takes care of mapping its
 *      buffer into the special multibus memory area that is set up for
 *      Sun DMA.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Those of the command (Read, write etc.)
 *
 *----------------------------------------------------------------------
 */
static void
SendCommand(diskPtr, requestPtr, wait, index)
    ATCDisk      *diskPtr;	/* Unit, type, etc, of disk to send command. */
    Request	 *requestPtr;	/* Command request block. */
    Boolean 	 wait;		/* Wait for the command to complete. */
    int		index;
{
    ATCController *atcPtr	= diskPtr->atcPtr;
    int		diskAddr	= requestPtr->diskAddress;
    int		numSectors	= requestPtr->numSectors;
    Address	address		= requestPtr->buffer;

    /*
     * If the command is going to transfer data be relocate the address
     * into DMA space. 
     */

    if (atcDebug){
      printf("Entered Send_Command.\n");
    }

    SetupIOPB(requestPtr, diskPtr, diskAddr, numSectors, 
	       address, &(atcPtr->iopblist[index]));

    if (atcDebug){
      printf("Contents of iopb about to be sent to board:\n");
      printf("    command = 0x%x\n",atcPtr->iopblist[index].iopb.command);
      printf("    option = 0x%x\n",atcPtr->iopblist[index].iopb.option);
      printf("    priority = 0x%x\n",atcPtr->iopblist[index].iopb.priority);
      printf("    reserved1 = 0x%x\n",atcPtr->iopblist[index].iopb.reserved1);
      printf("    flags = 0x%x\n",atcPtr->iopblist[index].iopb.flags);
      printf("    log_array = 0x%x\n",atcPtr->iopblist[index].iopb.log_array);
      printf("    reserved2 = 0x%x\n",atcPtr->iopblist[index].iopb.reserved2);
      printf("    id = 0x%x\n",atcPtr->iopblist[index].iopb.id);
      printf("    logical_block = 0x%x\n",atcPtr->iopblist[index].iopb.logical_block);
      printf("    byte_count = 0x%x\n",atcPtr->iopblist[index].iopb.byte_count);
      printf("    buffer = 0x%x\n",atcPtr->iopblist[index].iopb.buffer);
      printf("    buf_addr_mod = 0x%x\n",atcPtr->iopblist[index].iopb.buf_addr_mod);
      printf("    intr_level = 0x%x\n",atcPtr->iopblist[index].iopb.intr_level);
      printf("    intr_vector = 0x%x\n",atcPtr->iopblist[index].iopb.intr_vector);
      
      printf("    auxbuf_mod = 0x%x\n",atcPtr->iopblist[index].iopb.auxbuf_mod);
      printf("    sg_entries = 0x%x\n",atcPtr->iopblist[index].iopb.sg_entries);
      printf("    auxbuf_size = 0x%x\n",atcPtr->iopblist[index].iopb.auxbuf_size);
      printf("    aux_buffer = 0x%x\n",atcPtr->iopblist[index].iopb.aux_buffer);
    }

    /*
     * Start up the controller by pushing index of iopb onto fifo.
     */
    put_fifo(atcPtr, index);

    if (atcDebug){
      printf("In Send_Command:  Wrote index to down_fifo.\n");
    }

}



/*
 *----------------------------------------------------------------------
 *
 * atcEntryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for a
 *	device.. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for a device. 
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *
 * Results:
 *	TRUE if the request is processed. FALSE if the request should be
 *	enqueued.
 *
 * Side effects:
 *	Request may be dequeue and submitted to the device. Request callback
 *	function may be called.
 *
 *----------------------------------------------------------------------
 */

static Boolean
atcEntryAvailProc(clientData, newRequestPtr) 
   ClientData	clientData;	/* Really the Device this request ready. */
   List_Links *newRequestPtr;	/* The new request. */
{
    register ATCDisk    *diskPtr = (ATCDisk *) clientData ;
    ATCController	*atcPtr = diskPtr->atcPtr;
    register Request	*requestPtr = (Request *) newRequestPtr;
    struct iopbhdr	*iopbhdrPtr;
    int			index;

    if (atcDebug){
      printf("Entered atcEntryAvailProc.\n");
    }

    if (requestPtr->numSectors == 0) {
	MASTER_UNLOCK(&atcPtr->mutex);
	RequestDone(requestPtr->diskPtr, requestPtr, SUCCESS, 0);
	MASTER_LOCK(&atcPtr->mutex);
	return TRUE;
    }
    
/*  actEntryAvailProc is called with master locks held
    MASTER_LOCK(&atcPtr->mutex);
*/
    iopbhdrPtr = alloc_iopb(atcPtr);
    index = iopbhdrPtr->index;
/*
    MASTER_UNLOCK(&atcPtr->mutex);
*/
    if (iopbhdrPtr == NULL) {
	return(FALSE);
    }

    MASTER_UNLOCK(&atcPtr->mutex);
    SendCommand(diskPtr, requestPtr, FALSE, index);
    MASTER_LOCK(&atcPtr->mutex);

    if (atcDebug){
      printf("In atcEntryAvailProc:  Returned from SendCommand.\n");
    }

    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * DevATCInit --
 *      Main thing to do here is appropriately malloc everything and
 *      map appropriate stuff into dma space.
 *	Initialize an ATC controller.
 *
 * Results:
 *	A NIL pointer if the controller does not exists. Otherwise a pointer
 *	the the ATCController stucture.
 *
 * Side effects:
 *	Map the controller into kernel virtual space.
 *	Allocate buffer space associated with the controller.
 *	Do a hardware reset of the controller.
 *
 *----------------------------------------------------------------------
 */
ClientData
DevATCInit(cntrlrPtr)
    DevConfigController *cntrlrPtr;	/* Config info for the controller */
{
    ATCController *atcPtr;	/* ATC specific state */
    register volatile ATCRegs *regsPtr;/* Control registers for ATC */
    short x;				/* Used when probing the controller */
    int	i;
    ReturnStatus status;

    /*
     * Poke at the controller's registers to see if it works
     * or we get a bus error.  
     * Mach_Probe(int byteCount, Address readAddress, Address writeAddress)
     * returns ReturnStatus.
     */
    regsPtr = (volatile ATCRegs *) cntrlrPtr->address;

    if (atcDebug){
      printf("In DevATCInit:  regsPtr = %x\n",regsPtr);
    }
    
    status = Mach_Probe(sizeof(regsPtr->read_fifo),
			 (char *) &(regsPtr->read_fifo), (char *) &x);
    if (status != SUCCESS) {
      printf("ATC%d NOT found.\n", cntrlrPtr->controllerID);
      return DEV_NO_CONTROLLER;
    }

#ifdef notdef
    /*
     * Don't write into fifo.
     */
    status = Mach_Probe(sizeof(regsPtr->write_fifo), "x",
				(char *) &(regsPtr->write_fifo));
    if (status != SUCCESS) {
	return DEV_NO_CONTROLLER;
    }
#endif

    /*
     * Allocate and initialize the controller state info.
     */


    atcPtr = (ATCController *)malloc(sizeof(ATCController));
    bzero((char *) atcPtr, sizeof(ATCController));
    ATC[cntrlrPtr->controllerID] = (ATCController *)atcPtr;

    atcPtr->regsPtr = regsPtr;
    atcPtr->number = cntrlrPtr->controllerID;
    atcPtr->ivec = cntrlrPtr->vectorNumber;            
    atcPtr->ilev = 3;            
   
    atcPtr->flags = 0;
    atcPtr->debug = D_TIMEOUTS|D_STIMEOUTS;

    for (i=0; i < MAXVOL ;i++)
      atcPtr->vol_sizes[i] = 0;

    atcPtr->vol_sizes[0] = 10000000;
    
    /*
     * Allocate the mapped DMA memory for the IOPBs and status table
     * entries.  This memory should not be freed unless the controller 
     * is not going to be accessed again.
     */

{
    static Address	linkMemAddress[4] =
/*
	{(Address) 0, (Address) 0, (Address) 0, (Address) 0};
*/
	{(Address) 0xff9e0000, (Address) 0xff9f0000, (Address) 0, (Address) 0};
    int			bufferSize;
    Address		hostAddr;
    Address		deviceAddr;
    Address		tempBuffer;
    int			addrOffset;

    bufferSize = NUM_IOPBS * (sizeof(struct iopbhdr) + sizeof(struct status) +
	    sizeof(struct iopb_table) + sizeof(struct status_table));
    if (linkMemAddress[cntrlrPtr->controllerID] == 0) {
	tempBuffer = malloc(bufferSize);
	hostAddr = (Address)VmMach_DMAAlloc(bufferSize, tempBuffer);
	deviceAddr = hostAddr - VMMACH_DMA_START_ADDR;
	if (hostAddr == (Address) NIL) {
	    panic("DevATCInit: unable to allocate DMA memory.\n");
	}
    } else {
	deviceAddr = linkMemAddress[cntrlrPtr->controllerID];
	hostAddr = (Address) VmMach_MapInBigDevice(deviceAddr, 0x00010000, 
		VMMACH_TYPE_VME32DATA);
    }
    printf("IOPBs (size:  %d bytes) mapped host=0x%x, device=0x%x\n",
	    bufferSize, hostAddr, deviceAddr);

    atcPtr->iopblist = (struct iopbhdr *) hostAddr;
    atcPtr->iopblistp = (struct iopbhdr *) deviceAddr;
    addrOffset = NUM_IOPBS * sizeof(struct iopbhdr);

    atcPtr->statuslist = (struct status *) (hostAddr + addrOffset);
    atcPtr->statuslistp = (struct status *) (deviceAddr + addrOffset);
    addrOffset += NUM_IOPBS * sizeof(struct status);

    atcPtr->iopb_table = (struct iopb_table *) (hostAddr + addrOffset);
    atcPtr->td.iopb_list = (struct iopb_table *) (deviceAddr + addrOffset);
    addrOffset += NUM_IOPBS * sizeof(struct iopb_table);

    atcPtr->status_table = (struct status_table *) (hostAddr + addrOffset);
    atcPtr->td.status_list = (struct status_table *) (deviceAddr + addrOffset);

    atcPtr->td.vme_timeout = VMETIMEOUT;
    atcPtr->td.iopb_list_mod = AMOD;
    atcPtr->td.num_iopbs = NUM_IOPBS;
    atcPtr->td.status_list_mod = AMOD;
    atcPtr->td.flags = TD_FLAGS;
    atcPtr->td.reserved = 0;
    atcPtr->td.rdma_burst_sz = RBURSTSZ;
    atcPtr->td.wdma_burst_sz = WBURSTSZ;

    for (i=0; i < NUM_IOPBS; i++) {
      atcPtr->iopb_table[i].iopb_ptr = &atcPtr->iopblistp[i].iopb;
      atcPtr->iopb_table[i].iopb_addr_mod = AMOD;
      atcPtr->status_table[i].status_ptr = &atcPtr->statuslistp[i];
      atcPtr->status_table[i].status_addr_mod = AMOD;
    }

    atcPtr->numActiveIOPBs = 0;
    atcPtr->maxActive = 0;
    atcPtr->free_iopbs = NULL;
    for (i=0; i < NUM_IOPBS ;i++) {
      atcPtr->iopblist[i].index = i;
      free_iopb(atcPtr, &atcPtr->iopblist[i]);
    }
}
  
    if (atcDebug){
      printf("table_desc.iopb_list       = 0x%x\n", atcPtr->td.iopb_list);
      printf("table_desc.status_list     = 0x%x\n", atcPtr->td.status_list);
      printf("table_desc.iopb_list_mod   = 0x%x\n", AMOD);
      printf("table_desc.status_list_mod = 0x%x\n", AMOD);
      printf("table_desc.num_iopbs       = %d\n", NUM_IOPBS);
    }

    atcPtr->flags |= F_PRESENT;

    Sync_SemInitDynamic(&atcPtr->mutex,"Dev:ATC mutex");
    atcPtr->numSpecialWaiting = 0;
    atcPtr->ctrlQueues =Dev_CtrlQueuesCreate(&atcPtr->mutex, atcEntryAvailProc);

    for (i = 0 ; i < ATC_MAX_DISKS ; i++) {
	 atcPtr->disks[i] =  (ATCDisk *) NIL;
    }

    return( (ClientData) atcPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseProc --
 *	Device release proc for controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
ReleaseProc(handlePtr)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
{

    PartitionDisk	*pdiskPtr = (PartitionDisk *) handlePtr;

    if (atcDebug){
      printf("Entered ReleaseProc.\n");
    }

    free((char *) pdiskPtr);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * IOControlProc --
 *
 *      Do a special operation on a raw SMD Disk.
 *      Need to put atc iocontrols in here.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static ReturnStatus
IOControlProc(handlePtr, ioctlPtr, replyPtr)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    PartitionDisk	*pdiskPtr	= (PartitionDisk *) handlePtr;
    register ATCDisk    *diskPtr	= pdiskPtr->diskPtr;
    ATCController       *atcPtr         = diskPtr->atcPtr;

/*
    atcIOCParam         *atcIOCParamPtr = (atcIOCParam *)ioctlPtr->inBuffer;
*/
    DevBlockDeviceRequest *reqPtr       = (DevBlockDeviceRequest *)ioctlPtr->inBuffer;
    int  amtTransfer;

    if (atcDebug){
      printf("Entered IOControlProc.\n");
    }


     switch (ioctlPtr->command) {

     case IOC_ATC_DEBUG_ON:
       atcDebug = TRUE;
       return(SUCCESS);
       break;

     case IOC_ATC_DEBUG_OFF:
       atcDebug = FALSE;
       return(SUCCESS);
       break;

     case CMD_READ_CONF:
       return(SUCCESS);
       break;

     case CMD_WRITE_CONF:
       return(SUCCESS);
       break;

     case IOC_REPOSITION:
       /*
	* Reposition is ok
	*/
       return(SUCCESS);
       break;
       /*
	* No disk specific bits are set this way.
	*/
       
/*
     case IOC_DEV_ATC_READXBUS:
       break;

     case IOC_DEV_ATC_WRITEXBUS:
       break;

     case IOC_DEV_ATC_SSTATS:
       return(SUCCESS);
       break;
       
     case IOC_DEV_ATC_CSTATS:
       return(SUCCESS);
       break;
*/

     case IOC_DEV_ATC_IO:
       if (atcDebug){
	 printf("operation = %d\n",reqPtr->operation);
	 printf("startAddress = 0x%x\n",reqPtr->startAddress);
	 printf("startAddrHigh = 0x%x\n",reqPtr->startAddrHigh);
	 printf("bufferLen = %d\n",reqPtr->bufferLen);
	 printf("buffer = 0x%x\n",reqPtr->buffer);
       }
       Dev_BlockDeviceIOSync(handlePtr, reqPtr, &amtTransfer);
       if (amtTransfer != reqPtr->bufferLen){
	 return(FAILURE);
       } else {
	 return(SUCCESS);
       }
       break;

     case ATC_RESET:
       ResetController(atcPtr);
       return(SUCCESS);
       break;

     default:
       return(GEN_INVALID_ARG);
     }
}

/*
 *----------------------------------------------------------------------
 *
 * BlockIOProc --
 *	Start a block IO operations on an ATC board.
 *      Remember that we are treating the queue as a single queue for
 *      the entire board.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
BlockIOProc(handlePtr, requestPtr) 
    DevBlockDeviceHandle	*handlePtr;
    DevBlockDeviceRequest	*requestPtr;
{
    PartitionDisk	*pdiskPtr	= (PartitionDisk *) handlePtr;
    register ATCDisk    *diskPtr	= pdiskPtr->diskPtr;
    register Request	*reqPtr		= (Request *) requestPtr->ctrlData;

    if (atcDebug){
      printf("Entered BlockIOProc.\n");
    }


    if (requestPtr->operation == FS_READ) {
	reqPtr->command = CMD_READ;
	reqPtr->option = 0;
	reqPtr->flags = 0;
    } else {
	reqPtr->command = CMD_WRITE;
	reqPtr->option = 0;
	reqPtr->flags = 0;
    }

    reqPtr->diskPtr = diskPtr;
    FillInDiskTransfer(pdiskPtr, requestPtr->startAddress, 
			(unsigned) requestPtr->bufferLen,
			&(reqPtr->diskAddress), &(reqPtr->numSectors));
    reqPtr->buffer = requestPtr->buffer;
    reqPtr->retries = 0;
    reqPtr->requestPtr = requestPtr;
    Dev_QueueInsert(diskPtr->queue, (List_Links *) reqPtr);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * atcIdleCheck --
 *
 *	Routine for the Disk Stats module to use to determine the idleness
 *	for a disk.   I have changed this to look for the F_INUSE or F_DIAG
 *      flags for the controller.  These tell whether the controller is in
 *      use for normal I/O or for diagnostics.
 *
 * Results:
 *	TRUE if the disk pointed to by clientData is idle, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
atcIdleCheck(clientData, diskStatsPtr) 
    ClientData	clientData;
    DevDiskStats	*diskStatsPtr;	/* Unused for ATC. */
{
    ATCDisk *diskPtr = (ATCDisk *) clientData;

    return (!(diskPtr->atcPtr->flags & F_PRESENT));
}


/*
 *----------------------------------------------------------------------
 *
 * DevATCDiskAttach --
 *
 *      Initialize a device hanging off an ATC controller. 
 *
 * Results:
 *	A DevBlockDeviceHanlde for this disk.
 *
 * Side effects:
 *	Disks:  The label sector is read and the partitioning of
 *	the disk is set up.  The partitions correspond to device
 *	files of the same type but with different unit number.
 *
 *----------------------------------------------------------------------
 */
DevBlockDeviceHandle *
DevATCDiskAttach(devicePtr)
    Fs_Device	    *devicePtr;	/* Device to attach. */
{
    ReturnStatus error;
    ATCController       *atcPtr;	/* ATC specific controller state */
    register ATCDisk    *diskPtr;
    PartitionDisk	*pdiskPtr;	/* Partitioned disk pointer. */
    int		controllerID;
    int		diskNumber;

    if (atcDebug){
      printf("Entered DevATCDiskAttach.\n");
    }

    controllerID = CTRL(devicePtr->unit);   
    diskNumber = VOL(devicePtr->unit);
    
    if (atcDebug){
      printf("AtcDiskAttach unit 0x%x ctrlr %d disk %d\n",
	     devicePtr->unit, controllerID, diskNumber);
    }

    atcPtr = (ATCController *)ATC[controllerID];


    if (atcPtr == (ATCController *)NIL ||
	atcPtr == (ATCController *)0 ||
	diskNumber > MAXVOL) {
	return ((DevBlockDeviceHandle *) NIL);
    }

    if (!sentTables){
      ResetController(atcPtr);
      sentTables = 1;
      MACH_DELAY(100000);
    }

    /*
     * Set up a slot in the disk list. We do a malloc before we grab the
     * MASTER_LOCK().
     */
    diskPtr = (ATCDisk *) malloc(sizeof(ATCDisk));
    bzero((char *) diskPtr, sizeof(ATCDisk));
    diskPtr->atcPtr = atcPtr;
    diskPtr->atcDriveType = 0;
    diskPtr->slaveID = diskNumber;
    diskPtr->queue = Dev_QueueCreate(atcPtr->ctrlQueues, 1, 
				DEV_QUEUE_FIFO_INSERT, (ClientData) diskPtr);

    MASTER_LOCK(&(atcPtr->mutex));
    if (atcPtr->disks[diskNumber] == (ATCDisk *) NIL) {
	/*
	 * See if the disk is really there.
	 */
	atcPtr->disks[diskNumber] = diskPtr;

	error = TestDisk(atcPtr, diskPtr);
	if (error == SUCCESS) {
	    /*
	     * Look at the zero'th sector for disk information.  This also
	     * sets the drive type with the controller.
	     */
	    error = ReadDiskLabel(atcPtr, diskPtr);
	}

	if (error != SUCCESS) {
	    atcPtr->disks[diskNumber] =  (ATCDisk *) NIL;
	    MASTER_UNLOCK(&(atcPtr->mutex));
	    Dev_QueueDestroy(diskPtr->queue);
	    free((Address)diskPtr);
	    return ((DevBlockDeviceHandle *) NIL);
	} 
	MASTER_UNLOCK(&(atcPtr->mutex));
	/* 
	 * Register the disk with the disk stats module. 
	 */
	{
	    Fs_Device	rawDevice;
	    char	name[128];
	    extern int	sprintf();

	    rawDevice =  *devicePtr;
	    rawDevice.unit = rawDevice.unit & ~0xf;
	    (void) sprintf(name, "atc%d-%d", atcPtr->number, diskPtr->slaveID);
	    diskPtr->diskStatsPtr = DevRegisterDisk(&rawDevice, name, 
				   atcIdleCheck, (ClientData) diskPtr);
	}
    } else {
	/*
	 * The disk already exists. Use it.
	 */
	MASTER_UNLOCK(&(atcPtr->mutex));
	Dev_QueueDestroy(diskPtr->queue);
	free((Address)diskPtr);
	diskPtr = atcPtr->disks[diskNumber];
    }
    pdiskPtr = (PartitionDisk *) malloc(sizeof(*pdiskPtr));
    bzero((char *) pdiskPtr, sizeof(*pdiskPtr));
    pdiskPtr->handle.blockIOProc = BlockIOProc;
    pdiskPtr->handle.IOControlProc = IOControlProc;
    pdiskPtr->handle.releaseProc = ReleaseProc;
    pdiskPtr->handle.minTransferUnit = DEV_BYTES_PER_SECTOR;
    pdiskPtr->handle.maxTransferSize = 100*1024;
    pdiskPtr->partition = DISK_IS_PARTITIONED(devicePtr) ? 
					DISK_PARTITION(devicePtr) : 
					WHOLE_DISK_PARTITION;
    pdiskPtr->diskPtr = diskPtr;
    return ((DevBlockDeviceHandle *) pdiskPtr);
}



/*
 *----------------------------------------------------------------------
 *
 * ResetController --
 *
 *	Reset the controller.  This is done by sending down the descriptor
 *	table to the board through the FIFO.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the controller.
 *
 *----------------------------------------------------------------------
 */
static void
ResetController(atcPtr)
    volatile ATCController *atcPtr;
{

  if (atcDebug){
    printf("Entered ResetController.\n");
  }


  send_table(atcPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TestDisk --
 *	Get a controller's status to see if it exists.  We do not
 *      test individual disks in this controller.
 *
 * Results:
 *	SUCCESS if the device is ok, DEV_OFFLINE otherwise.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
TestDisk(atcPtr, diskPtr)
    ATCController *atcPtr;
    ATCDisk *diskPtr;
{

  if (atcDebug){
    printf("Entered TestDisk.\n");
  }


  if (atcPtr->flags & F_PRESENT){
    return(SUCCESS);
  } else {
    return(DEV_OFFLINE);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * ReadDiskLabel --
 *
 *	Read the label of the disk and record the partitioning info.
 *
 *      For the ATC driver, this is a dummy routine which currently
 *      specifies a single partition containing all the data available.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Define the disk partitions that determine which part of the
 *	disk each different disk device uses.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
ReadDiskLabel(atcPtr, diskPtr)
    ATCController *atcPtr;
    ATCDisk *diskPtr;
{
  int part;

  if (atcDebug){
    printf("Entered ReadDiskLabel.\n");
  }

  diskPtr->atcDriveType = 1;
  diskPtr->numCylinders = TOTAL_CYL;
  diskPtr->numHeads = HEADS_PER_CYL;
  diskPtr->numSectors = SECTORS_PER_TRACK;
    
  for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
    diskPtr->map[part].firstCylinder = 0;
    diskPtr->map[part].numCylinders = TOTAL_CYL;
  }
  return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 *  FillInDiskTransfer
 *	Fill in the disk address and number of sectors of a command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The number of sectors to transfer gets trimmed down if it would
 *	cross into the next partition.
 *
 *----------------------------------------------------------------------
 */
static void
FillInDiskTransfer(pdiskPtr, startAddress, length, diskAddrPtr, numSectorsPtr)
    PartitionDisk	*pdiskPtr;	/* Target Disk Partition. */
    unsigned int	startAddress;	/* Starting offset in bytes.*/
    unsigned int	length;		/* Length in bytes. */
    int 		*diskAddrPtr;	/* Disk disk address of
					 * the first sector to transfer */
    int *numSectorsPtr;			/* The number
					 * of sectors to transferred. */
{
    ATCDisk *diskPtr;	/* State of the disk */
    int totalSectors;	/* The total number of sectors to transfer */
    int lastSector;	/* Last sector of the partition */
    int startSector;	/* The first sector of the transfer */
    int	part;		/* Partition number. */
    int	cylinderSize;	/* Size of a cylinder in sectors. */

    if (atcDebug){
      printf("Entered FillInDiskTransfer.\n");
    }

    diskPtr = pdiskPtr->diskPtr;
    part = pdiskPtr->partition;
    cylinderSize = diskPtr->numHeads * diskPtr->numSectors;
    /*
     * Do bounds checking to keep the I/O within the partition.
     * sectorZero is the sector number of the first sector in the partition,
     * lastSector is the sector number of the last sector in the partition.
     * (These sector numbers are relative to the start of the partition.)
     */
    lastSector = diskPtr->map[part].numCylinders * cylinderSize - 1;
    totalSectors = length/DEV_BYTES_PER_SECTOR;

    startSector = startAddress / DEV_BYTES_PER_SECTOR;

    if (startSector > lastSector) {
	/*
	 * The offset is past the end of the partition.
	 */
	*numSectorsPtr = 0;
	printf("Warning: ATCDiskIO: Past end of partition %d\n",
				part);
	return;
    } else if ((startSector + totalSectors - 1) > lastSector) {
	/*
	 * The transfer is at the end of the partition.  Reduce the
	 * sector count so there is no overrun.
	 */
	totalSectors = lastSector - startSector + 1;
	printf("Warning: ATCDiskIO: Overrun partition %d\n",
				part);
    }
    (*diskAddrPtr) = startSector;
    (*diskAddrPtr) += diskPtr->map[part].firstCylinder * cylinderSize;

    *numSectorsPtr = totalSectors;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * SetupIOPB --
 *
 *      What to do about the bp structure, and all the info passed to/from it?
 *
 *      Setup a IOPB for a command.  The IOPB can then
 *      be passed to SendCommand.  It specifies everything the
 *	controller needs to know to do a transfer, and it is updated
 *	with status information upon completion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the control block.
 *
 *----------------------------------------------------------------------
 */
static void
SetupIOPB(requestPtr, diskPtr, diskAddr, numSectors, address, IOPBPtr)
     Request *requestPtr;
     ATCDisk *diskPtr;		        /* Spefifies unit, drive type, etc */
     register int diskAddr;	        /* Disk address of operation*/
     int numSectors;			/* Number of sectors to transfer */
     register Address address;		/* Address of the buffer */
     struct iopbhdr *IOPBPtr;       /* I/O Parameter Block  */
{
    ATCController	*atcPtr		= diskPtr->atcPtr;
    int			logicalBlock	= diskAddr;
    int			byteCount	= numSectors * DEV_BYTES_PER_SECTOR;
    int			index		= IOPBPtr->index;
    char          	cmd             = requestPtr->command;
    int			bufSize		= DEV_BYTES_PER_SECTOR  * numSectors;
    Address		dmaAddress;
    int			amod;

    if (atcDebug){
      printf("Entered SetupIOPB.\n");
    }
    
    /*
     * Determine DMA address and address modifier.
     */
    if (VmMachIsXbusMem(address)) {
	amod = A32BAMOD;
	dmaAddress = address;
    } else {
	amod = A32WAMOD;
	if (bufSize > 0) {
	    dmaAddress = (Address) VmMach_32BitDMAAlloc(bufSize, address);
	} else {
	    dmaAddress = VMMACH_DMA_START_ADDR;
	}
	if ((unsigned) dmaAddress > (unsigned)VMMACH_DMA_START_ADDR) {
	    dmaAddress -= VMMACH_DMA_START_ADDR;
	}
    }
    if (atcDebug){
        printf("dmaBuffer size %d sectors mapped to dma address 0x%x\n",
		numSectors, dmaAddress);
    }

    bzero((char *) &(IOPBPtr->iopb), sizeof(struct iopb));
    IOPBPtr->bp			= NULL;		    /* we are not using this */
    IOPBPtr->iopb.command	= requestPtr->command;
    IOPBPtr->iopb.option	= requestPtr->option;
    IOPBPtr->iopb.reserved1	= 0x7f;
    IOPBPtr->iopb.flags		= requestPtr->flags;
    IOPBPtr->iopb.log_array	= 1;                /* logical array number */
    IOPBPtr->iopb.reserved2	= 0;
    IOPBPtr->iopb.buffer	= dmaAddress;
    IOPBPtr->iopb.buf_addr_mod	= amod;
    IOPBPtr->iopb.intr_level	= atcPtr->ilev;
    IOPBPtr->iopb.intr_vector	= atcPtr->ivec;
    IOPBPtr->iopb.auxbuf_mod	= 0;
    IOPBPtr->iopb.sg_entries	= 0;		    /* no scatter-gather */
    IOPBPtr->iopb.auxbuf_size	= 0;
    IOPBPtr->iopb.aux_buffer	= 0;
    
    /*  
     *  byte_count used to be byteCount
     */

    switch (cmd) {
    case CMD_READ:
      IOPBPtr->iopb.byte_count = byteCount;
      IOPBPtr->iopb.id = diskPtr->slaveID;
      IOPBPtr->iopb.logical_block = logicalBlock;
      break;	
    case CMD_WRITE:
      IOPBPtr->iopb.byte_count = byteCount;
      IOPBPtr->iopb.id = diskPtr->slaveID;
      IOPBPtr->iopb.logical_block = logicalBlock;
      break;
    default:
      printf("setup_iopb: invalid command %d\n", cmd);
      free_iopb(atcPtr, IOPBPtr);
    }

    atcPtr->requestInfo[index].requestPtr = requestPtr;
    atcPtr->requestInfo[index].dmaBuffer = dmaAddress;
    atcPtr->requestInfo[index].dmaBufferSize = byteCount;
    atcPtr->requestInfo[index].IOPBPtr = IOPBPtr;
}



/*
 *----------------------------------------------------------------------
 *
 * reportError --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */


static void
reportError(statusPtr)
struct status	*statusPtr;
{
  int etype	= GET_ETYPE(statusPtr->status);
  int estatus	= GET_ESTATUS(statusPtr->status);
  int ecmd	= GET_ECMD(statusPtr->status);
  int ecode	= GET_ECODE(statusPtr->status);

  if (atcDebug){
    printf("Entered reportError.\n");
  }

  printf("ETYPE = %d, ESTATUS = %d, ECMD = %d, ECODE = %d\n",
	 etype, estatus, ecmd, ecode);


  switch (etype){
  case ET_CMD_SPEC:
    printf("Error type ET_CMD_SPEC (command specific) \n");
    break;
  case ET_CMD_GEN:
    printf("Error type ET_CMD_GEN (generic to all host commands)\n");
    break;
  case ET_ENVIRON:
    printf("Error type ET_ENVIRON (environmental problems)\n");
    break;
  case ET_INFO:
    printf("Error type ET_INFO (informational messages for host)\n");
    break;
  case ET_BKD_DIAG:
    printf("Error type ET_BKD_DIAG (reported from bckgrnd diagnostics)\n");
    break;
  case ET_ABORT:
    printf("Error type ET_ABORT (error contains abort code #)\n");
    break;
  case ET_INIT:
    printf("Error type ET_INIT (for reporting initialization problems)\n");
    break;
  default:
    printf("Problem!!! Unrecognized error type!\n");
  }


  switch (estatus) {
  case ES_EMPTY:
    printf("Error status ES_EMPTY (no status bits active)\n");
    break;
  case ES_SENSE_VALID:
    printf("Error status ES_SENSE_VALID (sense data is valid)\n");
    break;
  case ES_NONFATAL:
    printf("Error status ES_NONFATAL (host command completed successfully)\n");
    break;
  case ES_EXT_VALID:
    printf("Error status ES_EXT_VALID (external error info is valid)\n");
    break;
  case ES_PASS_THRU:
    printf("Error status ES_PASS_THRU (to log pass-thru ops to error log)\n");
    break;
  default:
    printf("Problem!!! Unrecognized error status!\n");
  }

  switch (ecode) {
  case EC_CMD:
    printf("Error code EC_CMD -- invalid command in iopb\n");
    break;
  case EC_OPTION:
    printf("Error code EC_OPTION -- invalid option in iopb\n");
    break;
  case EC_PRIORITY:
    printf("Error code EC_PRIORITY -- invalid priority in iopb\n");
    break;
  case EC_LINK:
    printf("Error code EC_LINK -- invalid linked_iopb in iopb\n");
    break;
  case EC_FLAGS:
    printf("Error code EC_FLAGS -- invalid flags in iopb\n");
    break;
  case EC_ID:
    printf("Error code EC_ID -- invalid id field in iopb\n");
    break;
  case EC_LOG_BLK:
    printf("Error code EC_LOG_BLK -- invalid logical block in iopb\n");
    break;
  case EC_BYTE_CNT:
    printf("Error code EC_BYTE_CNT -- invalid byte count in iopb\n");
    break;
  case EC_BUFFER:
    printf("Error code EC_BUFFER -- invalid buffer address in iopb\n");
    break;
  case EC_BUFADDRMOD:
    printf("Error code EC_BUFADDRMOD -- invalid buf address mod in iopb\n");
    break;
  case EC_INT_LEVEL:
    printf("Error code EC_INT_LEVEL -- invalid interrupt level in iopb\n");
    break;
  case EC_AUXBUFMOD:
    printf("Error code EC_AUXBUFMOD -- invalid aux buf mod in iopb\n");
    break;
  case EC_SG_ENTRIES:
    printf("Error code EC_SG_ENTRIES -- invalid # s/g entries in iopb\n");
    break;
  case EC_AUXBUFSIZE:
    printf("Error code EC_AUXBUFSIZE -- invalid aux buffer size in iopb\n");
    break;
  case EC_AUXBUFFER:
    printf("Error code EC_AUXBUFFER -- invalid aux buff address in iopb\n");
    break;
  case EC_SG_COUNT:
    printf("Error code EC_SG_COUNT -- invalid s/g count in iopb\n");
    break;
  case EC_SG_BUFFER:
    printf("Error code EC_SG_BUFFER -- invalid s/g buffer in iopb\n");
    break;
  case EC_SG_AMOD:
    printf("Error code EC_SG_AMOD -- invalid  s/g buffer amod in iopb\n");
    break;
  default:
    printf("Problem!!! Unrecognized error code!\n");
  }
}




/*
 *----------------------------------------------------------------------
 *
 * DevATCIntr --
 *
 *	Handle interrupts from the ATC controller.   This routine
 *	may start any queued requests.
 *
 * Results:
 *	TRUE if an ATC controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevATCIntr(clientData)
    ClientData	clientData;
{
    ATCController	*atcPtr	= (ATCController *) clientData;
    Request		*requestPtr;
    struct iopbhdr	*iopbPtr;
    struct status	*statusPtr;
    int			index, returnVal;
    ReturnStatus	status;
    int                 error;
    
    if (atcDebug){
      printf("Entered DevATCIntr.\n");
    }

    /*  read fifo to get iopb index */

    returnVal = get_fifo(atcPtr);
    index = (returnVal & 0x7f);
    error = (returnVal & 0x100);
    if (atcDebug){
      printf("Value from get_fifo is 0x%x; index after interrupt is 0x%x\n",
	     returnVal, index);
    }

    if (error == 0){
      if (atcDebug){
	printf("No error occurred on iopb\n");
      }
    } else {
      printf("Error in iopb!!\n");
    }

    iopbPtr	= &atcPtr->iopblist[index];
    statusPtr	= &atcPtr->statuslist[index];
    requestPtr	= atcPtr->requestInfo[index].requestPtr;

    /*
     * Release the DMA buffer if command mapped one. 
     */
    if (!VmMachIsXbusMem(atcPtr->requestInfo[index].dmaBuffer) &&
    	    atcPtr->requestInfo[index].dmaBufferSize > 0) {
        if (((unsigned)atcPtr->requestInfo[index].dmaBuffer) & 0x80000000) {
	    VmMach_32BitDMAFree(atcPtr->requestInfo[index].dmaBufferSize, 
		    atcPtr->requestInfo[index].dmaBuffer);
        } else {
	    VmMach_DMAFree(atcPtr->requestInfo[index].dmaBufferSize, 
		    atcPtr->requestInfo[index].dmaBuffer);
        }
	atcPtr->requestInfo[index].dmaBufferSize = 0;
    }
    bzero((char *) &(atcPtr->requestInfo[index]), sizeof(perRequestInfo));
	
    /*  
     * Need to signal error if detected 
     */
    {
	int etype	= GET_ETYPE(statusPtr->status);
	int estatus	= GET_ESTATUS(statusPtr->status);
	/*
	int ecmd	= GET_ECMD(statusPtr->status);
	int ecode	= GET_ECODE(statusPtr->status);
	*/


	if (error == 0){
	  status = SUCCESS;
	} else{
	  switch (etype) {
	  case ET_CMD_GEN:
	  case ET_CMD_SPEC:
	    reportError(statusPtr);
	    if (estatus & ES_NONFATAL) {
	      status = SUCCESS;
	    } else {
	      status = FAILURE;
	    }
	    break;
	  default:
	    reportError(statusPtr);
	    status = FAILURE;
	    break;
	  }
	}
      }
  
      MASTER_LOCK(&(atcPtr->mutex));
      free_iopb(atcPtr, iopbPtr);
      MASTER_UNLOCK(&(atcPtr->mutex));
      
      RequestDone(requestPtr->diskPtr,requestPtr,status,requestPtr->numSectors);
      StartNextRequest(atcPtr);
      return(TRUE);

  }


/*
 *----------------------------------------------------------------------
 *
 * RequestDone --
 *
 *	Mark a request as done by calling the request's doneProc. DMA memory
 *	is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
RequestDone(diskPtr, requestPtr, status, numSectors)
    ATCDisk		*diskPtr;
    Request		*requestPtr;
    ReturnStatus	status;
    int			numSectors;
{

  if (atcDebug){
    printf("Entered RequestDone.\n");
  }

    if (numSectors > 0) {
	if (requestPtr->requestPtr->operation == FS_READ) {
	    diskPtr->diskStatsPtr->diskStats.diskReads++;
	} else {
	    diskPtr->diskStatsPtr->diskStats.diskWrites++;
	}
    }
    (requestPtr->requestPtr->doneProc)(requestPtr->requestPtr, status,
				numSectors*DEV_BYTES_PER_SECTOR);
}

/*
 *----------------------------------------------------------------------
 *
 * StartNextRequest --
 *
 *	Start the next request of an ATC controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
StartNextRequest(atcPtr)
    ATCController *atcPtr;
{
    ATCDisk		*diskPtr;
    Request		*requestPtr;
    int			index;

    if (atcDebug){
      printf("Entered StartNextRequest.\n");
    }

     /*
      * If the controller is busy, just return.
      */
    MASTER_LOCK(&atcPtr->mutex);
    while (atcPtr->free_iopbs != NULL) {
	requestPtr = (Request *) Dev_QueueGetNextFromSet(atcPtr->ctrlQueues,
		DEV_QUEUE_ANY_QUEUE_MASK, (ClientData *) &diskPtr);
	if (requestPtr == (Request *) NIL) {
	    break;
	}
	index = alloc_iopb(atcPtr)->index;

	MASTER_UNLOCK(&atcPtr->mutex);
	SendCommand(diskPtr, requestPtr, FALSE, index);
	MASTER_LOCK(&atcPtr->mutex);
    }
    MASTER_UNLOCK(&atcPtr->mutex);
}
