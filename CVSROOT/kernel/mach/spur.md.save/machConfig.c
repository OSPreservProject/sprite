/*
 * machConfig.c --
 *
 * 	Routines and data structures specifying the configuration of a 
 *	SPUR processor.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#include "sprite.h"
#include "mach.h"
#include "machInt.h"
#include "machConfig.h"

/*
 * The number of slots for boards in a SPUR processor.  A NuBus may have at
 * most 16 slots.
 */

#define	MACH_NUM_BUS_SLOTS	16

/*
 * Definitions of each slot in the SPUR system. Currently these values are
 * hardwired. 
 *
 * Slot 0xf - 8 megs of memory.
 * Slot 0xe - ethernet board.
 * Slot 0xd - 1/2 meg of memory for booting. (Not used by Sprite. )
 * Slot 0x9 - The processor board.
 */
#ifndef UNI
Mach_Board  machConfig[MACH_NUM_BUS_SLOTS] = {
/* SlotId 	BoardType		number		flags		*/
{ 0x0, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x1, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x2, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x3, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x4, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x5, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x6, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x7, MACH_CONFIG_CPU_BOARD,		0,	MACH_CONFIG_MASTER_FLAG }, 
{ 0x8, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG},
{ 0x9, MACH_CONFIG_CPU_BOARD,		2,	0 },
{ 0xA, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0xB, MACH_CONFIG_CPU_BOARD,		1,	0 },
{ 0xC, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0xD, MACH_CONFIG_CPU_BOARD,		3,	0 }, 
{ 0xE, MACH_CONFIG_ETHER_BOARD,		0,	0 },
{ 0xF, MACH_CONFIG_8_MEG_BOARD,		0,	MACH_CONFIG_INITIALIZED_FLAG |
						MACH_CONFIG_KERNEL_MEM_FLAG}
};
#else
Mach_Board  machConfig[MACH_NUM_BUS_SLOTS] = {
/* SlotId 	BoardType		number		flags		*/
{ 0x0, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x1, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x2, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x3, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x4, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x5, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x6, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x7, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_MASTER_FLAG },
{ 0x8, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0x9, MACH_CONFIG_NO_BOARD,		0,	0 },
{ 0xA, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0xB, MACH_CONFIG_CPU_BOARD,		0,	MACH_CONFIG_MASTER_FLAG},
{ 0xC, MACH_CONFIG_NO_BOARD,		0,	MACH_CONFIG_NO_ACCESS_FLAG },
{ 0xD, MACH_CONFIG_HALF_MEG_BOARD,		0,	MACH_CONFIG_INITIALIZED_FLAG |
						MACH_CONFIG_NO_ACCESS_FLAG },
{ 0xE, MACH_CONFIG_ETHER_BOARD,		0,	0 },
{ 0xF, MACH_CONFIG_8_MEG_BOARD,		0,	MACH_CONFIG_INITIALIZED_FLAG |
						MACH_CONFIG_KERNEL_MEM_FLAG}
};
#endif
/*
 * Map processor number to slot ID.
 */
int	machMapPnumToSlotId[MACH_MAX_NUM_PROCESSORS];


