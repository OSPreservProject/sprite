/* 
 * devJaguarHBA.c --
 *
 *	Driver for the Interphase V/SCSI 4210 Jaguar SCSI host bus adapter 
 *	for SUN's running Sprite.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "mach.h"
#include "jaguar.h"
#include "jaguarDefs.h"
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "sync.h"
#include "stdlib.h"
#include "vmMach.h"

/*
 * WARNING -- WARNING --- WARNING --- WARNING
 * The code and data structures in this file are carefully coded to 
 * run on machines that don't do autobus sizing and unaligned memory fetches.
 *
 * Please keep in mind the following:
 *
 * 1) Although structures like struct JaguarMem appear to be C data structures
 *    they are used to describe the interface to the Jaguar controller board.
 *    Because these data structures need to be understood by both the
 *    host CPU and the Jaguar's processor changes should only occur when
 *    the Jaguar's firmware changes the interface. 
 * 2) The Jaguar is accessed in the short IO spaces on the VME bus. This
 *    means that the data path is only 16 bits wide. This means that
 *    unless your processor does autobus sizing (ie convert 32 bit accesses
 *    into 2 16 bit access if the bus width is 16 bit) only shorts should
 *    be stored into JaguarMem. Note: SPARC (the sun4) doesn't do autobus
 *    sizing.
 * 3) Beware of padding introduced by the compiler. For example, if the
 *    Jaguar documentation says:
 *	  2 bytes length
 *	  4 bytes buffer address
 *    and you declare a C structure to be 
 *       struct {
 *	    short length;
 *	    char  *bufAddr;
 *	 }
 *    Things will break (on sun4) because the compiler will added 2 bytes of
 *    padding to insure that the (char *) starts on the (4 byte) word boundry.
 *    The routine CheckSizes in this file tries to catch errors of this 
 *    form.
 */

/*
 * The Jaguar occupies 2 Kbytes on the VME short I/O space (DEV_VME_D16A16).
 * The image of these 2 Kbytes is described by the structure 
 * struct JaguarMem. Some of the members of this structure like 
 * the mcsb, mce, cmdQueue,and css must occur at fixed offsets from the start
 * of the 2K region. The other items in JaguarMem are placed at fixed offsets
 * to simply the driver and may be moved around.
 */

/*
 * Constants that define the layout of JaguarMem.
 *
 * SIZE_JAUGAR_MEM - The size of the dual ported memory accessible to the
 *		     host.
 * NUM_CQE 	  -  Number of entries in the command queue. This limits
 * 	             the number of unacknowledged commands that we can submit
 *	             to the Jaguar. For a lack of a better number, 16.
 *
 * NUM_SG_ELEMENTS - Number of scatter/gather elements allocated in the 
 *		     juguar memory.  Scatter/gather elements don't needed
 *		     to be allocated in Jaguar memory but it is convenient
 *		     to do so.  Otherwise the would have to be allocated
 *		     or mapped into DVMA.
 *
 * MEM_PAD           Number of bytes of free space not allocated in the
 * 	             structure JaguarMem. Note that MEM_PAD is a function 
 *	             of NUM_SG_ELEMENTS and NUM_CQE.
 * NUM_WORK_QUEUES   Number of work queues to use. 
 * 
 */
#define	SIZE_JAUGAR_MEM	(2*1024)
#define	NUM_CQE 8
#define	NUM_SG_ELEMENTS	64
#define	NUM_WORK_QUEUES	14

#define	MEM_PAD (SIZE_JAUGAR_MEM - sizeof(JaguarMCSB) -  \
		(NUM_CQE+1)*(sizeof(JaguarIOPB)+sizeof(JaguarCQE)) - \
		sizeof(JaguarCRB) - sizeof(JaguarCCSB) - \
		NUM_SG_ELEMENTS * sizeof(JaguarSG))

typedef struct JaguarMem {
    JaguarMCSB	mcsb;	/* Master Control and Status Block. 16 Bytes  */
    JaguarCQE	mce;	/* Master Command Entry Queue. 12 Bytes */
    JaguarCQE	cmdQueue[NUM_CQE]; /* Command Queue. 12*NUM_CQE Bytes */
    JaguarIOPB	masterIOPB;	/* IOPB used for the MCE. 64 Bytes */
    JaguarIOPB	iopbs[NUM_CQE]; /* IOPB for cmdQueue. 64*NUM_CQE Bytes*/
    JaguarSG	sgElements[NUM_SG_ELEMENTS];
					/* Scatter/gather elements for host. */
    char		padding[MEM_PAD]; /* Padding to force css field into
					   * correct offset. */
    JaguarCRB	crb;	/* Command response block. 76 Bytes */
    JaguarCCSB	css;	/* Contoller specific Space. 120 Bytes */
} JaguarMem;

/*
 * Many of the Jaguar's data structures need addresses as offsets into the
 * JaguarMem. To deal with this offsets:
 *
 * POINTER_TO_OFFSET()	- Convert a host pointer to an offset.
 * OFFSET_TO_POINTER()	- Convert an offset to a host pointer.
 *
 */
#define	POINTER_TO_OFFSET(ptr, memPtr)	((unsigned)(ptr) - (unsigned)(memPtr))
#define OFFSET_TO_POINTER(offset, memPtr) ((char *)(memPtr)+(unsigned)(offset))

/* 
 * workq (s) - 
 * From the documentation it appears that for the Jaguar to  efficiently 
 * talk to a device a workq must be configured for the device.  The 
 * documentation doesn't mention a cost associated with having many 
 * unused workq so it is tempting to create all 14 work queues at 
 * initialization time.  The reason for not doing this is that certain
 * options such as parity and workq priority is determined during
 * work queue initialization. 
 * 
 */
/* Forward declaration. */
typedef struct Controller Controller;

/*
 * Device - The data structure containing information about a device. One of
 * these structure is kept for each attached device. Note that is structure
 * is casted into a ScsiDevice and returned to higher level software.
 * This implies that the ScsiDevice must be the first field in this
 * structure.
 */

