/* 
 * fsSetFd.c --
 *
 *	These are the routines that fill in the file descriptors of the files
 *	in the file system.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.2 89/01/07 04:12:18 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "option.h"
#include "disk.h"
#include <stdio.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>

extern Boolean rootFree;
extern Boolean rootFile;
extern Boolean noRoot;


/*
 *----------------------------------------------------------------------
 *
 * SetRootFileDescriptor --
 *
 *	File 2.
 *	Set up the file descriptor for the root directory. The root
 *	directory uses block 0.
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetRootFileDescriptor(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;

    if (noRoot) {
	bzero((Address) fileDescPtr, sizeof(Fsdm_FileDescriptor));
	return;
    }
    fileDescPtr->flags = rootFree ? FSDM_FD_FREE :  FSDM_FD_ALLOC;
    fileDescPtr->fileType = rootFile ? FS_FILE : FS_DIRECTORY;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FS_BLOCK_SIZE-1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 3;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    /*
     * Place the data in the first filesystem block.
     */
    fileDescPtr->direct[0] = 0;
    for (index = 1; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 4;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetBadBlockFileDescriptor --
 *
 *	File 1.
 *	Set up the file descriptor for the bad block file.
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetBadBlockFileDescriptor(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;

    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0000;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = -1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 0;		/* Intentionally unreferenced */
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    /*
     * Place the data in the first filesystem block.
     */
    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 0;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetLostFoundFileDescriptor --
 *
 *	File 3.
 *	Set up the file descriptor for the lost and found directory.
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetLostFoundFileDescriptor(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;

    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_DIRECTORY;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSDM_NUM_LOST_FOUND_BLOCKS * FS_BLOCK_SIZE - 1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 2;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    for (index = 0; index < FSDM_NUM_LOST_FOUND_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FS_FRAGMENTS_PER_BLOCK * (index + 1);
    }
    for (; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 8;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetEmptyFileDescriptor --
 *
 *	File 4
 *	Set up a file descriptor for an empty file.
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetEmptyFileDescriptor(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;

    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0666;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = -1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 0;
    fileDescPtr->version = 1;
}
/*
 *----------------------------------------------------------------------
 *
 * SetTooBigFD --
 *
 *	File 6
 *	The file size is bigger than the number of blocks.
 *	The block count is too large.
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetTooBigFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;
    int numBlocks = 1;
    int start;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = numBlocks * FS_BLOCK_SIZE + 10;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 2;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    start = (FSDM_NUM_LOST_FOUND_BLOCKS + 1);

    for (index = 0; index < numBlocks ; index++) {
	fileDescPtr->direct[index] = (start + index) * FS_FRAGMENTS_PER_BLOCK;
    }
    for (; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 8;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetTooSmallFD --
 *
 * 	File 7
 *	The file size is smaller than the number of blocks.
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
SetTooSmallFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;
    int num1KBlocks = 2;
    int start;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = (num1KBlocks - 1) * 
			    (FS_BLOCK_SIZE / FS_FRAGMENTS_PER_BLOCK) - 10;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    start = 16;

    for (index = 0; index < num1KBlocks ; index++) {
	fileDescPtr->direct[index] = (start + index);
    }
    for (; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 2;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetHoleFileFD --
 *
 *	File 8
 *	Nothing wrong here - just a file with a hole.
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
SetHoleFileFD(headerPtr, partFID, fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
    Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    Time time;
    int index;
    char block[FS_BLOCK_SIZE];
    int *indBlock = (int *) block;
    int status;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE +
			    FSDM_INDICES_PER_BLOCK * FS_BLOCK_SIZE +
			    1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;


    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    blockNum = 20 + FS_FRAGMENTS_PER_BLOCK * headerPtr->dataOffset ;
    fileDescPtr->indirect[1] = blockNum;
    indBlock[0] = blockNum + 4;
    for (index = 1; index < FSDM_INDICES_PER_BLOCK;index++) {
	indBlock[index] = FSDM_NIL_INDEX;
    }
    status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset + 5,
			     1, block);
    if (status != SUCCESS) {
	fprintf(stderr, "SetHoleFileFD : write failed.\n");
	return;
    }
    blockNum += 4;
    indBlock[0] = blockNum + 4 - FS_FRAGMENTS_PER_BLOCK * headerPtr->dataOffset;
    for (index = 1; index < FSDM_INDICES_PER_BLOCK;index++) {
	indBlock[index] = FSDM_NIL_INDEX;
    }
    status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset + 6, 
			     1, block);
    if (status != SUCCESS) {
	fprintf(stderr, "SetHoleFileFD : write failed.\n");
	return;
    }
    fileDescPtr->numKbytes = 4;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetHoleDirFD --
 *
 *	File 9
 *	Directory with a hole, which isn't allowed.
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
SetHoleDirFD(headerPtr, partFID, fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
    Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    Time time;
    int index;
    char block[FS_BLOCK_SIZE];
    int *indBlock = (int *) block;
    int status;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_DIRECTORY;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = 2 * FS_BLOCK_SIZE -1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;


    fileDescPtr->direct[0] = 32;
    fileDescPtr->direct[1] = FSDM_NIL_INDEX;
    fileDescPtr->direct[2] = 36;
    for (index = 3; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[1] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 1;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetBadEntryFileFD --
 *
 *	File 10.
 *	The first indirect block contains an illegal index.
 *	First direct block number is bad.
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
SetBadEntryFileFD(headerPtr, partFID, fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
    Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    Time time;
    int index;
    char block[FS_BLOCK_SIZE];
    int *indBlock = (int *) block;
    int status;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE +
			    FSDM_INDICES_PER_BLOCK * FS_BLOCK_SIZE + 1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    fileDescPtr->direct[0] = 10000000;
    for (index = 1; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    blockNum = 44 +  FS_FRAGMENTS_PER_BLOCK * headerPtr->dataOffset;
    fileDescPtr->indirect[0] = blockNum;
    bzero(block,FS_BLOCK_SIZE);

    indBlock[0] = 10000000;
    for (index = 1; index < FSDM_INDICES_PER_BLOCK;index++) {
	indBlock[index] = FSDM_NIL_INDEX;
    }
    status = Disk_BlockWrite(partFID, headerPtr,headerPtr->dataOffset + 11,  
			     1, block);
    if (status != SUCCESS) {
	fprintf(stderr, "SetHoleFileFD : write failed.\n");
	return;
    }
    blockNum = 48 +  FS_FRAGMENTS_PER_BLOCK * headerPtr->dataOffset;
    fileDescPtr->indirect[1] = blockNum;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    indBlock[0] = blockNum + 4;
    for (index = 1; index < FSDM_INDICES_PER_BLOCK;index++) {
	indBlock[index] = FSDM_NIL_INDEX;
    }
    status = Disk_BlockWrite(partFID, headerPtr,headerPtr->dataOffset + 12,
			     1, block);
    if (status != SUCCESS) {
	fprintf(stderr, "SetHoleFileFD : write failed.\n");
	return;
    }
    indBlock[0] = 56;
    for (index = 1; index < FSDM_INDICES_PER_BLOCK;index++) {
	indBlock[index] = FSDM_NIL_INDEX;
    }
    status = Disk_BlockWrite(partFID, headerPtr,headerPtr->dataOffset + 13 ,
			     1, block);
    if (status != SUCCESS) {
	fprintf(stderr, "SetHoleFileFD : write failed.\n");
	return;
    }
    fileDescPtr->numKbytes = 1;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetFragFileFD --
 *
 *	File 11
 *	The first block is a fragment in the middle of a file.
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
SetFragFileFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FS_BLOCK_SIZE + FS_FRAGMENT_SIZE -1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    fileDescPtr->direct[0] = 61;
    fileDescPtr->direct[1] = 62;
    for (index = 2; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 2;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetCopyFragFileFD --
 *
 *	File 12
 *	The first direct block shares block 60  with file 11. 
 *	The second direct block shares fragment 62  with file 11. 
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
SetCopyFragFileFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FS_BLOCK_SIZE + FS_FRAGMENT_SIZE -1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    fileDescPtr->direct[0] = 60;
    fileDescPtr->direct[1] = 62;
    for (index = 2; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[1] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 4;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetCopyBlockFileFD --
 *
 * 	File 13
 *	Shares a block with file 10.
 *	First indirect block number is bad.
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
SetCopyBlockFileFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;

    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte =FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE +
			    FSDM_INDICES_PER_BLOCK * FS_BLOCK_SIZE + 1 ;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    fileDescPtr->direct[0] = 56;
    fileDescPtr->direct[1] = 0;
    for (index = 2; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = 8;
    fileDescPtr->indirect[1] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 8;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetCopyIndBlockFileFD --
 *
 * 	File 14
 *	Shares an indirect block with file 10
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
SetCopyIndBlockFileFD(headerPtr, partFID, fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
    Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    Time time;
    int index;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE +
			    FSDM_INDICES_PER_BLOCK * FS_BLOCK_SIZE + 1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;


    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    blockNum = 48 +  FS_FRAGMENTS_PER_BLOCK * headerPtr->dataOffset;
    fileDescPtr->indirect[1] = blockNum;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 4;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetCopyBogusIndBlockFileFD --
 *
 * 	File 15
 *	Shares an indirect block with file 10, but the block contains bogus
 *		indices.
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
SetCopyBogusIndBlockFileFD(headerPtr, partFID, fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
    Fsdm_DomainHeader *headerPtr;
    int partFID;
{
    Time time;
    int index;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSDM_NUM_DIRECT_BLOCKS * FS_BLOCK_SIZE +
			    FSDM_INDICES_PER_BLOCK * FS_BLOCK_SIZE + 1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;


    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    blockNum = 56 +  FS_FRAGMENTS_PER_BLOCK * headerPtr->dataOffset;
    fileDescPtr->indirect[1] = blockNum;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 1;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetDirFD --
 *
 *	File 17	
 *	Set up a file descriptor for an empty directory
 *
 * Results:
 *	Fill in the file descriptor.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
SetDirFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;

    fileDescPtr->flags =   FSDM_FD_ALLOC;
    fileDescPtr->fileType =  FS_DIRECTORY;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSLCL_DIR_BLOCK_SIZE-1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 0;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    /*
     * Place the data in the first filesystem block.
     */
    fileDescPtr->direct[0] = 64;
    for (index = 1; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 1;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetOutputFD --
 *
 * 	File 18
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
SetOutputFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_FILE;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = 2 * FS_BLOCK_SIZE - 1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 1;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    fileDescPtr->direct[0] = 68;
    fileDescPtr->direct[1] = 72;

    for (index = 2; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[1] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 8;
    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetBadDotDotFD --
 *
 * 	File 21
 *	The ".." entry points to an unallocated file descriptor (22).
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
SetBadDotDotFD(fileDescPtr)
    register Fsdm_FileDescriptor *fileDescPtr;
{
    Time time;
    int index;
    int blockNum;


    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = FS_DIRECTORY;
    fileDescPtr->permissions = 0755;
    fileDescPtr->uid = 0;
    fileDescPtr->gid = 0;
    fileDescPtr->lastByte = FSLCL_DIR_BLOCK_SIZE-1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->numLinks = 0;
    /*
     * Can't know device information because that depends on
     * the way the system is configured.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    Sys_GetTimeOfDay(&time, NULL, NULL);
    fileDescPtr->createTime = time.seconds;
    fileDescPtr->accessTime = 0;
    fileDescPtr->descModifyTime = time.seconds;
    fileDescPtr->dataModifyTime = time.seconds;

    fileDescPtr->direct[0] = 76;

    for (index = 1; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->indirect[0] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[1] = FSDM_NIL_INDEX;
    fileDescPtr->indirect[2] = FSDM_NIL_INDEX;
    fileDescPtr->numKbytes = 1;
    fileDescPtr->version = 1;
}