/*
 *----------------------------------------------------------------------
 *
 * Mach_FindBoardDescription --
 *
 *	Find the Mach_Board data structure  of the specified board.
 *
 * Results:
 *      SUCCESS if board's is in system,
 *      FAILURE otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Mach_FindBoardDescription(boardType, number, wildcard, machBoardPtr)
    int		boardType;		/* Type of board. */
    int		number;			/* Board number. */
    Boolean	wildcard;		/* TRUE if wild card matching possible.
					 */
    Mach_Board	*machBoardPtr;		/* Return Mach_Board entry */
{
    int		slotId;

    /*
     * For each NuBus slot. 
     */
    for (slotId = 0; slotId < MACH_NUM_BUS_SLOTS; slotId++) {
	/* 
	 * If the board type match or wildcard is specified and
	 * the wild card hits. 
	 */
	if ((machConfig[slotId].boardType == boardType) ||
	    (wildcard && (machConfig[slotId].boardType & boardType))) {
	    if (machConfig[slotId].number == number &&
		!(machConfig[slotId].flags & MACH_CONFIG_NO_ACCESS_FLAG)) {
		*machBoardPtr = machConfig[slotId];
		return (SUCCESS);
	    }
	}
    }
    return (FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_ConfigInit --
 *
 *	Initialize the system configuration description.
 *
 * Results: None
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
Mach_ConfigInit()
{
    ReturnStatus	status;
    int			pnum;
    Mach_Board		machBoard;

	/*
	 * Current the system configuation is hardwired. No initialized is
	 * need on this structure.
	 */
     if (MachMemBoardSize(0xf) == 0x2000000) {
	machConfig[0].slotId = 0x0;
	machConfig[0].boardType = MACH_CONFIG_24_MEG_BOARD;
	machConfig[0].number = 1;
	machConfig[0].flags = MACH_CONFIG_MUST_MAP | MACH_CONFIG_SPLIT_MEM;
     }
    /*
     * Generate the  slot Id Map to processor.
     */
    for (pnum = 0; pnum < MACH_MAX_NUM_PROCESSORS; pnum++) {
	status = Mach_FindBoardDescription(MACH_CONFIG_CPU_BOARD, pnum, FALSE,
			&machBoard);
	if (status == SUCCESS) {
	    machMapSlotIdToPnum[machBoard.slotId] = pnum;
	    machMapPnumToSlotId[pnum] = machBoard.slotId;
	} 
    }
}


/*
/*
 *----------------------------------------------------------------------
 *
 * Mach_ConfigMemSize --
 *
 *	Return the size in bytes of a memory board.
 *
 * Results: None
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */


int
Mach_ConfigMemSize(board)
    Mach_Board	board;
{
	int		halfMegs, size;

	switch (board.boardType) {

	case MACH_CONFIG_HALF_MEG_BOARD: {
		halfMegs = 1; 
		break; 
	}
	case MACH_CONFIG_2_MEG_BOARD: {
		halfMegs = 4; 
		break; 
	}
	case MACH_CONFIG_8_MEG_BOARD: {
		halfMegs = 16; 
		break;
	}
	case MACH_CONFIG_16_MEG_BOARD: {
		halfMegs = 32; 
		break;
	}
	case MACH_CONFIG_32_MEG_BOARD: {
		halfMegs = 64; 
		break; 
	}
	case MACH_CONFIG_24_MEG_BOARD: {
		halfMegs = 48; 
		break; 
	}
	default: {
		panic("Unknown memory board type (%d)\n",board.boardType); 
	}
	}; 
	size = (halfMegs * (512 * 1024));
	if (board.flags & MACH_CONFIG_KERNEL_MEM_FLAG) {
	    size -= MACH_NUM_RESERVED_PAGES * VMMACH_PAGE_SIZE;
	}

	return (size);
}


/*
/*
 *----------------------------------------------------------------------
 *
 * Mach_ConfigInitMem --
 *
 *	Initialized and return the starting location of a memory board.
 *
 * Results: None
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

unsigned int
Mach_ConfigInitMem(board)
    Mach_Board	board;
{
    unsigned int	startAddress;
    int	     size;
    if (!(board.boardType & MACH_CONFIG_MEMORY_MASK)) {
	panic("Not a memory board in Mach_ConfigInitMem");
    }
    startAddress = ((0xf0 | board.slotId) << 24);
    if (!(board.flags & MACH_CONFIG_INITIALIZED_FLAG) ) {
	size = Mach_ConfigMemSize(board);
	if (board.flags & MACH_CONFIG_MUST_MAP) {
	    if (board.flags | MACH_CONFIG_SPLIT_MEM) {
		    Mach_WritePhysicalWord(0xffffc008,0x10*board.number);
		    startAddress = 0x10000000*board.number+(8*1024*1024);
	    } else {
		    Mach_WritePhysicalWord(0xf0ffc008 | (board.slotId << 24)
							,0x10*board.number);
		    startAddress = 0x10000000*board.number;
	    }
	}
	MachZeroMemBoard(startAddress,startAddress+(size-1));
    }
    /*
     * Assume that we address board in slot space.
     */
    if (board.flags & MACH_CONFIG_KERNEL_MEM_FLAG) {
	startAddress += (MACH_NUM_RESERVED_PAGES * VMMACH_PAGE_SIZE);
    }
    return (startAddress);
}