typedef struct Device {
    ScsiDevice handle;	/* Scsi Device handle. This is the only part
			 * of this structure visible to higher 
			 * level software. MUST BE FIRST FIELD IN STRUCTURE. */
    int	targetID;	/* TargetID of device. */
    int	unitAddress;	/* Jaguar address of this device.  */
    int	workQueue;	/* Jaguar workqueue allocated for this device. */
    int	numActiveCmds;	/* Number of commands enqueued on the HBA for this
			 * command. */
    DevQueue	queue;	/* Queue for the device. */
    Controller  *ctrlPtr; /* Controller to which device is attached. */
    Address	dmaBuffer; /* DMA buffer for device. */
		   /*
		    * The following part of this structure is 
		    * used to handle SCSI commands that return 
		    * CHECK status. To handle the REQUEST SENSE
		    * command we must: 1) Save the state of the current
		    * command into the "struct FrozenCommand". 2) Submit
		    * a request sense command  */
    struct FrozenCommand {		       
	ScsiCmd	*scsiCmdPtr;	   /* The frozen command. */
	unsigned char statusByte; /* It's SCSI status byte, Will always have
				   * the check bit set. */
	int amountTransferred;    /* Number of bytes transferred by this 
				   * command. */
    } frozen;	
} Device;

typedef struct CmdAction {
    int	action;		/* Action to be performed when command completes. 
			 * See below for list of actions. */
    ClientData	actionArg;	/* Argument for action. */
} CmdAction;

/*
 * Possibly action values.
 */
#define	FILL_IN_CRB_ACTION	0x1
#define	SCSI_CMD_ACTION		0x2
#define	IS_WAIT_ACTION(action)	((action)== FILL_IN_CRB_ACTION)

/*
 * Controller - The Data structure describing a Jaguar controller. One
 * of these structures exists for each active Jaguar HBA on the system. Each
 * controller may have from zero to 14 devices attached to it. 
 */
struct Controller {
    volatile JaguarMem *memPtr;	/* Pointer to the registers
                                    of this controller. */
    volatile JaguarCQE *nextCQE; /* Next available CQE. */
    Boolean workQueue0Busy; /* Work Queue 0 is being used. */
    char    *name;	/* String for error message for this controller.  */
    DevCtrlQueues devQueues;    /* Device queues for devices attached to this
				 * controller.	 */
    Sync_Semaphore mutex; /* Lock protecting controller's data structures. */
    Sync_Condition ctrlCmdWait; /* Wait condition for syncronous command
				 * to finish. */
    Sync_Condition ctrlQueue0Wait; /* Wait condition for exclustive access
				    * to workqueue 0. */
    int		intrLevel;	/* VME interrupt level for controller. */
    int		intrVector;	/* VME interrupt vector for controller. */
    CmdAction	cmdAction[NUM_CQE]; /* Action to be performed when command 
				     * completes. */
    Device  *devices[NUM_WORK_QUEUES];   /* Pointers to the device attached. The
					 * index is the workQueue number - 1. */
};

#define MAX_JAGUAR_CTRLS	16
static Controller	*Controllers[MAX_JAGUAR_CTRLS];

static int	devJaguarDebug = 0;

/*
 * Constants for the sun implementation.
 * DMA_BURST_COUNT - The number of VME DMA transfers performed in a 
 *		     single burst before releasing the bus.  A value of
 *		     0 uses the maximum of 128 32-bit transfers.
 * JAGUAR_ADDRESS_MODIFIER - The Address space modifier and transfer type
 *			     for Jaguar DMA. We choose 32 bit normal mode
 *			     transfers with a A24 bit supervisor data address
 *			     modifier (0x3d).
 * MAX_CMDS_QUEUED - Maximum number of command to queue per device.
 * SELECTION_TIMEOUT - Timeout value for selecting a device in 1 millisecond
 *		       ticks. We choose 1 second timeout.
 * RESELECTION_TIMEOUT - Timeout value for a device re-selecting in 32 
 *			 milliseconds ticks. 0 means infinite.  
 * VME_TIMEOUT - Timeout value for VME access. 0 means 100 milliseconds.
 * DEV_MAX_DMA_SIZE - Maximum size of a DMA request to this device.
 */

#define	DMA_BURST_COUNT		0
#define	JAGUAR_ADDRESS_MODIFIER  (JAGUAR_32BIT_MEM_TYPE | \
				  JAGUAR_NORMAL_MODE_XFER | 0x3D)
#define	MAX_CMDS_QUEUED		1
#define	SELECTION_TIMEOUT	1000
#define	RESELECTION_TIMEOUT	0
#define	VME_TIMEOUT		0
#define	DEV_MAX_DMA_SIZE	(32*1024)
#define	VME_INTERRUPT_PRIORITY	2


/*
 * Macros for reading and writing 32 bit values from the short IO space.
 */

#define	READ_LONG(var)	(((var)[0]<<16)|((var)[1]))
#define	SET_LONG(var,value) \
		(((var)[0] = ((value)>>16)),((var)[1]=(0xff&(value))))

static void LockWorkq0();
static void UnLockWorkq0();
static void CopyFromJaguarMem();
static void CopyToJaguarMem();
static void ZeroJaguarMem();
static Boolean WaitForBitSet();
static void FillInScsiIOPB();
static void RequestDone();
static void StartNextRequest();
static char *ErrorString();
static Boolean SendJaguarCmd();


