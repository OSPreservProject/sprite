/*
 * fsMigrate.c --
 *
 * Procedures to handle migrating open files between machines.  The basic
 * strategy is to first do some local book-keeping on the client we are
 * leaving, then ship state to the new client, then finally tell the
 * I/O server about it, and finish up with local book-keeping on the
 * new client.  There are three stream-type procedures used: 'migStart'
 * does the initial book-keeping on the original client, 'migEnd' does
 * the final book-keeping on the new client, and 'migrate' is called
 * on the I/O server to shift around state associated with the client.
 *
 * Copyright (C) 1985, 1988, 1989 Regents of the University of California
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
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsutil.h"
#include "fsconsist.h"
#include "fspdev.h"
#include "fsio.h"
#include "fsprefix.h"
#include "fsNameOps.h"
#include "byte.h"
#include "rpc.h"
#include "procMigrate.h"

extern int fsio_MigDebug;
#define DEBUG( format ) \
	if (fsio_MigDebug) { printf format ; }


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_InitiateMigration --
 *
 *	Return the size of the encapsulated file system state.
 *	(Note: for now, we'll let the encapsulation procedure do the same
 *	work (in part); later things can be simplified to use a structure
 *	and to keep around some info off the ClientData hook.)
 *
 * Results:
 *	SUCCESS is returned directly; the size of the encapsulated state
 *	is returned in infoPtr->size.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

/* ARGSUSED */
ReturnStatus
Fs_InitiateMigration(procPtr, hostID, infoPtr)
    Proc_ControlBlock *procPtr;			/* process being migrated */
    int hostID;					/* host to which it migrates */
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
{
    Fs_ProcessState *fsPtr;
    int numStreams;
    int streamFlagsLen;
    Fsprefix *prefixPtr;
    int cwdLength;


    fsPtr = procPtr->fsPtr;
    numStreams = fsPtr->numStreams;
    /*
     * Get the prefix for the current working directory, and its size.
     * We pass the name over so it can be opened to make sure the prefix
     * is available.
     */
    if (fsPtr->cwdPtr->nameInfoPtr == (Fs_NameInfo *)NIL) {
	panic("Fs_GetEncapSize: no name information for cwd.\n");
	return(FAILURE);
    }
    prefixPtr = fsPtr->cwdPtr->nameInfoPtr->prefixPtr;
    if (prefixPtr == (Fsprefix *)NIL) {
	panic("Fs_GetEncapSize: no prefix for cwd.\n");
	return(FAILURE);
    }
    cwdLength = Byte_AlignAddr(prefixPtr->prefixLength + 1);
    
    /*
     * When sending an array of characters, it has to be even-aligned.
     */
    streamFlagsLen = Byte_AlignAddr(numStreams * sizeof(char));
    
    /*
     * Send the groups, file permissions, number of streams, and encapsulated
     * current working directory.  For each open file, send the
     * streamID and encapsulated stream contents.
     *
     *	        data			size
     *	 	----			----
     * 		# groups		int
     *	        groups			(# groups) * int
     *		permissions		int
     *		# files			int
     *		per-file flags		(# files) * char
     *		encapsulated files	(# files) * (FsMigInfo + int)
     *		cwd			FsMigInfo + int + strlen(cwdPrefix) + 1
     */
    infoPtr->size = (4 + fsPtr->numGroupIDs) * sizeof(int) +
	    streamFlagsLen + numStreams * (sizeof(FsMigInfo) + sizeof(int)) +
	    sizeof(FsMigInfo) + cwdLength;
    return(SUCCESS);	
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_GetEncapSize --
 *
 *	Return the size of the encapsulated stream.
 *
 * Results:
 *	The size of the migration information structure.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

int
Fs_GetEncapSize()
{
    return(sizeof(FsMigInfo));
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_EncapFileState --
 *
 *	Encapsulate the file system state of a process for migration.  
 *
 * Results:
 *	Any error during stream encapsulation
 *	is returned; otherwise, SUCCESS.  The encapsulated state is placed
 *	in the area referenced by ptr.
 *
 * Side effects:
 *	None.  
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Fs_EncapFileState(procPtr, hostID, infoPtr, ptr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address ptr;			   /* Pointer to allocated buffer */
{
    Fs_ProcessState *fsPtr;
    int numStreams;
    int numGroups;
    int streamFlagsLen;
    Fs_Stream *streamPtr;
    int i;
    ReturnStatus status;
    Fsprefix *prefixPtr;
    int cwdLength;
    int size;


    fsPtr = procPtr->fsPtr;
    numStreams = fsPtr->numStreams;
    /*
     * Get the prefix for the current working directory, and its size.
     * We pass the name over so it can be opened to make sure the prefix
     * is available.
     */
    if (fsPtr->cwdPtr->nameInfoPtr == (Fs_NameInfo *)NIL) {
	panic("Fs_EncapFileState: no name information for cwd.\n");
	return(FAILURE);
    }
    prefixPtr = fsPtr->cwdPtr->nameInfoPtr->prefixPtr;
    if (prefixPtr == (Fsprefix *)NIL) {
	panic("Fs_EncapFileState: no prefix for cwd.\n");
	return(FAILURE);
    }
    cwdLength = Byte_AlignAddr(prefixPtr->prefixLength + 1);
    
    /*
     * When sending an array of characters, it has to be even-aligned.
     */
    streamFlagsLen = Byte_AlignAddr(numStreams * sizeof(char));
    
    /*
     * Send the groups, file permissions, number of streams, and encapsulated
     * current working directory.  For each open file, send the
     * streamID and encapsulated stream contents.
     *
     *	        data			size
     *	 	----			----
     * 		# groups		int
     *	        groups			(# groups) * int
     *		permissions		int
     *		# files			int
     *		per-file flags		(# files) * char
     *		encapsulated files	(# files) * (FsMigInfo + int)
     *		cwd			FsMigInfo + int + strlen(cwdPrefix) + 1
     */
    size = (4 + fsPtr->numGroupIDs) * sizeof(int) +
	    streamFlagsLen + numStreams * (sizeof(FsMigInfo) + sizeof(int)) +
	    sizeof(FsMigInfo) + cwdLength;
    if (size != infoPtr->size) {
	panic("Fs_EncapState: size of encapsulated state changed.\n");
	return(FAILURE);
    }

    /*
     * Send groups, filePermissions, numStreams, the cwd, and each file.
     */
    
    numGroups = fsPtr->numGroupIDs;
    Byte_FillBuffer(ptr, unsigned int, numGroups);
    if (numGroups > 0) {
	bcopy((Address) fsPtr->groupIDs, ptr, numGroups * sizeof(int));
	ptr += numGroups * sizeof(int);
    }
    Byte_FillBuffer(ptr, unsigned int, fsPtr->filePermissions);
    Byte_FillBuffer(ptr, int, numStreams);
    if (numStreams > 0) {
	bcopy((Address) fsPtr->streamFlags, ptr, numStreams * sizeof(char));
	ptr += streamFlagsLen;
    }
    
    Byte_FillBuffer(ptr, int, prefixPtr->prefixLength);
    strncpy(ptr, prefixPtr->prefix, prefixPtr->prefixLength);
    ptr[prefixPtr->prefixLength] = '\0';
    ptr += cwdLength;

    status = Fsio_EncapStream(fsPtr->cwdPtr, ptr);
    if (status != SUCCESS) {
	printf(
		  "Fs_EncapFileState: Error %x from Fsio_EncapStream on cwd.\n",
		  status);
	return(status);
    }
    fsPtr->cwdPtr = (Fs_Stream *) NIL;
    ptr += sizeof(FsMigInfo);

    for (i = 0; i < fsPtr->numStreams; i++) {
	streamPtr = fsPtr->streamList[i];
	if (streamPtr != (Fs_Stream *) NIL) {
	    Byte_FillBuffer(ptr, int, i);
	    status = Fsio_EncapStream(streamPtr, ptr);
	    if (status != SUCCESS) {
		printf(
			  "Fs_EncapFileState: Error %x from Fsio_EncapStream.\n",
			  status);
		return(status);
	    }
	    fsPtr->streamList[i] = (Fs_Stream *) NIL;
	} else {
	    Byte_FillBuffer(ptr, int, NIL);
	    bzero(ptr, sizeof(FsMigInfo));
	}	
	ptr += sizeof(FsMigInfo);
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_DeencapFileState --
 *
 *	Get the file system state of a process from another node.  The
 *	buffer contains group information, permissions, the encapsulated
 *	current working directory, and encapsulated streams.
 *
 * Results:
 *	If Fsio_DeencapStream returns an error, that error is returned.
 *	Otherwise, SUCCESS is returned.  
 *
 * Side effects:
 *	"Local" Fs_Streams are created and allocated to the foreign process.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_DeencapFileState(procPtr, infoPtr, buffer)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Proc_EncapInfo *infoPtr;		  /* information about the buffer */
    Address buffer;			  /* buffer containing data */
{
    register Fs_ProcessState *fsPtr;
    int i;
    int index;
    int numGroups;
    int numStreams;
    ReturnStatus status;
    char *cwdName;
    int cwdLength;
    Fs_Stream *prefixStreamPtr;

    /*
     * Set up an fsPtr for the process.  Initialize some fields so that
     * at any point we can bail out on error by calling Fs_CloseState.  Some
     * fields are initialized from the information from the other host.
     */
    procPtr->fsPtr = fsPtr = mnew(Fs_ProcessState);
    fsPtr->cwdPtr = (Fs_Stream *) NIL;

    /*
     * Get group and permissions information.
     */
    Byte_EmptyBuffer(buffer, unsigned int, numGroups);
    fsPtr->numGroupIDs = numGroups;
    if (numGroups > 0) {
	fsPtr->groupIDs = (int *)malloc(numGroups * sizeof(int));
	bcopy(buffer, (Address) fsPtr->groupIDs, numGroups * sizeof(int));
	buffer += numGroups * sizeof(int);
    } else {
	fsPtr->groupIDs = (int *)NIL;
    }
    Byte_EmptyBuffer(buffer, unsigned int, fsPtr->filePermissions);

    /*
     * Get numStreams, flags, and the encapsulated cwd.  Allocate memory
     * for the streams and flags arrays if non-empty.  The array of
     * streamFlags may be an odd number of bytes, so we skip past the
     * byte of padding if it exists (using the Byte_AlignAddr macro).
     */

    Byte_EmptyBuffer(buffer, int, numStreams);
    fsPtr->numStreams = numStreams;
    if (numStreams > 0) {
	fsPtr->streamList = (Fs_Stream **)
		malloc(numStreams * sizeof(Fs_Stream *));
	fsPtr->streamFlags = (char *)malloc(numStreams * sizeof(char));
	bcopy(buffer, (Address) fsPtr->streamFlags, numStreams * sizeof(char));
	buffer += Byte_AlignAddr(numStreams * sizeof(char));
	for (i = 0; i < fsPtr->numStreams; i++) {
	    fsPtr->streamList[i] = (Fs_Stream *) NIL;
	}
    } else {
	fsPtr->streamList = (Fs_Stream **)NIL;
	fsPtr->streamFlags = (char *)NIL;
    }
    /*
     * Get the name of the current working directory and make sure it's
     * an installed prefix.
     */
    Byte_EmptyBuffer(buffer, int, cwdLength);
    cwdName = buffer;
    buffer += Byte_AlignAddr(cwdLength + 1);
    status = Fs_Open(cwdName, FS_READ | FS_FOLLOW, FS_FILE, 0,
		     &prefixStreamPtr);
    if (status != SUCCESS) {
	if (fsio_MigDebug) {
	    panic("Unable to open prefix '%s' for migrated process.\n",
		   cwdName);
	} else if (proc_MigDebugLevel > 1) {
	    printf("%s unable to open prefix '%s' for migrated process.\n",
		   "Warning: Fs_DeencapFileState:", cwdName);
	}
	goto failure;
    } else {
	(void) Fs_Close(prefixStreamPtr);
    }

    status = Fsio_DeencapStream(buffer, &fsPtr->cwdPtr);
    if (status != SUCCESS) {
	if (fsio_MigDebug) {
	    panic("GetFileState: Fsio_DeencapStream returned %x for cwd.\n",
		  status);
	} else if (proc_MigDebugLevel > 1) {
	    printf("%s Fsio_DeencapStream returned %x for cwd.\n",
		  "Warning: Fs_DeencapFileState:", status);
	}
	fsPtr->cwdPtr = (Fs_Stream *) NIL;
        goto failure;
    }
    buffer += sizeof(FsMigInfo);

    

    /*
     * Get the other streams.
     */
    for (i = 0; i < fsPtr->numStreams; i++) {
	Byte_EmptyBuffer(buffer, int, index);
	if ((status == SUCCESS) && (index != NIL)) {
	    status = Fsio_DeencapStream(buffer, &fsPtr->streamList[index]);
	    if (status != SUCCESS) {
		    printf(
      "Fs_DeencapFileState: Fsio_DeencapStream for file id %d returned %x.\n",
			      index, status);
		    fsPtr->streamList[index] = (Fs_Stream *) NIL;
		    goto failure;
	    }
	}
	buffer += sizeof(FsMigInfo);
    }
    return(SUCCESS);
    
failure:
    Fs_CloseState(procPtr);
    return(status);
    
}
