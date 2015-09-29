/* fs.c -
 *
 *	General filesystem support.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/scsiDiskBoot/RCS/fs.c,v 1.9 89/06/16 08:29:57 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fsBoot.h"
#include "machMon.h"

/*
 * For non-block aligned reads.
 */
char	readBuffer[FS_BLOCK_SIZE];

/*
 * For lookup
 */
static char	component[FS_MAX_NAME_LENGTH];

/*
 * Forward declarations.
 */
void FsGetFileDesc();
void FsInitFileHandle();

/*
 * ----------------------------------------------------------------------------
 *
 * Fs_Open --
 *
 *	Open a file.  This does a simple lookup (based on the kernel's
 *	FsLocalLookup) and creates a handle for the file.
 *
 * Results:
 *	SUCCESS or a return code from various sub-operations.
 *
 * Side effects:
 *	Calls malloc
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Fs_Open(fileName, useFlags, permissions, handlePtrPtr)
    char *fileName;
    int useFlags;
    int permissions;
    FsLocalFileIOHandle 	**handlePtrPtr;
{
    register ReturnStatus status;
    FsLocalFileIOHandle *curHandlePtr;
    register char *curCharPtr;
    register char *componentPtr;
    register int index;

    curCharPtr = fileName;
    while(*curCharPtr == '/') {
	curCharPtr++;
    }
    curHandlePtr = fsRootHandlePtr;

    while (*curCharPtr != '\0') {
	if (curHandlePtr->descPtr->fileType != FS_DIRECTORY) {
	    return(FS_NOT_DIRECTORY);
	}
        /*
         * Get the next component.
         */
        index = 0;
	componentPtr = component;
        while (*curCharPtr != '/' && *curCharPtr != '\0') {
            *componentPtr++ = *curCharPtr++;
        }
        *componentPtr = '\0';
#ifndef NO_PRINTF
	Mach_MonPrintf(" %s ", component);
