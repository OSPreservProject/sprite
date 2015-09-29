/*
 * fsmake.h --
 *
 *	Declarations for the fsmake program.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/admin/fsmake/RCS/fsmake.h,v 1.2 90/10/10 16:00:36 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _FSMAKE
#define _FSMAKE
#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <bstring.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <option.h>
#include <disk.h>
#include <host.h>
#include <dev/scsi.h>
#include <sys/stat.h>
#include <sys/time.h>

#if defined(sprite) || defined(sun)
#define S_GFDIR     S_IFDIR
#define S_GFMT      S_IFMT
#define S_GFLNK     S_IFLNK
#define S_GFREG      S_IFREG
#endif

/*
 * This suffix on a file indicates that it contains information for
 * building a Sprite device.
 */

#define DEVICE_SUFFIX ".SPRITE_DEV"

/*
 * The following structures are used to send a raw scsi command to the
 * disk.  These really don't belong here.  Someday there should be a 
 * seperate header file for defining the format of scsi commands.
 */

typedef struct {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		
    unsigned char lun		:3;
    unsigned char pad1		:4;
    unsigned char reladr	:1;
    unsigned char addr3;
    unsigned char addr2;
    unsigned char addr1;
    unsigned char addr0;
    unsigned char pad2;
    unsigned char pad3;
    unsigned char pad4		:7;
    unsigned char pmi		:1;
    unsigned char control;
#else
    unsigned char command;		
    unsigned char reladr	:1;
    unsigned char pad1		:4;
    unsigned char lun		:3;
    unsigned char addr3;
    unsigned char addr2;
    unsigned char addr1;
    unsigned char addr0;
    unsigned char pad2;
    unsigned char pad3;
    unsigned char pmi		:1;
    unsigned char pad4		:7;
    unsigned char control;
#endif
} ReadCapacityCommand;


/*
 * This structure is passed to Fs_IOControl to do an IOC_SCSI_COMMAND
 * ioctl.
 */
typedef struct  {
    Dev_ScsiCommand	hdr;
    ReadCapacityCommand cmd;
} ScsiCmd;

/*
 * The result of an IOC_SCSI_COMMAND.
 */
typedef struct {
    Dev_ScsiStatus	hdr;
    union {
	struct {
	    unsigned char	addr3;
	    unsigned char	addr2;
	    unsigned char	addr1;
	    unsigned char	addr0;
	    unsigned char	size3;
	    unsigned char	size2;
	    unsigned char	size1;
	    unsigned char	size0;
	} result;
	unsigned char	data[3*1024];
    } info;
} CmdStatus;

/*
 * Structure used to peruse Sprite directories.
 */
typedef struct DirIndexInfo {
    Fsdm_FileDescriptor *fdPtr;	     	     /* The file descriptor being
					      * read. */
    int		 blockNum;		     /* Block that is being read, 
					      * written, or allocated. */
    int		 blockAddr;		     /* Address of directory block
					      * to read. */
    int		 dirOffset;		     /* Offset of the directory entry 
					      * that we are currently examining 
					      * in the directory. */
    char	 dirBlock[FS_BLOCK_SIZE];    /* Where directory data is 
					      * stored. */
} DirIndexInfo;

/*
 * Macro to get a pointer into the bit map for a particular block.
 */
#define BlockToCylinder(headerPtr, blockNum) \
    (blockNum) / (headerPtr)->geometry.blocksPerCylinder

#define GetBitmapPtr(headerPtr, bitmapPtr, blockNum) \
  &((bitmapPtr)[BlockToCylinder(headerPtr, blockNum) * \
  bytesPerCylinder + (blockNum) % (headerPtr)->geometry.blocksPerCylinder / 2])

/*
 * Macros to convert physical block numbers to virtual block numbers. All direct
 * blocks are virtual, indirect blocks are physical.
 */
#define VirtToPhys(blockNum) \
    ((blockNum) + (headerPtr)->dataOffset * FS_FRAGMENTS_PER_BLOCK)

#define PhysToVirt(headerPtr,blockNum) \
    ((blockNum) - (headerPtr)->dataOffset * FS_FRAGMENTS_PER_BLOCK)

/*
 * Macro to mark the file descriptor bit map.
 */
#define MarkFDBitmap(num,bitmapPtr) \
    (bitmapPtr)[(num) >> 3] |= (1 << (7 -((num)  & 7)))

#define Max(a,b) ((a) > (b)) ? (a) : (b)

extern Boolean printOnly;

#undef _ARGS_
#ifdef __STDC__
#define	_ARGS_(x)	x
#else 
#define	_ARGS_(x)	()
#endif 

extern ReturnStatus ConfirmDiskSize _ARGS_ ((int fid,
    Disk_Label *labelPtr, int *sizes)); /* Size of partitions. */
extern ReturnStatus Reconfig _ARGS_((int fid, Boolean disktab,
    char *disktabName, char *diskType, Disk_NativeLabelType labelType,
    int scsiDisk, Disk_Label **labelPtrPtr));
extern ReturnStatus Repartition _ARGS_((int fid, Boolean disktab,
    char *disktabName, char *diskType, Disk_NativeLabelType labelType,
    int partition, int *sizes, Disk_Label **labelPtrPtr));
extern ReturnStatus InventConfig _ARGS_((int fid, Disk_Label *labelPtr));
extern ReturnStatus ConfirmDiskSize _ARGS_((int fid,
    Disk_Label *labelPtr, int *sizes));

#endif /* _FSMAKE */