/*
 *----------------------------------------------------------------------
 *
 * WaitForResponseBlock --
 *
 *	Wait for a Jaguar command to complete. 
 *
 * Results:
 *	TRUE always. Should probably sent up a timeout handler.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

static void
WaitForResponseBlock(ctrlPtr,crbPtr)
    Controller	*ctrlPtr;         /* Controller to which command 
				   * was submitted. */
    volatile JaguarCRB *crbPtr;   /* Command Response Block to be filled in by
				   * interrupt handler. 
				   */
{
    while (!(crbPtr->status & JAGUAR_CRB_BLOCK_VALID)) {
	Sync_MasterWait(&(ctrlPtr->ctrlCmdWait), &ctrlPtr->mutex,FALSE);
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * InitializeWorkq --
 *
 *      Send Jaguar the command to initialize the specified work queue
 *	with the given parameters and options. 
 *
 * 	NOTE: This routine assumes that the error recovery method used 
 *	      in the driver would be to Freeze the Work queue upon error.
 *
 * Results:
 *	TRUE is queue was initialized, FALSE otherwise.
 *
 * Side effects:
 *	A work queue initialize command is send to the controller.
 *
 *----------------------------------------------------------------------
 */

static Boolean
InitializeWorkq(ctrlPtr,workqNum, parity, priority)
    Controller	*ctrlPtr;	/* Controller to initalize Work Queue on. */
    int		workqNum;	/* Workq to initialize must be 1-14. */
    Boolean	parity;		/* TRUE - parity check should be enabled. When
				 * communicated thru this workq. FALSE if not. 
				 */
    int		priority;	/* The priority level of this workq. (1-14) */
{
    JaguarIOPB		inMemIOPB;
    volatile register JaguarIOPB	*iopb = &inMemIOPB;
    JaguarCRB		crb;

    /*
     * Build the appropriate Initialized Work Queue Command IOPB
     * and send it to workQ 0. We must use workq 0 for this comand.
     */
    bzero((char *) &crb, sizeof(crb));
    bzero((char *) iopb, sizeof(*iopb));
    iopb->command = JAGUAR_INIT_WORK_QUEUE_CMD;
    iopb->options = JAGUAR_IOPB_INTR_ENA;
    iopb->intrLevel = ctrlPtr->intrLevel;
    iopb->intrVector = ctrlPtr->intrVector;
    iopb->cmd.workQueueArg.number = workqNum;
    iopb->cmd.workQueueArg.options =
		     JAGUAR_WQ_INIT_QUEUE | 
		     JAGUAR_WQ_FREEZE_QUEUE | 
		     (parity ? JAGUAR_WQ_PARITY_ENABLE : 0);

    iopb->cmd.workQueueArg.slots = NUM_CQE;
    iopb->cmd.workQueueArg.priority = priority;
    /*
     * Send the command into work queue zero. SendJaguarCmd will do the
     * waiting for us and return 
     */
    if (!SendJaguarCmd(ctrlPtr, 0, iopb, FILL_IN_CRB_ACTION, 
			(ClientData)&crb)) {
	panic("%s: Initialize WorkQ %d did not finished.\n",ctrlPtr->name,
	       workqNum);
	return (FALSE);
    }
    if (crb.status & (JAGUAR_CRB_ERROR|JAGUAR_CRB_EXCEPTION)) {
	printf("%s: Initialize WorkQ %d failed with error 0x%s %s\n",
	     ctrlPtr->name, workqNum, crb.iopb.returnStatus, 
	     ErrorString(crb.iopb.returnStatus));
	return (FALSE);

    }
    return (TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * InitializeJaguar --
 *
 *	Initialize the Jaguar HBA and ready it for commands.
 *
 *	This routine assumes that the controller is MASTER_LOCKed()
 *
 * Results:
 *	TRUE if the controller initializes. FALSE if controller is broken.
 *
 * Side effects:
 *	Controller is reset and initialized. This causes the SCSI bus(es) to
 *	be reset and any in progress commands aborted. 
 *
 *----------------------------------------------------------------------
 */

static Boolean
InitializeJaguar(memPtr, name, intrLevel, intrVector)
    volatile JaguarMem *memPtr;	/* Pointer to VME Short IO space of HBA. */
    char	*name;		/* Name of the controller for error messges. */
    int		intrLevel;	/* VME Interrupt level for HBA. */
    int		intrVector;	/* VME Interrupt vector for HBA. */
{
     register volatile JaguarMCSB *mcsb = &(memPtr->mcsb);
     register volatile JaguarCQE  *cqe;
     register volatile JaguarIOPB *iopb;
     register volatile JaguarCRB  *crb;

    /*
     * Start off with a clean slate by reseting the board. Documentation
     * says must keep reset bit set at least 50 microseconds.
     * We give it 1 millesecond of reset in case 
     */

    mcsb->control = JAGUAR_MCR_RESET;
    MACH_DELAY(1000); 
    mcsb->control = 0;
    /*
     * Wait for the Jaugar to signal Board OK. Board OK not valid for
     * 100 microseconds after reset. We give it 1 millesecond to be happy.
     */
    MACH_DELAY(1000);
    if (!(mcsb->status & JAGUAR_MSR_BOARD_OK)) {
	printf("Warning: %s board not OK\n", name);
	return (FALSE);
    }


    /*
     * Initialize the rest of the MCSB. Currently we wont deal with 
     * queue available interrupts so we turn them off.
     */
    mcsb->queueAvail = 0;
    mcsb->queueHead = 0;
    mcsb->reserved[0] = mcsb->reserved[1] = mcsb->reserved[2] = 0;
    /*
     * Initialize the Command Queue (CQ). To do this we clear out the 
     * CQE's and point them at their IOPBs. Clear out the IOPBs too.
     * The commandTag the the CQE's is given an unique number equal
     * to it's index plus one.  (Tag 0 is the MCE. )
     */
     {
	 int	i;
	 cqe  = memPtr->cmdQueue;
	 iopb = memPtr->iopbs;
	 for (i = 0; i < NUM_CQE; i++, cqe++, iopb++) {
	     cqe->controlReg = 0;
	     cqe->iopbOffset = POINTER_TO_OFFSET(iopb,memPtr);
	     cqe->iopbLength = sizeof(JaguarIOPB)/4;
	     cqe->commandTag[0] = (i+1);
	     cqe->commandTag[1] = 0;
	     cqe->workQueue = 0;
	     cqe->reserved = 0;
	 }
	 ZeroJaguarMem((short *) iopb, sizeof(JaguarIOPB) * NUM_CQE);
   }
    /*
     * Clear the area to be used as the Command Response Block.
     */
    crb = &(memPtr->crb);
    ZeroJaguarMem((short *) crb, sizeof(JaguarCRB));

    /*
     * Initialize the Master Control Enter (MCE) so we can send commands
     * to the board. 
     */
    cqe  = &(memPtr->mce);
    iopb = &(memPtr->masterIOPB);
    cqe->controlReg = 0;
    cqe->iopbOffset = POINTER_TO_OFFSET(iopb,memPtr);
    cqe->iopbLength = sizeof(JaguarIOPB)/4;
    cqe->workQueue = 0;
    cqe->reserved = 0;
    cqe->commandTag[0] = 0;
    cqe->commandTag[1] = 0;
    ZeroJaguarMem((short *)iopb , sizeof(JaguarIOPB));
    /*
     * The first command we send is the Initialize controller command.
     * In order to send this command we must build a Controller
     * Initialization Block (CIP) in JaguarMem.  Since this block is 
     * not needed after initialization, we overlay the scatter sgElements
     * gather vectors with the CIP.
     */
    {
	volatile JaguarCIB *cib = (volatile JaguarCIB *) (memPtr->sgElements);
	ZeroJaguarMem((short *) cib, sizeof(JaguarCIB));

	cib->numQueueSlots = NUM_CQE;
	cib->dmaBurstCount = DMA_BURST_COUNT;
	cib->normalIntrVector =	JAGUAR_INTR_VECTOR(intrLevel,intrVector);
	cib->errorIntrVector = JAGUAR_INTR_VECTOR(intrLevel,intrVector);
	cib->priTargetID = JAGUAR_DEFAULT_BUS_ID;
	cib->secTargetID = JAGUAR_DEFAULT_BUS_ID;
	cib->offsetCRB = POINTER_TO_OFFSET(&(memPtr->crb),memPtr);
	SET_LONG(cib->scsiSelTimeout, SELECTION_TIMEOUT);
	SET_LONG(cib->scsiReselTimeout, RESELECTION_TIMEOUT);
	SET_LONG(cib->vmeTimeout, VME_TIMEOUT);

	ZeroJaguarMem((short *)iopb, sizeof(iopb));
	iopb->command = JAGUAR_INIT_HBA_CMD;
	iopb->addrModifier = JAGUAR_BOARD_MEM_TYPE;
	SET_LONG(iopb->bufferAddr,POINTER_TO_OFFSET(cib,memPtr));
    }
    {
	register unsigned int status;
	/*
	 * Send the command off and wait for response.
	 */
	cqe->controlReg = JAGUAR_CQE_GO_BUSY;
	if (!WaitForBitSet(&(crb->status),JAGUAR_CRB_BLOCK_VALID,10000)) {
	     panic("%s init controller timeout.\n", name);
	     return FALSE;
	}
	/*
	 * Check for happy completion status.
	 */
	status = crb->status;
	if (!(status & JAGUAR_CRB_COMMAND_COMPLETE)) {
	     panic("%s init ctrl cmd didn't complete, status 0x%x\n",
		    name, status);
	     return FALSE;
	}
	if (status & (JAGUAR_CRB_ERROR|JAGUAR_CRB_EXCEPTION)) {
	     panic("%s init ctrl cmd error 0x%x, status 0x%x\n",
		    name, iopb->returnStatus, status);
	     return FALSE;
	}
	/*
	 * If we got here the controller init cmd successfully completed.
	 * Acknowledge the command to release the CRB.
	 */
	crb->status = 0;
	/*
	 * Start Queue Mode operation and wait for ack.
	 */
	mcsb->control |= JAGUAR_MCR_START_QUEUES;
	if (!WaitForBitSet(&(crb->status),JAGUAR_CRB_BLOCK_VALID,10000)) {
	     panic("%s start queue mode timeout.\n",name);
	     return FALSE;
	}
	status = crb->status;
	if (!(status & JAGUAR_CRB_QUEUE_START)) {
	     panic("%s start queue mode bad status 0x%x.\n",name,status);
	     return FALSE;
	}
	crb->status = 0;
    }
    /*
     * Be neat! Clear out the rest of the Jaguar memory.
     */
    ZeroJaguarMem((short *) (memPtr->sgElements), 
		     NUM_SG_ELEMENTS * sizeof(JaguarSG));
    ZeroJaguarMem((short *) (memPtr->padding), MEM_PAD);
    /*
     * Notifiy the world that we are alive by printing our Controller
     * Configuration Status Block. This will be useful if someone ask
     * the question: "What rev PROMs are you running?"
     */
    { 
	JaguarCCSB css;
	/*
	 * Note we copy the CCSB from the board because passing pointers
	 * into Jaguar  Memory to printf would be a no-no because we 
	 * don't have control of the type memory references done by
	 * printf.
	 */
	CopyFromJaguarMem((short *)&(memPtr->css),(short *)&css, sizeof(css));
	printf("%s product %3s/%c firmware %3s %2s/%2s/%4s ",
		name, css.code, css.variation, css.firmwareLevel,
		css.firmwareDate, css.firmwareDate+2, css.firmwareDate+4);
	printf("%dK RAM bus0 ID %d bus1 ID %d\n", css.bufferRAMsize,
		css.primaryID, css.secondaryID);
    }
    return TRUE;
}

static struct errorString {
    int	code;
    char *string ;
} errorStrings[] = JAGUAR_ERROR_CODES;
#define NUM_ERROR_STRINGS (sizeof(errorStrings) / sizeof(errorStrings[0]))


/*
 *----------------------------------------------------------------------
 *
 * ErrorString --
 *
 *	Return a string describing a Jaguar error code.
 *
 * Results:
 *	An error string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
ErrorString(returnStatus)
    unsigned short	returnStatus;
{
    int	code = returnStatus & 0xff;
    int	i;

    for (i = 0; i < NUM_ERROR_STRINGS; i++) {
	if (errorStrings[i].code == code) {
	    return errorStrings[i].string;
	}
    }
    return "Unknown error";
}

/*
 *----------------------------------------------------------------------
 *
 * MapJaguarToSpriteErrorCode --
 *
 *	Map a jaguar return error code to a Sprite ReturnStatus value.
 *
 * Results:
 *	A sprite return status value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus 
MapJaguarToSpriteErrorCode(status)
    unsigned short	status;
{
    int	code = status & 0xff;

    if (code == 0) {
	return SUCCESS;
    }
    if (code <= 0x20 && code < 0x30) {
	return DEV_DMA_FAULT;
    }
    return DEV_HARD_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 *  SendScsiCommand --
 *
 *	Enqueue a SCSI command into the Jaguar command queue.
 *
 * Results:
 *	
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static Boolean
SendScsiCommand( devPtr, scsiCmdPtr)
    Device *devPtr; /* Jaguar device for command. */
    ScsiCmd	*scsiCmdPtr; /* SCSI command to send. */

{
    volatile JaguarIOPB iopbMem;	
    Boolean	retVal;

    /*
     * Check to see if we can use normal PASS_THRU command or must be
     * PASS_THRU_EXT extented.
     */
    if (scsiCmdPtr->commandBlockLen <= sizeof(iopbMem.cmd.scsiArg.cmd)) {
	/*
	 * We can use a simple PASS_THRU command. 
	 */
	FillInScsiIOPB(devPtr, scsiCmdPtr, &iopbMem);
	retVal = SendJaguarCmd(devPtr->ctrlPtr, devPtr->workQueue, &iopbMem, 
				SCSI_CMD_ACTION, (ClientData) scsiCmdPtr);
    } else {
	/*
	 * Can't handle PASS_THRU_EXT command yet.
	 */
	panic("%s: SCSI command too large, %d bytes.\n",devPtr->ctrlPtr->name, 
					scsiCmdPtr->commandBlockLen);
	return (FALSE);
    }
    return (retVal);

}

/*
 *----------------------------------------------------------------------
 *
 * DevJaguarIntr --
 *
 *	Process a Jaguar interrupt by processing the entry in the ctrl's
 *	Command Response Block and reseting the controller.
 *
 * Results:
 *	TRUE if an interrupt was process. FALSE if not interrupt was
 *	present on this controller.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
DevJaguarIntr(clientData)
    ClientData	clientData;	/* Controller to process. */
{
    register volatile JaguarCRB *crb;
    unsigned int	        status;
    unsigned int 	        returnStatus;
    CmdAction		        *actionPtr;
    register Controller         *ctrlPtr;

    ctrlPtr = (Controller *) clientData;
    crb = &(ctrlPtr->memPtr->crb);
    /*
     * Check the controller's CRB. If BLOCK_VALID is we have some sort
     * of condition present.
     */
    status = crb->status;
    if (!(status & JAGUAR_CRB_BLOCK_VALID)) {
	return (FALSE);
    }
    /*
     * Classify the condition. If should be one of the following:
     * 1) A command just completed - COMMAND_COMPLETE.
     * 2) Queue mode was started - QUEUE_START
     * 3) A command queue entry became available. - QUEUE_AVAILABLE
     * 
     * QUEUE_START interrupts are uninteresting to us. No processing
     * is necessary.  QUEUE_AVAILABLE are also uninteresting and 
     * unless I don't understand something impossible because we
     * never set anything in the IQAR on the MCSB. Because these
     * interrupt are boring we processing them first.
     */
    if (status & (JAGUAR_CRB_QUEUE_START|JAGUAR_CRB_QUEUE_AVAILABLE)){
	/*
	 * Clear the interrupt.
	 */
	crb->status = 0;
	return (TRUE);
    }
    if (!(status & JAGUAR_CRB_COMMAND_COMPLETE)) {
	printf("Warning: %s unknown interrupt, status = 0x%x\n", 
		ctrlPtr->name, status);
	crb->status = 0;
	return (TRUE);
    }
    /*
     * Since all work queues should be setup with FREEZE on error and not
     * ABORT on error, take aborted commands very seriously.
     */
    if (status & JAGUAR_CRB_ABORTED) {
	panic("%s command aborted, status = 0x%x!!!\n", 
			ctrlPtr->name,status);
	crb->status = 0;
	return (TRUE);
    }
    /*
     * We have a real COMMAND_COMPLETION. Read the tag out of the
     * return CQE to find the action to be performed. Also, read the
     * return status from the IOPB returned.
     */
    actionPtr = &(ctrlPtr->cmdAction[crb->commandTag[0]]);
    returnStatus = crb->iopb.returnStatus;
    /*
     * Check to see an error.
     */
    if (actionPtr->action & FILL_IN_CRB_ACTION) {
	CopyFromJaguarMem((short *)crb,(short *) actionPtr->actionArg, 
				sizeof(*crb));
    } 
    if (actionPtr->action & SCSI_CMD_ACTION) {
	ScsiCmd	*scsiCmdPtr = (ScsiCmd *) actionPtr->actionArg;
	Device	*devPtr = (Device *) (ctrlPtr->devices[crb->workQueue-1]);
	ReturnStatus	spriteStatus = SUCCESS;
	int	transferCount = 0;
	if (returnStatus & 0xff) {
	    /*
	     * Error code 0x34 means that the transfer count didn't match
	     * the number of bytes transfers. We don't consider this an 
	     * error.
	     */
	    if ((returnStatus & 0xff) != 0x34)  {
		spriteStatus = MapJaguarToSpriteErrorCode(returnStatus);
	    }
	    if (spriteStatus != SUCCESS) {
		printf("Warning: Device %s HBA error 0x%x: %s\n",
			devPtr->handle.locationName, returnStatus,
			ErrorString(returnStatus));
	    }
	    transferCount = (spriteStatus == SUCCESS) ? 
				READ_LONG(crb->iopb.maxXferLen) : 0;
	}
	devPtr->numActiveCmds--;
	RequestDone(devPtr, scsiCmdPtr, spriteStatus,
			(returnStatus >> 8) & 0xff, transferCount);
	StartNextRequest(devPtr);

    }
    if (IS_WAIT_ACTION(actionPtr->action)) {
	Sync_MasterBroadcast(&(ctrlPtr->ctrlCmdWait));
    }
    crb->status = 0;
    return (TRUE);

}


/*
 *----------------------------------------------------------------------
 *
 *  SendJaguarCmd --
 *
 *	Enter a Jaguar command into the specified Jaguar work Queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
SendJaguarCmd(ctrlPtr, workQueue, iopbPtr, action, actionArg)
    Controller	*ctrlPtr;	/* Controller to enter command. */
    int		workQueue;	/* Command destination workq number. */
    volatile JaguarIOPB *iopbPtr;	/* Jaguar IOPB command block. */
    int		action;		/* Action to be performed on completion. */
    ClientData  actionArg;	/* Argument to action. */
{
    volatile JaguarMem	*memPtr = ctrlPtr->memPtr;
    volatile JaguarCQE	*cqe;
    volatile JaguarIOPB	*iopb;

    MASTER_LOCK(&ctrlPtr->mutex);
    /*
     * Work queue is special because it has only one entry. To keep from
     * overrunning it we must observe a locking protocol.
     */ 
    if (workQueue == 0) {
	LockWorkq0(ctrlPtr);	
    }
    /*
     * Get the next Command Queue entry stepping the next available queue
     * entry pointer around the circular queue.
     */
    cqe = ctrlPtr->nextCQE++;
    if ((memPtr->cmdQueue - ctrlPtr->nextCQE) >= NUM_CQE) {
	ctrlPtr->nextCQE = memPtr->cmdQueue;
    }
    if (cqe->controlReg & JAGUAR_CQE_GO_BUSY) {
	panic("%s: Command Queue Full\n", ctrlPtr->name);
    }
    /*
     * Find to IOPB we hardwired this CQE to point at.   
     */
    iopb = memPtr->iopbs + (cqe - memPtr->cmdQueue);
    /*
     * Fill in the on-board iopb with the one our caller passed us.
     */
    CopyToJaguarMem((short *) iopbPtr, (short *)iopb, sizeof(*iopb));
    cqe->workQueue = workQueue;
     /*
     * Inform interrupt handler of the action we want on completion.
     */
    {
	CmdAction *actionPtr = &(ctrlPtr->cmdAction[cqe->commandTag[0]]);
	actionPtr->action = action;
	actionPtr->actionArg = actionArg;
    }
    /*
     * If the caller specified a CRB we wait for the response.
     */
    if(IS_WAIT_ACTION(action)) { 
	WaitForResponseBlock(ctrlPtr,(volatile JaguarCRB *) actionArg);
    }
    if (workQueue == 0) {
	UnLockWorkq0(ctrlPtr);	
    }
    MASTER_UNLOCK(&ctrlPtr->mutex);
    return (TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * LockWorkq0 --
 *
 *	Grap exclusive access to work queue 0 of the specified controller.
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
LockWorkq0(ctrlPtr)
    Controller	*ctrlPtr;
{
    while (ctrlPtr->workQueue0Busy) {
	Sync_MasterWait(&(ctrlPtr->ctrlQueue0Wait), &ctrlPtr->mutex,FALSE);
    }
    ctrlPtr->workQueue0Busy = TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * UnLockWorkq0 --
 *
 *	Release exclusive access to work queue 0 of the specified controller.
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
UnLockWorkq0(ctrlPtr)
    Controller	*ctrlPtr;
{
    ctrlPtr->workQueue0Busy = FALSE;
    Sync_MasterBroadcast(&(ctrlPtr->ctrlQueue0Wait));
}


/*
 *----------------------------------------------------------------------
 *
 * CopyFromJaguarMem --
 *
 *	Copy a block from Jaguar Memory to host memory.
 * 	NOTE:
 *		This routine assumes that both the block and the host
 *		memory region are aligned on a 2 byte boundry and are
 *		an even number of bytes long.
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
CopyFromJaguarMem(blockPtr, hostPtr, blockSize)
    register short	*blockPtr;	/* Block in Jaguar Memory to copy. */
    register short	*hostPtr;	/* Address in host memory. */
    register int	blockSize;	/* Size of block in bytes. */

{
    /*
     * We do the copy 2 bytes at a time because the Jaguar is in 16 bit
     * data IO space of the VME bus.
     */
    while (blockSize > 0) {
	*(hostPtr++) = *(blockPtr++);
	blockSize -= sizeof(short);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CopyToJaguarMem --
 *
 *	Copy a block to Jaguar Memory from host memory.
 * 	NOTE:
 *		This routine assumes that both the block and the host
 *		memory region are aligned on a 2 byte boundry and are
 *		an even number of bytes long.
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
CopyToJaguarMem(blockPtr, hostPtr, blockSize)
    register short	*blockPtr;	/* Block in Jaguar Memory to copy. */
    register short	*hostPtr;	/* Address in host memory. */
    register int	blockSize;	/* Size of block in bytes. */

{
    /*
     * We do the copy 2 bytes at a time because the Jaguar is in 16 bit
     * data IO space of the VME bus.
     */
    while (blockSize > 0) {
	*(hostPtr++) = *(blockPtr++);
	blockSize -= sizeof(short);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ZeroJaguarMem --
 *
 *	Zero a block of Jaguar board memory. 
 *
 * NOTE: This routine assumes the block is aligned on a 2 byte boundry and
 *	 is an even number of bytes long.
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
ZeroJaguarMem(blockPtr, blockSize)
    register short	*blockPtr;	/* Pointer to block to zero. */
    register int	blockSize;	/* Number of bytes in the block. */
{
    /*
     * We do the zeroing 2 bytes at a time because the Jaguar is in 16 bit
     * data IO space of the VME bus.
     */
    while (blockSize > 0) {
	*(blockPtr++) = 0;
	blockSize -= sizeof(short);
    }
}


/*
 *----------------------------------------------------------------------
 *
 *  WaitForBitSet --
 *
 *	Wait for a bit to become set in a Jaguar word.
 *
 *
 * Results:
 *	TRUE if the bit appears. FALSE if we timeout.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
WaitForBitSet(wordPtr, bit, maxCount)
    register volatile unsigned short *wordPtr;	/* Word to check. */
    register unsigned short bit;	/* Bit to check for. */
    int	     maxCount;			/* Number of 100 microseconds to check 
					 * before giving up.
					 */
{
    /*
     * Timeout after waiting one second, poll every 100 microseconds.
     */
    for(; maxCount > 0; maxCount--) {
	if (*wordPtr & bit) {
	    return (TRUE);
	}
	MACH_DELAY(100);
    }
    return ((*wordPtr & bit) != 0);
}


/*
 *----------------------------------------------------------------------
 *
 * CheckSizes --
 *
 *	Check the sizes of the Jaguar structure declarations to insure they
 *	are consistent with the controller's ideas.  This routine is 
 *	intended to catch padding introduced by the compiler that may 
 *	break the driver's code. This checking should really be done at
 *	compile time but run time checking is better nothing. 
 *
 * Results:
 *	TRUE if the sizes checked are ok, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Boolean
CheckSizes()
{
    return (
        (SIZE_JAUGAR_MEM == sizeof(JaguarMem)) &&
	(JAGUAR_MCSB_SIZE == sizeof(JaguarMCSB)) &&
	(JAGUAR_CQE_SIZE == sizeof(JaguarCQE)) &&
	(JAGUAR_CCSB_SIZE == sizeof(JaguarCCSB)) &&
	(JAGUAR_CIB_SIZE == sizeof(JaguarCIB)) &&
	(JAGUAR_MAX_IOBP_SIZE == sizeof(JaguarIOPB)) &&
	(JAGUAR_CRB_SIZE == (sizeof(JaguarCRB) - sizeof(JaguarIOPB))) &&
	(JAGUAR_SG_SIZE == sizeof(JaguarSG))
	);
}

/*
 *----------------------------------------------------------------------
 *
 * FillInScsiIOPB --
 *
 *	Fill in a Jaguar IOPB with a SCSI PASS-THRU command to send 
 *	the specified scsi command block to the specified device.
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
FillInScsiIOPB(devPtr, scsiCmdPtr, iopbPtr)
    Device	*devPtr;	/* Target device for command. */
    ScsiCmd	*scsiCmdPtr;	/* SCSI command being sent. */
    volatile JaguarIOPB	*iopbPtr;	/* IOPB to be filled in . */
{
    Address	addr;
    iopbPtr->command = JAGUAR_PASS_THRU_CMD;
    iopbPtr->options = JAGUAR_IOPB_INTR_ENA | 
		    (scsiCmdPtr->dataToDevice ? 0 : JAGUAR_IOPB_TO_HBA);
    iopbPtr->addrModifier = JAGUAR_ADDRESS_MODIFIER;
    SET_LONG(iopbPtr->maxXferLen, scsiCmdPtr->bufferLen);
    if (scsiCmdPtr->bufferLen > 0) {
	devPtr->dmaBuffer = addr = (Address) 
		VmMach_DMAAlloc(scsiCmdPtr->bufferLen, scsiCmdPtr->buffer);
    } else {
	devPtr->dmaBuffer = (Address) NIL;
	addr = (Address) VMMACH_DMA_START_ADDR;
    }
    SET_LONG(iopbPtr->bufferAddr, (unsigned)addr - VMMACH_DMA_START_ADDR);
    iopbPtr->cmd.scsiArg.length = scsiCmdPtr->commandBlockLen;
    iopbPtr->cmd.scsiArg.unitAddress = devPtr->unitAddress;
    bcopy(scsiCmdPtr->commandBlock, (char *) iopbPtr->cmd.scsiArg.cmd, 
			    scsiCmdPtr->commandBlockLen);
}


/*
 *----------------------------------------------------------------------
 *
 * ScsiErrorProc --
 *
 *	This function retrieves the Sense data from a device and 
 *	Unfreezes the queue to the device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
ScsiErrorProc(data, callInfoPtr)
    ClientData          data;
    Proc_CallInfo       *callInfoPtr;
{
    Device	*devPtr = (Device *) data;
    volatile JaguarIOPB iopbMem;	
    JaguarCRB	crb;
    ScsiCmd	senseCmd;
    char	senseBuffer[DEV_MAX_SENSE_BYTES];

    DevScsiSenseCmd((ScsiDevice *)devPtr, DEV_MAX_SENSE_BYTES, senseBuffer, 
			&senseCmd);
    FillInScsiIOPB(devPtr, &senseCmd, &iopbMem);
    (void) SendJaguarCmd(devPtr->ctrlPtr, 0, &iopbMem, FILL_IN_CRB_ACTION, 
			  (ClientData) &crb);
    /*
     * Ignore the 0x34 ( transfer length mismatch) we're likely to get.
     */
    if (crb.iopb.returnStatus & 0xff) {
	crb.iopb.returnStatus &= ~0xff;
    }
    if (crb.iopb.returnStatus != 0) {
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr, 
				   SUCCESS, devPtr->frozen.statusByte,
				   devPtr->frozen.amountTransferred,
				   0, (char *) 0);
    } else {
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr, 
				   SUCCESS, devPtr->frozen.statusByte,
				   devPtr->frozen.amountTransferred,
				   READ_LONG(crb.iopb.maxXferLen), senseBuffer);

    }
    /*
     * Unfreze the workqueue for this device. 
     */
    devPtr->ctrlPtr->memPtr->mcsb.thawQueue = 
			THAW_WORK_QUEUE(devPtr->workQueue);
    MACH_DELAY(100);
}


/*
 *----------------------------------------------------------------------
 *
 * RequestDone --
 *
 *	Process a request that has finished. Unless a SCSI check condition
 *	bit is present in the status returned, the request call back
 *	function is called.  If check condition is set we fire off a
 *	SCSI REQUEST SENSE to get the error sense bytes from the device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The call back function may be called.
 *
 *----------------------------------------------------------------------
 */

static void
RequestDone(devPtr,scsiCmdPtr,status,scsiStatusByte,amountTransferred)
    Device	*devPtr;	/* Device for request. */
    ScsiCmd	*scsiCmdPtr;	/* Request that finished. */
    ReturnStatus status;	/* Status returned. */
    unsigned char scsiStatusByte;	/* SCSI Status Byte. */
    int		amountTransferred; /* Amount transferred by command. */
{
    if (devJaguarDebug > 3) {
	printf("RequestDone for %s status 0x%x scsistatus 0x%x count %d\n",
	    devPtr->handle.locationName, status,scsiStatusByte,
	    amountTransferred);
    }
    if (devPtr->dmaBuffer != (Address) NIL) {
	VmMach_DMAFree(scsiCmdPtr->bufferLen, devPtr->dmaBuffer);
    }
    /*
     * If the request 
     * suffered an error or the HBA or the scsi status byte
     * says there is no error sense present, we can do the
     * callback and free the controller.
     */
    if ((status != SUCCESS) || !SCSI_CHECK_STATUS(scsiStatusByte)) {
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	 return;
   } 
   /*
    * If we got here than the SCSI command came back from the device
    * with the CHECK bit set in the status byte. We do this with 
    * a call back process that can wait for workQueue 0 to become
    * available.
    */
   devPtr->frozen.scsiCmdPtr = scsiCmdPtr;
   devPtr->frozen.statusByte = scsiStatusByte;
   devPtr->frozen.amountTransferred = amountTransferred;
   Proc_CallFunc(ScsiErrorProc, (ClientData) devPtr, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseProc --
 *
 *	Release an attached Jaguar device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGUSED*/
static ReturnStatus
ReleaseProc(scsiDevicePtr)
    ScsiDevice	*scsiDevicePtr;
{
    return SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * entryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for this
 *	controller. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for this controller. 
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *	NOTE: This routine is also called from DevSCSI3Intr to start the
 *	next request after the previously one finishes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Request may be dequeue and submitted to the device. Request callback
 *	function may be called.
 *
 *----------------------------------------------------------------------
 */

static Boolean
entryAvailProc(clientData, newRequestPtr) 
   ClientData	clientData;	/* Really the Device this request ready. */
   List_Links *newRequestPtr;	/* The new SCSI request. */
{
    register Device *devPtr = (Device *) clientData;
    register Controller *ctrlPtr = devPtr->ctrlPtr;
    register ScsiCmd	*scsiCmdPtr = (ScsiCmd *) newRequestPtr;
    ReturnStatus	status;


    if (devPtr->numActiveCmds >= MAX_CMDS_QUEUED) { 
	return FALSE;
    }
    devPtr->numActiveCmds++;
    status = SendScsiCommand(devPtr, scsiCmdPtr);
    /*	
     * If the command couldn't be started do the callback function.
     */
    if (status != SUCCESS) {
	 devPtr->numActiveCmds--;
	 MASTER_UNLOCK(&(ctrlPtr->mutex));
	 RequestDone(devPtr,scsiCmdPtr,status,0,0);
	 MASTER_LOCK(&(ctrlPtr->mutex));
    }
    return TRUE;

}   

/*
 *----------------------------------------------------------------------
 *
 * StartNextRequest --
 *
 *	Start the next request on the device. 
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
StartNextRequest(devPtr)
    Device	*devPtr;	/* Device to start request on. */
{
    List_Links	*newRequest;

    while (devPtr->numActiveCmds < MAX_CMDS_QUEUED) { 
	newRequest = Dev_QueueGetNext(devPtr->queue);
	if (newRequest == (List_Links *) NIL) {
	    break;
	}
	entryAvailProc((ClientData) devPtr, newRequest);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DevJaguarInit --
 *
 *	Check for the existant of the Jaguar HBA controller. If it
 *	exists allocate data stuctures for it.
 *
 * Results:
 *	TRUE if the controller exists, FALSE otherwise.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *----------------------------------------------------------------------
 */
ClientData
DevJaguarInit(ctrlLocPtr)
    DevConfigController	*ctrlLocPtr;	/* Controller location. */
{
    int	ctrlNum;
    Controller *ctrlPtr;
    short	x;
    int		i;
    Address	address;
    ReturnStatus status;

    /*
     * See if the controller is there. This controller should occupy
     * 2k in the short IO space of the VME.
     */
    if (ctrlLocPtr->space != DEV_VME_D16A16) {
	panic("Jaguar SCSI HBA %d configured in bad address space %d @ 0x%x\n",
	      ctrlLocPtr->controllerID, ctrlLocPtr->space, ctrlLocPtr->address);
	return DEV_NO_CONTROLLER;
    }

    address =	(Address) ctrlLocPtr->address;
    status = Mach_Probe(sizeof(short), address, (char *)&x);
    if (status == SUCCESS) {
	status = Mach_Probe(sizeof(short), address + 2*1024 - 2,(char *)&x);
    }
    if (status != SUCCESS) {
	if (devJaguarDebug > 3) {
	    printf("Jaguar # %d not found at address 0x%x\n",
		   ctrlLocPtr->controllerID, address);
	}
	return DEV_NO_CONTROLLER;
    }
    if (!CheckSizes()) {
	panic("Jaguar driver structure layout broken\n");
	return DEV_NO_CONTROLLER;
    }
    ctrlNum = ctrlLocPtr->controllerID;
    { 
	Boolean	good;
	good = InitializeJaguar((volatile JaguarMem *) address,
	                     ctrlLocPtr->name,
			     VME_INTERRUPT_PRIORITY,
			     ctrlLocPtr->vectorNumber);
	if (!good) {
	    return DEV_NO_CONTROLLER;
	}
    }
    ctrlPtr = Controllers[ctrlNum] = (Controller *) malloc(sizeof(*ctrlPtr));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->memPtr = (JaguarMem *) address;
    ctrlPtr->name = ctrlLocPtr->name;
    Sync_SemInitDynamic(&(ctrlPtr->mutex),ctrlPtr->name);
    /* 
     * Initialized the name, device queue header, and the master lock.
     * The controller comes up with no devices active and no devices
     * attached.  
     */
    ctrlPtr->devQueues = Dev_CtrlQueuesCreate(&(ctrlPtr->mutex),entryAvailProc);
    for (i = 0; i < NUM_WORK_QUEUES; i++) {
	ctrlPtr->devices[i] = (Device *) NIL;
    }
    ctrlPtr->intrLevel = VME_INTERRUPT_PRIORITY;
    ctrlPtr->intrVector = ctrlLocPtr->vectorNumber;

    return (ClientData) ctrlPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * DevJaguarAttachDevice --
 *
 *	Attach a SCSI device using the Jaguar HBA. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ScsiDevice   *
DevJaguarAttachDevice(devicePtr, insertProc)
    Fs_Device	*devicePtr;	 /* Device to attach. */
    void	(*insertProc)(); /* Queue insert procedure. */
{
    Device *devPtr;
    Controller	*ctrlPtr;
    char   tmpBuffer[512];
    int	   length;
    int	   ctrlNum;
    int	   targetID, lun, bus;
    int	   i, workQueue;

    /*
     * First find the Jaguar controller this device is on. For the Jaguar
     * we really have 2 HBA per real controller because each HBA has 
     * two SCSI buses.
     */
    ctrlNum = SCSI_HBA_NUMBER(devicePtr)/2;
    if ((ctrlNum > MAX_JAGUAR_CTRLS) ||
	(Controllers[ctrlNum] == (Controller *) 0)) { 
	return (ScsiDevice  *) NIL;
    } 
    ctrlPtr = Controllers[ctrlNum];
    /*
     * See if the device is already present.
     */
    targetID = SCSI_TARGET_ID(devicePtr);
    lun = SCSI_LUN(devicePtr);
    bus = SCSI_HBA_NUMBER(devicePtr) & 0x1;
    MASTER_LOCK(&(ctrlPtr->mutex));
    /*
     * See if we already have a work queue setup for this device. 
     */
    workQueue = -1;
    for (i = 0; i < NUM_WORK_QUEUES; i++) {
	if (ctrlPtr->devices[i] != (Device *) NIL) {
	    if (ctrlPtr->devices[i]->targetID == targetID) {
		if (ctrlPtr->devices[i]->handle.LUN == lun) {
		    MASTER_UNLOCK(&(ctrlPtr->mutex));
		    return (ScsiDevice  *) (ctrlPtr->devices[i]);
		}
		/*
		 * The same targetID and a different LUN doesn't work.
		 */
		 MASTER_UNLOCK(&(ctrlPtr->mutex));
		return (ScsiDevice *) NIL;
	    }
	} else {
	    workQueue = i+1;
	}
    }
    if (workQueue == -1) {
	return (ScsiDevice *) NIL;
    }

    MASTER_UNLOCK(&(ctrlPtr->mutex));
    if (!InitializeWorkq(ctrlPtr, workQueue, TRUE, 1)) {
	return (ScsiDevice *) NIL;
    }

    ctrlPtr->devices[workQueue-1] = devPtr =
					(Device *) malloc(sizeof(Device));
    bzero((char *) devPtr, sizeof(Device));
    devPtr->unitAddress = JAGUAR_UNIT_ADDRESS(bus, targetID, lun);
    devPtr->workQueue = workQueue;
    devPtr->ctrlPtr = ctrlPtr;
    devPtr->handle.devQueue = Dev_QueueCreate(ctrlPtr->devQueues,
				0, insertProc, (ClientData) devPtr);
    devPtr->handle.releaseProc = ReleaseProc;
    devPtr->handle.LUN = lun;
    devPtr->handle.maxTransferSize = DEV_MAX_DMA_SIZE;
    (void) sprintf(tmpBuffer, "%s#%d Target %d LUN %d", ctrlPtr->name, ctrlNum,
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);
    return (ScsiDevice *) devPtr;
}