#endif
        /*
         * Skip intermediate and trailing slashes so that *curCharPtr
         * is Null when 'component' has the last component of the name.
         */
        while (*curCharPtr == '/') {
            curCharPtr++;
        }

	status = FsFindComponent(fsDomainPtr, curHandlePtr, component,
					      &curHandlePtr);

	if (status != SUCCESS) {
#ifndef NO_PRINTF
	    Mach_MonPrintf("<%x>\n", status);
#endif
	    return(status);
	}
    }
    *handlePtrPtr = curHandlePtr;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fs_Read --
 *
 *	Read from a file given its handle.
 *
 * Results:
 *	A return status from the read.
 *
 * Side effects:
 *	buffer is loaded with the data read in.
 *	*readCountPtr is updated to reflect the number of bytes read.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Fs_Read(handlePtr, offset, numBytes, buffer, readCountPtr)
    register FsLocalFileIOHandle 	*handlePtr;
    int			offset;
    int			numBytes;
    register Address	buffer;
    int			*readCountPtr;
{
    int				firstBlock;
    int				lastBlock;
    int				lastByte;
    BlockIndexInfo		indexInfo;
    register	int		readSize;
    register	int		blockAddr;
    register	int		blockOffset;
    register	int		bufferIndex;
    register 	ReturnStatus	status;
    register	int		size;

    firstBlock = offset / FS_BLOCK_SIZE; 
    lastByte = offset + numBytes - 1;
    if (lastByte > handlePtr->descPtr->lastByte) {
	lastByte = handlePtr->descPtr->lastByte;
    }
    lastBlock = lastByte / FS_BLOCK_SIZE;

    (void)FsGetFirstIndex(handlePtr, firstBlock, &indexInfo);

    bufferIndex = 0;
    blockOffset = offset & FS_BLOCK_OFFSET_MASK;
#ifdef SCSI0_BOOT 
    Mach_MonPrintf(" read %d at %d into %x\n", numBytes, offset, buffer);
#endif 

    while (indexInfo.blockNum <= lastBlock) {
	if (indexInfo.blockNum < lastBlock) {
	    size = FS_BLOCK_SIZE - blockOffset;
	    readSize = FS_BLOCK_SIZE;
	} else {
	    size = (lastByte & FS_BLOCK_OFFSET_MASK) + 1 - blockOffset;
	    readSize = size;
	}
	blockAddr = *indexInfo.blockAddrPtr + 
		    fsDomainPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK;
	if (blockOffset != 0 || size != FS_BLOCK_SIZE) { 
	    status = FsDeviceBlockIO(FS_READ, &fsDevice, blockAddr,
			   (readSize - 1) / FS_FRAGMENT_SIZE + 1, readBuffer);
	    if (status != SUCCESS) {
		goto readError;
	    }
	    bcopy(&(readBuffer[blockOffset]), &(buffer[bufferIndex]), size);
	} else {
	    status = FsDeviceBlockIO(FS_READ, &fsDevice, blockAddr,
			FS_FRAGMENTS_PER_BLOCK, &(buffer[bufferIndex]));
	    if (status != SUCCESS) {
		goto readError;
	    }
	}
	bufferIndex += size;
	blockOffset = 0;
	FsGetNextIndex(handlePtr, &indexInfo);
    }

readError:

    *readCountPtr = bufferIndex;

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFindComponent --
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsFindComponent(domainPtr, curHandlePtr, component, newHandlePtrPtr)
    FsDomain *domainPtr;
    FsLocalFileIOHandle *curHandlePtr;
    char *component;
    FsLocalFileIOHandle **newHandlePtrPtr;
{
    register ReturnStatus status;
    register int dirOffset;		/* Offset within the directory */
    register int blockOffset;		/* Offset within a directory block */
    register FsDirEntry *dirEntryPtr;	/* Reference to directory entry */
    int length;				/* Length variable for read call */
    register FsLocalFileIOHandle *handlePtr;

    dirOffset = 0;
    do {
	length = FS_DIR_BLOCK_SIZE;
	status = Fs_Read(curHandlePtr, dirOffset, length, readBuffer, &length);
	if (status != SUCCESS) {
	    return(status);
	}
	if (length == 0) {
	    return(FS_FILE_NOT_FOUND);
	}
	dirEntryPtr = (FsDirEntry *)readBuffer;
	blockOffset = 0;
	while (blockOffset < FS_DIR_BLOCK_SIZE) {
	    dirEntryPtr = (FsDirEntry *)((int)readBuffer + blockOffset);
	    if (dirEntryPtr->fileNumber != 0) {
		/*
		 * A valid directory record.
		 */
		Mach_MonPrintf("Found %s\n", dirEntryPtr->fileName);
		if (strcmp(component, dirEntryPtr->fileName) == 0) {
		    handlePtr = (FsLocalFileIOHandle *)malloc(sizeof(FsLocalFileIOHandle));
		    FsInitFileHandle(domainPtr, dirEntryPtr->fileNumber,
				    handlePtr);
		    *newHandlePtrPtr = handlePtr;
		    return(SUCCESS);
		}
	    }
	    blockOffset += dirEntryPtr->recordLength;
	}
	dirOffset += FS_DIR_BLOCK_SIZE;
    } while(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * FsInitFileHandle --
 *
 *	Initialize a file handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in the file handle that our caller has already allocated.
 *
 *----------------------------------------------------------------------
 */
void
FsInitFileHandle(domainPtr, fileNumber, handlePtr)
    FsDomain *domainPtr;
    int fileNumber;
    register FsLocalFileIOHandle *handlePtr;
{
    register FsFileDescriptor *descPtr;

    bzero((Address)handlePtr, sizeof(FsLocalFileIOHandle));
    handlePtr->hdr.fileID.minor = fileNumber;
    descPtr = (FsFileDescriptor *)malloc(sizeof(FsFileDescriptor));
    FsGetFileDesc(domainPtr, fileNumber, descPtr);
    handlePtr->descPtr = descPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FsGetFileDesc --
 *
 *	Read in a file descriptor from the disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fills in the file descriptor that our caller has already allocated.
 *
 *----------------------------------------------------------------------
 */
void
FsGetFileDesc(domainPtr, fileNumber, descPtr)
    FsDomain *domainPtr;
    register int fileNumber;
    register FsFileDescriptor *descPtr;
{
    register FsDomainHeader *headerPtr;
    register int 	    blockNum;
    register int 	    offset;

    headerPtr = domainPtr->headerPtr;
    blockNum = headerPtr->fileDescOffset + fileNumber / FS_FILE_DESC_PER_BLOCK;
    offset = (fileNumber & (FS_FILE_DESC_PER_BLOCK - 1)) *
		FS_MAX_FILE_DESC_SIZE;

    (void)FsDeviceBlockIO(FS_READ, &fsDevice, 
		       blockNum * FS_FRAGMENTS_PER_BLOCK,
		       FS_FRAGMENTS_PER_BLOCK, readBuffer);
    bcopy( readBuffer + offset, descPtr, sizeof(FsFileDescriptor));
#ifndef NO_PRINTF
    if (descPtr->magic != FS_FD_MAGIC) {
	Mach_MonPrintf("desc %d bad <%x>\n", fileNumber, descPtr->magic);
    }
#endif
}
