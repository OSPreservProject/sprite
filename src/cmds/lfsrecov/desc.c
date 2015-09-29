/* 
 * desc.c --
 *
 *	File descriptor manipulation routines for the lfsrecov program.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.5 91/02/09 13:24:44 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <bstring.h>
#include <unistd.h>
#include <bit.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <hash.h>
#include <libc.h>

#include "lfsrecov.h"
#include "desc.h"

/*
 * descHashTable -
 * Hash table data structures used to record the addresses of inode
 * written out to the log.
 * descHashHashTableInit - 
 * Set to TRUE when the hash table startEntryHashTable has been initialized.
 */
static Hash_Table descHashTable;
static Boolean    descHashTableInit = FALSE;

typedef struct NewDescValue { 
    int address;		/* Disk address of descriptor or address
				 * of delete record. */
    LfsFileDescriptor *descPtr;	/* New LfsFileDescriptor. NIL if deleted*/
} NewDescValue;


/*
 *----------------------------------------------------------------------
 *
 * RecordNewDesc --
 *
 *	Record the address of file descriptor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An entry is made in the startEntryHashTable.
 *
 *----------------------------------------------------------------------
 */

void
RecordNewDesc(fileNumber, address, descPtr)
    int	fileNumber;	/* File number of descriptor. */
    int	address;	/* Disk address of descriptor. FSDM_NIL_INDEX if
			 * descriptor being delete. */
    LfsFileDescriptor *descPtr; /* New descriptor. */
{
    Hash_Entry *hentryPtr;
    NewDescValue	*valuePtr;
    Boolean new;


    if (!descHashTableInit) {
	Hash_InitTable(&descHashTable, 0, HASH_ONE_WORD_KEYS);
	descHashTableInit = TRUE;
    } 

    hentryPtr = Hash_CreateEntry(&descHashTable, (Address) fileNumber, 
				&new);
    if (!new) { 
	valuePtr = (NewDescValue *) Hash_GetValue(hentryPtr);
    } else {
	valuePtr = (NewDescValue *) malloc(sizeof(NewDescValue));
	valuePtr->descPtr = (LfsFileDescriptor *) NIL;
    }
    if (descPtr == (LfsFileDescriptor *) NIL) {
	if (valuePtr->descPtr != (LfsFileDescriptor *) NIL) {
	    free((char *) (valuePtr->descPtr));
	}
	valuePtr->address = address;
	valuePtr->descPtr = descPtr;
    } else {
	if (valuePtr->descPtr == (LfsFileDescriptor *) NIL) {
	    valuePtr->descPtr = (LfsFileDescriptor *) 
			malloc(sizeof(LfsFileDescriptor));
	} 
	valuePtr->address = address;
	bcopy((char *) descPtr, (char *) (valuePtr->descPtr), 
		    sizeof(LfsFileDescriptor));
    }
    Hash_SetValue(hentryPtr, (ClientData)valuePtr);
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * FindNewDesc --
 *
 *	Find the new location of a inode.  
 *
 * Results:
 *	TRUE if the inode's address is found. FALSE if no entry
 * 	was record under that fileNumber.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

Boolean
FindNewDesc(fileNumber, addrPtr, descPtrPtr)
    int		fileNumber;	/* FileNubmer of entry we wish to lookup.*/
    int		*addrPtr;	/* OUT: Address of start entry record. */
    LfsFileDescriptor **descPtrPtr; /* OUT: Current file descriptor. */
{

    Hash_Entry *hentryPtr;
    NewDescValue	*valuePtr;

    if (!descHashTableInit) {
	Hash_InitTable(&descHashTable, 0, HASH_ONE_WORD_KEYS);
	descHashTableInit = TRUE;
    } 
    hentryPtr = Hash_FindEntry(&descHashTable, (Address)fileNumber);
    if (hentryPtr == (Hash_Entry *) NULL) {
	return FALSE;
    }
    valuePtr = (NewDescValue *) Hash_GetValue(hentryPtr);
    (*addrPtr) = valuePtr->address;
    (*descPtrPtr) = valuePtr->descPtr;
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * ScanNewDesc --
 *
 *	Scan the list of descriptors in the recovery log.
 *
 * Results:
 *	TRUE if more descriptors left. 
 *	FALSE if scan is done.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


Boolean
ScanNewDesc(clientDataPtr, fileNumberPtr, addressPtr, descPtrPtr)
    ClientData	*clientDataPtr;	/* Pointer to storage for this routine.
				 * MUST be initialized to NIL at start
				 * of scan. */
    int		*fileNumberPtr; /* OUT: filenumber of descriptor. */
    int		*addressPtr;	/* OUT: Disk address of descriptor. */
    LfsFileDescriptor **descPtrPtr; /* OUT: New value of descriptor. NIL if
				      * descriptor has been deleted.
				      */
{
    Hash_Entry *hentryPtr;
    Hash_Search  *searchPtr;
    NewDescValue	*valuePtr;

    if ((*clientDataPtr) == (ClientData) NIL) {
	searchPtr = (Hash_Search *) malloc(sizeof(Hash_Search));
	hentryPtr = Hash_EnumFirst(&descHashTable, searchPtr);
	(*clientDataPtr) = (ClientData) searchPtr;
    } else {
	searchPtr = (Hash_Search *) (*clientDataPtr);
	hentryPtr = Hash_EnumNext(searchPtr);
    }
    if (hentryPtr == (Hash_Entry *) NULL) {
	free((char *) searchPtr);
	(*clientDataPtr) = (ClientData) NIL;
	return FALSE;
    }
    valuePtr = (NewDescValue *) Hash_GetValue(hentryPtr);
    (*fileNumberPtr) = (int) (hentryPtr->key.ptr);
    (*addressPtr) = valuePtr->address;
    (*descPtrPtr) = valuePtr->descPtr;
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * ScanNewDescEnd --
 *
 *	End a scan of the active descriptor list.
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
ScanNewDescEnd(clientDataPtr)
ClientData	*clientDataPtr;  /* Clientdata from ScanNewDesc. */
{
    if ((*clientDataPtr) != (ClientData) NIL) {
	free((char *) (*clientDataPtr));
    }
}

#define UNREF_DESC_GROW_SIZE	100
static int *unrefDescArray;
static int unrefDescArraySize = 0;
static int nextEntryToUse = 0;


/*
 *----------------------------------------------------------------------
 *
 * RecordUnrefDesc --
 *
 *	Record a file descriptor as possibly being unreferenced.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	FileName is added to list.
 *
 *----------------------------------------------------------------------
 */
void
RecordUnrefDesc(fileNumber, dirFileNumber, dirEntryPtr)
    int	fileNumber;	/* Unreference file number. */
    int dirFileNumber;
    Fslcl_DirEntry *dirEntryPtr;
{
    if (nextEntryToUse >= unrefDescArraySize) {
	int	oldSize = unrefDescArraySize;
	unrefDescArraySize += UNREF_DESC_GROW_SIZE;
	if (oldSize == 0) {
	    unrefDescArray = (int *) malloc(unrefDescArraySize * sizeof(int));
	} else {
	    unrefDescArray = (int *) realloc((char *) unrefDescArray, 
				unrefDescArraySize * sizeof(int));
	}
    }
    unrefDescArray[nextEntryToUse] = fileNumber;
    nextEntryToUse++;
}


/*
 *----------------------------------------------------------------------
 *
 * ScanUnrefDesc --
 *
 *	Scan the list of descriptors left unreferences..
 *
 * Results:
 *	TRUE if more descriptors left. 
 *	FALSE if scan is done.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


Boolean
ScanUnrefDesc(clientDataPtr, fileNumberPtr)
    ClientData	*clientDataPtr;	/* Pointer to storage for this routine.
				 * MUST be initialized to NIL at start
				 * of scan. */
    int		*fileNumberPtr; /* OUT: filenumber of descriptor. */
{
    int	startIndex;

    if ((*clientDataPtr) == (ClientData) NIL) {
	startIndex = 0;
    } else {
	startIndex = (int) (*clientDataPtr);
    }
    if (startIndex < nextEntryToUse) {
	(*fileNumberPtr) = unrefDescArray[startIndex];
	startIndex++;
	(*clientDataPtr) = (ClientData) startIndex;
	return TRUE;
    }
    return FALSE;

}


/*
 *----------------------------------------------------------------------
 *
 * ScanUnrefDescEnd --
 *
 *	End a scan of the unreferenced desc.
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
ScanUnrefDescEnd(clientDataPtr)
ClientData	*clientDataPtr;  /* Clientdata from ScanNewDesc. */
{
}


/*
 *----------------------------------------------------------------------
 *
 * RecovDescMapSummary --
 *
 *	Check the segment summary regions for the desc map. For recovery
 *	we do nothing.
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
RecovDescMapSummary(lfsPtr, pass, segPtr, startAddress, offset, 
			segSummaryHdrPtr) 
    Lfs	*lfsPtr;	/* File system description. */
    enum Pass pass;	/* Pass number of recovery. */
    LfsSeg *segPtr;	/* Segment being examined. */
    int startAddress;   /* Starting address being examined. */
    int offset;		/* Offset into segment being examined. */
    LfsSegSummaryHdr *segSummaryHdrPtr; /* Summary header pointer */
{
    int blocks, *blockArray, i, startAddr, fsBlocks;

    if (pass == PASS2) {
	/*
	 * Ignore desc map blocks during pass2 recovery.   These don't
	 * contain anything we wont recovery already with the current
	 * algorithm except may be more up to date access times.  
	 */
	return;
    }
    /*
     * PASS1, Check to make sure that things look correct.
     */
    fsBlocks = lfsPtr->superBlock.descMap.stableMem.blockSize/blockSize;
    blocks = (segSummaryHdrPtr->lengthInBytes - sizeof(LfsSegSummaryHdr)) /
				sizeof(int);
    if (blocks * fsBlocks != segSummaryHdrPtr->numDataBlocks) {
	fprintf(stderr,"%s:RecovDescMapSummary: Wrong block count; is %d should be %s\n", deviceName,blocks * fsBlocks, segSummaryHdrPtr->numDataBlocks);
    }
    blockArray = (int *) (segSummaryHdrPtr + 1);
    for (i = 0; i < blocks; i++) {
	startAddr = startAddress - i * fsBlocks - fsBlocks;
	if ((blockArray[i] < 0) || 
	    (blockArray[i] > lfsPtr->superBlock.descMap.stableMem.maxNumBlocks)){
	   fprintf(stderr,"%s:RecovDescMapSummary: Bad block number %d at %d\n",
			deviceName,blockArray[i], startAddr);
	    continue;
	}
	if (showLog) {
	    printf("Addr %d DescMap Block %d\n", startAddr, blockArray[i]);
	}
    }
    stats.descMapBlocks += blocks;

}

