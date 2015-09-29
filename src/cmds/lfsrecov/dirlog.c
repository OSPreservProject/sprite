/* 
 * dirlog.c --
 *
 *	Directory change log manipulation routines for the lfsrecov program.
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
#include "dirlog.h"
#include "desc.h"
#include "fileop.h"
#include "usage.h"

/*
 * startEntryHashTable -
 * Hash table data structures used to record the addresses of log start
 * entries for future reference when the end entry is found.
 * startEntryHashTableInit - 
 * Set to TRUE when the hash table startEntryHashTable has been initialized.
 */
static Hash_Table startEntryHashTable;
static Boolean    startEntryHashTableInit = FALSE;

/*
 *----------------------------------------------------------------------
 *
 * RecordLogEntryStart --
 *
 *	Record the address of an FSDM_LOG_START_ENTRY that is not 
 *	also the END entry so we can match it will the END entry when
 *	we encounter it.
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
RecordLogEntryStart(entryPtr, addr)
    LfsDirOpLogEntry *entryPtr;	/* Log entry with start bit to be recorded. */
    int		     addr;	/* Disk address of log entry. */
{
    Hash_Entry *hentryPtr;
    Boolean new;

    assert((entryPtr->hdr.opFlags & FSDM_LOG_START_ENTRY) &&
            !(entryPtr->hdr.opFlags & FSDM_LOG_END_ENTRY));

    if (!startEntryHashTableInit) {
	Hash_InitTable(&startEntryHashTable, 0, HASH_ONE_WORD_KEYS);
	startEntryHashTableInit = TRUE;
    } 

    hentryPtr = Hash_CreateEntry(&startEntryHashTable, 
			(Address) entryPtr->hdr.logSeqNum, &new);
    if (!new) {
	panic("LogStart entry %d already in has table\n", 
			entryPtr->hdr.logSeqNum);
    }
    Hash_SetValue(hentryPtr, (ClientData)addr);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FindStartEntryAddr --
 *
 *	Find the address of the START entry of the specified log
 *	sequence number. This routine is used to find the range of
 *	blocks between a START and END entry.
 *
 * Results:
 *	TRUE if the start entry's address is found. FALSE if no entry
 * 	was record under that seqence number.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

Boolean
FindStartEntryAddr(logSeqNum, addrPtr)
    int		logSeqNum;	/* Log seq number of entry we wish to lookup.*/
    int		*addrPtr;	/* OUT: Address of start entry record. */
{

    Hash_Entry *hentryPtr;

    if (!startEntryHashTableInit) {
	Hash_InitTable(&startEntryHashTable, 0, HASH_ONE_WORD_KEYS);
	startEntryHashTableInit = TRUE;
    } 
    hentryPtr = Hash_FindEntry(&startEntryHashTable, (Address)logSeqNum);
    if (hentryPtr == (Hash_Entry *) NULL) {
	return FALSE;
    }
    (*addrPtr) = (int) Hash_GetValue(hentryPtr);
    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * DirOpFlagsToString --
 *
 *	Convert a directory change opcode into a printable strings.
 *
 * Results:
 *	A printable string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
DirOpFlagsToString(opFlags)
    int opFlags;
{
    int op = opFlags & FSDM_LOG_OP_MASK;
    char buffer[128];
    static char *opcodes[] = {"UNKNOWN", "CREATE", "UNLINK", "LINK", 
			"RENAME_DELETE", "RENAME_LINK", "RENAME_UNLINK", 
			"UNKNOWN"};
    if ((op < 0) || (op >= sizeof(opcodes)/sizeof(opcodes[0]))) {
	op = sizeof(opcodes)/sizeof(opcodes[0])-1;
    }
    strcpy(buffer,opcodes[op]);
    if (opFlags & FSDM_LOG_STILL_OPEN) {
	strcat(buffer, "-OPEN");
	opFlags ^= FSDM_LOG_STILL_OPEN;
    }
    if (opFlags & FSDM_LOG_START_ENTRY) {
	strcat(buffer, "-START");
	opFlags ^= FSDM_LOG_START_ENTRY;
    }
    if (opFlags & FSDM_LOG_END_ENTRY) {
	strcat(buffer, "-END");
	opFlags ^= FSDM_LOG_END_ENTRY;
    }
    if (opFlags & FSDM_LOG_IS_DIRECTORY) {
	strcat(buffer, "-DIR");
	opFlags ^= FSDM_LOG_IS_DIRECTORY;
    }
    if (opFlags & ~FSDM_LOG_OP_MASK) {
	strcat(buffer, "-UNKNOWN");
    }
    return buffer;

}


/*
 *----------------------------------------------------------------------
 *
 * ShowDirLogBlock --
 *
 *	Print the contents of a directory log block.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


extern void
ShowDirLogBlock(hdrPtr, addr)
    LfsDirOpLogBlockHdr *hdrPtr;  /* Header of log block. */
    int addr;			  /* Address of log block. */
{
    LfsDirOpLogEntry *entryPtr, *limitPtr;

    limitPtr = (LfsDirOpLogEntry *) (((char *) hdrPtr) + hdrPtr->size);
    entryPtr = (LfsDirOpLogEntry *) (hdrPtr+1);
    while (entryPtr < limitPtr) {
	entryPtr->dirEntry.fileName[entryPtr->dirEntry.nameLength] = '\0';
	printf("Addr %d LogSeqNum %d %s %d \"%s\" links %d in %d at %d\n",
		addr,
		entryPtr->hdr.logSeqNum, 
		DirOpFlagsToString(entryPtr->hdr.opFlags), 
		entryPtr->dirEntry.fileNumber, 
		entryPtr->dirEntry.fileName,
		entryPtr->hdr.linkCount,
		entryPtr->hdr.dirFileNumber,
		entryPtr->hdr.dirOffset);
	entryPtr = (LfsDirOpLogEntry *) 
		     (((char *)entryPtr) + LFS_DIR_OP_LOG_ENTRY_SIZE(entryPtr));
    }

}


/*
 *----------------------------------------------------------------------
 *
 * RecovDirLogBlock --
 *
 *	Process recovery of the directory change log.
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
RecovDirLogBlock(hdrPtr, addr, pass)
    LfsDirOpLogBlockHdr *hdrPtr;	/* Header of log block. */
    int addr;				/* Address of log block. */
    enum Pass pass;			/* Pass of recovery. */
{
    int	fileNumber;
    LfsDirOpLogEntry *entryPtr, *limitPtr;


    limitPtr = (LfsDirOpLogEntry *) (((char *) hdrPtr) + hdrPtr->size);
    entryPtr = (LfsDirOpLogEntry *) (hdrPtr+1);
    /*
     * Exmaine each enry in the log blocks. 
     */
    while (entryPtr < limitPtr) {
	int	op;
	op = entryPtr->hdr.opFlags & FSDM_LOG_OP_MASK;
	fileNumber = entryPtr->dirEntry.fileNumber;
	if (pass == PASS1) { 
	    stats.numDirLogEntries++;
	}
	switch (op) {
	    case FSDM_LOG_RENAME_DELETE:
	    case FSDM_LOG_UNLINK: {
		if (pass == PASS1) {
		    if (entryPtr->hdr.linkCount == 0) {
			if((entryPtr->hdr.opFlags & FSDM_LOG_STILL_OPEN) == 0) {
			    stats.numDirLogDelete++;
			    RecordNewDesc(fileNumber, addr, 
					(LfsFileDescriptor *) NIL);
			} else {
			    stats.numDirLogDeleteOpen++;
			    RecordUnrefDesc(fileNumber, 
					entryPtr->hdr.dirFileNumber,
					&entryPtr->dirEntry);
			}
		    } else {
			stats.numDirLogUnlink++;
		    }
		} else {
		    /*
		     * Recover the operation. 
		     */
		    RecovDirLogEntry(entryPtr, addr);
		}
		break;
	    }
	    case FSDM_LOG_CREATE:
	    case FSDM_LOG_RENAME_UNLINK:
	    case FSDM_LOG_LINK:
	    case FSDM_LOG_RENAME_LINK:
		if (pass == PASS2) {
		    stats.numDirLogCreate++;
		    RecovDirLogEntry(entryPtr, addr);
		}
		break;
	    default:
		fprintf(stderr,"%s:Unknown log entry 0x%x\n", deviceName,
				entryPtr->hdr.opFlags);
		break;
	}
	entryPtr = (LfsDirOpLogEntry *) 
		     (((char *)entryPtr) + LFS_DIR_OP_LOG_ENTRY_SIZE(entryPtr));
    }

}

/*
 *----------------------------------------------------------------------
 *
 * RecovDirLogEntry --
 *
 *	Recover changes record in a directory log entry.
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
RecovDirLogEntry(entryPtr, addr)
    LfsDirOpLogEntry *entryPtr;
    int		     addr;
{
    enum LogStatus	dirBlockStatus, descStatus;
    int	logStartAddr;
    Boolean  mayExist;
    ReturnStatus status;
    int	op = entryPtr->hdr.opFlags & FSDM_LOG_OP_MASK;

    if ((entryPtr->hdr.opFlags & FSDM_LOG_START_ENTRY) &&
        !(entryPtr->hdr.opFlags & FSDM_LOG_END_ENTRY)) {
	/*
	 * Record any START entry that wasn't finished. It will be
	 * processed when we find the END entry or the end-of-log.
	 */
	stats.numDirLogWithoutEnd++;
	RecordLogEntryStart(entryPtr, addr);
	return;
    } 
    /*
     * Find the status of the operands (the file and the directory).
     */
    logStartAddr = addr;
    if(!(entryPtr->hdr.opFlags & FSDM_LOG_START_ENTRY)) {
	/*
	 * Find address of start entry.
	 */
	if (!FindStartEntryAddr(entryPtr->hdr.logSeqNum, &logStartAddr)) {
	    fprintf(stderr, "No starting log entry for %d\n", entryPtr->hdr.logSeqNum);
	    logStartAddr = addr;
	}
    }

    dirBlockStatus = DirBlockStatus( entryPtr->hdr.dirFileNumber, 
				  entryPtr->hdr.dirOffset, logStartAddr,
				  addr); 
    descStatus = DescStatus(entryPtr->dirEntry.fileNumber, logStartAddr, addr);

    mayExist = (dirBlockStatus == UNKNOWN);
    if (descStatus == FORWARD) { 
	/*
	 * The inode made it out, all we need to do is make sure that
	 * the directory change did to.
	 */
	switch (dirBlockStatus) {
	    case FORWARD:
		/*
		 * Both the desc and the directory block made it out. The are 
		 * no changes except moving forward the descriptor map.
		 */
		stats.dirLogBothForward++;
		break;
	    case BACKWARD: 
	    case UNKNOWN:
		/*
		 * The desc made it out but the directory block didn't.  
		 * Correct directory entry.
		 */
		stats.dirLogDirBlockBackward++;
		if ((op == FSDM_LOG_CREATE) || (op == FSDM_LOG_LINK) ||
		    (op == FSDM_LOG_RENAME_LINK)) { 
		    status = AddEntryToDirectory(entryPtr->hdr.dirFileNumber, 
			entryPtr->hdr.dirOffset, &entryPtr->dirEntry, 
			(entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),
			mayExist);
		    if (status == FS_FILE_NOT_FOUND) {
			status = CreateLostDirectory(
					entryPtr->hdr.dirFileNumber);
			if (status == SUCCESS) {
			    status = AddEntryToDirectory(
					entryPtr->hdr.dirFileNumber, 
					entryPtr->hdr.dirOffset, 
					&entryPtr->dirEntry, 
					(entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),
					FALSE);
		        }

		    }
		    if (status != SUCCESS) {
			fprintf(stderr,"Can't rollforward CREATE\n"); 
			exit(1);
		    }


		} else if ((op == FSDM_LOG_UNLINK) || 
		           (op == FSDM_LOG_RENAME_UNLINK)) {
		    RemovedEntryFromDirectory( entryPtr->hdr.dirFileNumber, 
			  entryPtr->hdr.dirOffset, &entryPtr->dirEntry, 
			  (entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),
			  TRUE);
		}
		break;
	    default: 
		panic("Unknown dirBlockStatus status %d\n", dirBlockStatus);
		break;
	 }
	 return;
    } 
    /*
     * The descriptor may or may not have made it out. We can roll forward 
     * everything but creates.
     */
    if (op == FSDM_LOG_CREATE) {
	  /*
	   * We assume that an unknown descriptor really means it made
	   * it out. 
	   */
	  if (descStatus == BACKWARD) { 
	      stats.dirLogBothBackwardCreate++;
	      RecordLostCreate(entryPtr->hdr.dirFileNumber,&entryPtr->dirEntry);
	      if ((dirBlockStatus == FORWARD) || (dirBlockStatus == UNKNOWN)) {
		  RemovedEntryFromDirectory( entryPtr->hdr.dirFileNumber, 
			 entryPtr->hdr.dirOffset, &entryPtr->dirEntry, 
			 (entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),TRUE);
	      } 
	  } else if (descStatus == UNKNOWN) {
	      if ((dirBlockStatus == BACKWARD) || (dirBlockStatus == UNKNOWN)) {
		AddEntryToDirectory(entryPtr->hdr.dirFileNumber, 
			    entryPtr->hdr.dirOffset, &entryPtr->dirEntry,
			    (entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),
			    (dirBlockStatus == UNKNOWN));
	      }
	  }
	  return;
    } 
    /*
     * It is some other operation that changed the link count.  Just
     * update the link count to reflect the change.
     */
    UpdateDescLinkCount(OP_ABS, entryPtr->dirEntry.fileNumber, 
			entryPtr->hdr.linkCount);
    if ((dirBlockStatus == BACKWARD) || (dirBlockStatus == UNKNOWN)) {
	stats.dirLogBothBackward++;
	if ((op == FSDM_LOG_LINK) ||
	    (op == FSDM_LOG_RENAME_LINK)) {
		status = AddEntryToDirectory(entryPtr->hdr.dirFileNumber, 
		    entryPtr->hdr.dirOffset, &entryPtr->dirEntry, 
		    (entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY), mayExist);
		if (status == FS_FILE_NOT_FOUND) {
		    status = CreateLostDirectory(
				    entryPtr->hdr.dirFileNumber);
		    if (status == SUCCESS) {
			status = AddEntryToDirectory(
				    entryPtr->hdr.dirFileNumber, 
				    entryPtr->hdr.dirOffset, 
				    &entryPtr->dirEntry, 
				    (entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),
				    mayExist);
		    }

		}
		if (status != SUCCESS) {
		    fprintf(stderr,"Can't rollforward change\n");
		    exit(1);
		}
	} else if ((op == FSDM_LOG_UNLINK) ||
		   (op == FSDM_LOG_RENAME_UNLINK)) {
	    RemovedEntryFromDirectory(entryPtr->hdr.dirFileNumber, 
			     entryPtr->hdr.dirOffset, &entryPtr->dirEntry, 
			     (entryPtr->hdr.opFlags & FSDM_LOG_IS_DIRECTORY),
			     TRUE);
       }
    }
}




