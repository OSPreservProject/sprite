/* 
 * fsmake.c --
 *
 *	Format a domain to be an empty domain.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/fsmake/RCS/fsmake.c,v 1.18 90/11/01 23:24:48 jhh Exp $ SPRITE (Berkeley)";
#endif

#include "fsmake.h"
#include <assert.h>

/*
 * Constants settable via the command line.
 */
static int kbytesToFileDesc = 4;/* The ratio of kbytes to
				 * the number of file descriptors */
Boolean printOnly = TRUE;       /* Stop after computing the domain header
				 * and just print it out. No disk writes */
static Boolean overlapBlocks = FALSE;
                                /* Allow filesystem blocks to overlap track
				 * boundaries.  Some disk systems can't deal. */
static Boolean scsiDisk = TRUE;	/* If TRUE then simpler geometry is computed
				 * that knows that SCSI controllers don't
				 * think in terms of cylinders, heads, and
				 * sectors.  Instead, they think in terms of
				 * logical sector numbers, so we punt on finding
				 * rotationally optimal block positions. */
static int bootSectors = -1;	/* Number of boot sectors. */
static int hostID = 0;		/* Host id to write into domain header. 
				 * A host id of 0 means use local host id. */
static Boolean repartition = FALSE;
                                /* If TRUE then the partition map is changed. */
static Boolean reconfig = FALSE;	/* If TRUE then the disk configuration
				 * is changed. */
static Boolean partdisktab = FALSE;
                                /* If TRUE then read the partition map from
				 * the disktab file. */
static Boolean configdisktab = FALSE;
                                /* If TRUE then read the config information
				 * from the disktab file, otherwise get the
				 * disk size information directly from the
				 * disk and compute the "best" configuration..*/
static char *disktabName = "/etc/disktab"; /* Name of disktab file. */
static char *sizeString = NULL;	/* Size of partitions. */
static char *diskType = NULL;	/* Type of disk (e.g.rz55). */
static char *labelTypeName = NULL; /* Type of label (e.g. sun). */
static char *dirName = NULL;	/* Name of directory that contains files to
				 * copy to the disk. */
extern char *hostFileName;      /* Name of file to get host info from.  By
                                   default it is /etc/spritehosts.  It is 
				   defined in Host_Start.c. */

/*
 * The following are used to go from a command line like
 * makeFilesystem -D rsd0 -P b
 * to /dev/rsd0a 	- for the partition that has the disk label
 * and to /dev/rsd0b	- for the partition to format.
 */
static char *deviceName;	/* Set to "rsd0" or "rxy1", etc. */
static char *partName;		/* Set to "a", "b", "c" ... "g" */
static char *rawDeviceName;	/* Set to "raw_rsd00", etc */
static char defaultFirstPartName[] = "a";
static char *firstPartName = defaultFirstPartName;
static char defaultDevDirectory[] = "/dev";
static char *devDirectory = defaultDevDirectory;

static char *hostIDString = NULL;

static Option optionArray[] = {
    {OPT_STRING, "dev", (Address)&deviceName,
	"Required: Name of device, eg \"rsd0\" or \"rxy1\""},
    {OPT_STRING, "part", (Address)&partName,
	"Required: Partition ID: (a, b, c, d, e, f, g)"},
    {OPT_STRING, "rawdev", (Address)&rawDeviceName,
	"Required: Name of raw device, eg \"raw_rsd00\""},
    {OPT_TRUE, "scsi", (Address)&scsiDisk,
	"Compute geometry for SCSI type disk (TRUE)"},
    {OPT_FALSE, "noscsi", (Address)&scsiDisk,
	"Compute geometry for non-SCSI disk (FALSE)"},
    {OPT_TRUE, "overlap", (Address)&overlapBlocks,
	"Overlap filesystem blocks across track boundaries (FALSE)"},
    {OPT_INT, "ratio", (Address)&kbytesToFileDesc,
	"Ratio of Kbytes to file descriptors (4)"},
    {OPT_TRUE, "test", (Address)&printOnly,
	"Test: print results, don't write disk (TRUE)"},
    {OPT_FALSE, "write", (Address)&printOnly,
	"Write the disk (FALSE)"},
    {OPT_STRING, "devDir", (Address)&devDirectory,
	"Name of device directory (\"/dev\")"},
    {OPT_STRING, "initialPart", (Address)&firstPartName,
	"Name of initial partition (\"a\")"},
    {OPT_INT, "boot", (Address)&bootSectors,
	"Number of boot sectors in root partition"},
    {OPT_STRING, "host", (Address)&hostIDString,
	"Host id or name of machine domain is attached to."},
    {OPT_TRUE, "repartition", (Address)&repartition,
	"Change the partitioning of the disk.  USE ONLY ON EMPTY DISKS"},
    {OPT_TRUE, "reconfig", (Address) &reconfig,
	"Change the disk configuration info in label. USE ONLY ON EMPTY DISKS"},
    {OPT_TRUE, "configdisktab", (Address)&configdisktab,
	"Reconfigure according to info in disktab file"},
    {OPT_TRUE, "partdisktab", (Address)&partdisktab,
	"Repartition according to info in disktab file"},
    {OPT_STRING, "disktabName", (Address)&disktabName,
	"Name of the disktab file"},
    {OPT_STRING, "sizes", (Address)&sizeString,
	"Size of partitions (as a percentage), eg \"a:25,g:75\""},
    {OPT_STRING, "disktype", (Address)&diskType,
	"Type of disk. Used to look up information in disktab file"},
    {OPT_STRING, "labeltype", (Address)&labelTypeName,
	"Type of native disk label (sun or dec)"},
    {OPT_STRING, "dir", (Address)&dirName,
	"Directory to copy files from"},
    {OPT_STRING, "spritehosts", (Address)&hostFileName,
        "File to get host information from"},
};
#define numOptions (sizeof(optionArray) / sizeof(Option))

/*
 * Time of day when this program runs.
 */

static struct timeval curTime;

static int		freeFDNum;	/* The currently free file descriptor.*/
static int		freeBlockNum;	/* The currently free data block. */
static unsigned char	*fdBitmapPtr;	/* Pointer to the file descriptor
					 * bitmap. */
static unsigned char	*cylBitmapPtr;	/* Pointer to the cylinder bit map. */
static int	        bytesPerCylinder;/* The number of bytes in
					  * the bitmap for a cylinder.*/
static Ofs_DomainHeader *headerPtr;	/* The domain header. */
static Ofs_SummaryInfo *summaryPtr;	/* The summary info. */

static char *myName;
/*
 * Forward Declarations.
 */

static void Usage _ARGS_((void));
static ReturnStatus MakeFilesystem _ARGS_((int firstPartFID, int partFID,
    int partition, int spriteID, Disk_Label **labelPtrPtr));
static void CopyTree _ARGS_((int partFID, char *dirName, int dirFDNum,
    Fsdm_FileDescriptor *dirFDPtr, int parentFDNum,
    Boolean createDir, char *path));
static ReturnStatus SetDomainHeader _ARGS_((Disk_Label *labelPtr,
    Ofs_DomainHeader *headerPtr, int spriteID, int partition));
static void SetSCSIDiskGeometry _ARGS_((Disk_Label *labelPtr,
    Ofs_Geometry *geoPtr));
static void SetDiskGeometry _ARGS_((Disk_Label *labelPtr,
    Ofs_Geometry *geoPtr));
static void SetDomainParts _ARGS_((Disk_Label *labelPtr, int partition,
    Ofs_DomainHeader *headerPtr));
static void SetSummaryInfo _ARGS_((Ofs_DomainHeader *headerPtr,
    Ofs_SummaryInfo *summaryPtr));
static ReturnStatus WriteAllFileDescs _ARGS_((int partFID,
    Ofs_DomainHeader *headerPtr));
static char *MakeFileDescBitmap _ARGS_((Ofs_DomainHeader *headerPtr));
static ReturnStatus WriteBitmap _ARGS_((int partFID,
    Ofs_DomainHeader *headerPtr));
static ReturnStatus WriteRootDirectory _ARGS_((int partFID,
    Ofs_DomainHeader *headerPtr, Fsdm_FileDescriptor *fdPtr));
static ReturnStatus WriteLostFoundDirectory _ARGS_((int partFID,
    Ofs_DomainHeader *headerPtr));
static void InitDesc _ARGS_((Fsdm_FileDescriptor *fileDescPtr, int fileType,
    int numBytes, int devServer, int devType, int devUnit,
    int uid, int gid, int permissions, long time));
static ReturnStatus AddToDirectory _ARGS_((int fid,
    Ofs_DomainHeader *headerPtr, Boolean writeDisk, DirIndexInfo *dirIndexPtr,
    Fslcl_DirEntry **dirEntryPtrPtr, int fileNumber, char *fileName));
static ReturnStatus OpenDir _ARGS_((int fid, Ofs_DomainHeader *headerPtr,
    Fsdm_FileDescriptor *fdPtr, DirIndexInfo *indexInfoPtr,
    Fslcl_DirEntry **dirEntryPtrPtr));
static ReturnStatus NextDirEntry _ARGS_((int fid, Ofs_DomainHeader *headerPtr,
    Boolean writeDisk, DirIndexInfo *indexInfoPtr,
    Fslcl_DirEntry **dirEntryPtrPtr));
static ReturnStatus CloseDir _ARGS_((int fid, Ofs_DomainHeader *headerPtr,
    Boolean writeDisk, DirIndexInfo *indexInfoPtr));
static ReturnStatus ReadFileDesc _ARGS_((int fid, Ofs_DomainHeader *headerPtr,
    int fdNum, Fsdm_FileDescriptor *fdPtr));
static ReturnStatus WriteFileDesc _ARGS_((int fid, Ofs_DomainHeader *headerPtr,
    int fdNum, Fsdm_FileDescriptor *fdPtr));
static void CreateDir _ARGS_((Address blocks, int numBlocks,
    int dot, int dotDot));
static void MarkDataBitmap _ARGS_((Ofs_DomainHeader *headerPtr,
    unsigned char *cylBitmapPtr, int blockNum, int numFrags));
static unsigned char *ReadFileDescBitmap _ARGS_((int fid,
    Ofs_DomainHeader *headerPtr));
static unsigned char *ReadBitmap _ARGS_((int fid,
    Ofs_DomainHeader *headerPtr));
static void WriteFileDescBitmap _ARGS_((int fid, Ofs_DomainHeader *headerPtr,
    unsigned char *bitmap));

extern char *getwd();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Create the required file names from the command line
 *	arguments.  Then open the first partition on the disk
 *	because it contains the disk label, and open the partition
 *	that is to be formatted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls MakeFilesystem
 *
 *----------------------------------------------------------------------
 */
void
main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status;	/* status of system calls */
    int firstPartFID;		/* File ID for first parition on the disk */
    int partFID;		/* File ID for partiton to format */
    int rawFID = -1;
    char firstPartitionName[64];
    char partitionName[64];
    int partition;		/* Index of partition derived from the
				 * partition name */
    int spriteID;		/* Host ID of the machine with the disks */
    Fs_Attributes attrs;
    Disk_NativeLabelType	labelType;	
    Disk_Label			*labelPtr = NULL;
    int	sizes[8];
    int i;

    myName = argv[0];
    argc = Opt_Parse(argc, argv,optionArray, numOptions, 0);
    if (argc > 1) {
	Usage();
    }
    status = SUCCESS;
    if (deviceName == (char *)0) {
	fprintf(stderr,"Specify device name with -dev option\n");
	status = FAILURE;
    }
    if (partName == (char *)0) {
	fprintf(stderr,"Specify partition with -part option\n");
	status = FAILURE;
    }
    if (bootSectors > FSDM_MAX_BOOT_SECTORS) {
	fprintf(stderr,"Maximum number of boot sectors is %d.\n", 
	    FSDM_MAX_BOOT_SECTORS);
	status = FAILURE;
    }
    if ((bootSectors != -1) && (bootSectors % FSDM_BOOT_SECTOR_INC != 0)) {
	fprintf(stderr, "Number of boot sectors must be a multiple of %d.\n",
	    FSDM_BOOT_SECTOR_INC);
	status = FAILURE;
    }
    for (i = 0; i < 8; i++) {
	sizes[i] = 0;
    }
    if (sizeString != NULL) {
	char *cPtr;
	int part;
	int size;
	int n;
	cPtr = sizeString;
	while (*cPtr != '\0') {
	    part = (int) (*cPtr - 'a');
	    if (part < 0 || part > 7) {
		fprintf(stderr, 
	    "Argument to -sizes parameter must start with partition.\n");
		Usage();
	    }
	    cPtr++;
	    if (*cPtr != ':') {
		fprintf(stderr, 
	    "Argument to -sizes parameter is missing a ':'.\n");
		Usage();
	    }
	    cPtr++;
	    n = sscanf(cPtr,"%d",&size);
	    if (n != 1) {
		fprintf(stderr, 
    "Argument to -sizes parameter must have an integer following the ':'\n");
		Usage();
	    }
	    sizes[part] = size;
	    cPtr = strchr(cPtr, ',');
	    if (cPtr == NULL) {
		break;
	    }
	    cPtr++;
	}
    }
    if (hostIDString != (char *) NULL) {
	int		scanned;
	int		temp;
	Host_Entry	*entryPtr;

	scanned = sscanf(hostIDString, " %d", &temp);
	if (scanned == 1) {
	    entryPtr = Host_ByID(temp);
	    if (entryPtr == NULL) {
		fprintf(stderr, 
		    "WARNING: Host %d is not in /etc/spritehosts.\n", temp);
	    }
	    hostID = temp;
	} else {
	    entryPtr = Host_ByName(hostIDString);
	    if (entryPtr == NULL) {
		fprintf(stderr, 
		    "Host %s is not in /etc/spritehosts.\n", temp);
		status = FAILURE;
	    }
	    hostID = entryPtr->id;
	}
    }
    labelType = DISK_NO_LABEL;
    if (labelTypeName != NULL) {
	if (!strcasecmp(labelTypeName, "sun")) {
	    labelType = DISK_SUN_LABEL;
	} else if (!strcasecmp(labelTypeName, "dec")) {
	    labelType = DISK_DEC_LABEL;
	} else {
	    fprintf(stderr, 
		"Argument to -labeltype must be \"sun\" or \"dec\"\n");
	    status = FAILURE;
	}
    }
    if (status != SUCCESS) {
	exit(status);
    }
    /*
     * Gen up the name of the first partition on the disk,
     * and the name of the parition that needs to be formatted.
     */
    sprintf(firstPartitionName, "%s/%s%c", devDirectory, deviceName,
	*firstPartName);
    sprintf(partitionName, "%s/%s%c", devDirectory, deviceName,
	*partName);
    if (rawDeviceName != NULL) {
	char buffer[64];
	sprintf(buffer, "%s/%s", devDirectory, rawDeviceName);
	if (!printOnly) {
	    rawFID = open(buffer, O_RDWR);
	} else {
	    rawFID = open(buffer, O_RDONLY);
	}
	if (rawFID < 0) {
	    fprintf(stderr, "Can't open raw device %s: ", buffer);
	    perror((char *) NULL);
	    exit(FAILURE);
	}
    }
    if (reconfig) {
	if (rawFID < 0) {
	    fprintf(stderr, 
	"You must specify a raw device with the -reconfig flag\n");
	    exit(FAILURE);
	}
	status = Reconfig(rawFID, configdisktab, disktabName, diskType,
	    labelType, scsiDisk, &labelPtr);
	if (status != SUCCESS) {
	    fprintf(stderr, "Reconfiguration of disk failed.\n");
	    exit(1);
	}
    }
    if (repartition || reconfig) {
	if (rawFID < 0) {
	    fprintf(stderr, 
	"You must specify a raw device with the -repartition flag\n");
	    exit(FAILURE);
	}
	status = Repartition(rawFID, partdisktab, disktabName, diskType,
	    labelType, partition,
	    sizes, &labelPtr);
	if (status != SUCCESS) {
	    fprintf(stderr, "Repartition of disk failed.\n");
	    exit(1);
	}
	printf("New disk label:\n");
	bzero(labelPtr->asciiLabel,sizeof(labelPtr->asciiLabel));
	strcpy(labelPtr->asciiLabel,diskType);
	Disk_PrintLabel(labelPtr);
	status = ConfirmDiskSize(rawFID, labelPtr, sizes);
	if (status != SUCCESS) {
	    exit(status);
	}
	if (!printOnly) {
	    status = Disk_WriteLabel(rawFID, labelPtr);
	    if (status != SUCCESS) {
		fprintf(stderr, "Can't write label to raw device\n");
		exit(status);
	    }
	}
    }
    if (!printOnly) {
	firstPartFID = open(firstPartitionName, O_RDWR);
    } else {
	firstPartFID = open(firstPartitionName, O_RDONLY);
    }
    if (firstPartFID < 0 ) {
	fprintf(stderr, "Can't open first partition %s: ", firstPartitionName);
	perror((char *) NULL);
	exit(FAILURE);
    }
    if (!printOnly) {
	partFID = open(partitionName, O_RDWR);
    } else {
	partFID = open(partitionName, O_RDONLY);
    }
    if (partFID < 0) {
	perror("Can't open partition to format");
	exit(FAILURE);
    }

    partition = partName[0] - 'a';
    if (partition < 0 || partition > 7) {
	fprintf(stderr,
	       "Can't determine partition index from the partition name\n");
	exit(FAILURE);
    }
#ifdef sprite  /* This stuff will not work if run under Unix. */
    if (hostID == 0) {

	/*
	 * Determine where the disk is located so we can set the
	 * spriteID in the header correctly.
	 */
	Fs_GetAttributesID(firstPartFID, &attrs);
	if (attrs.devServerID == FS_LOCALHOST_ID) {
	    Proc_GetHostIDs((int *) NULL, &spriteID);
	    printf("Making filesystem for local host, ID = 0x%x\n", spriteID);
	} else {
	    spriteID = attrs.devServerID;
	    printf("Making filesystem for remote host 0x%x\n", spriteID);
	}
    } else
#endif
    {
	spriteID = hostID;
	printf("Making filesystem for host, ID = 0x%x\n", spriteID);
    }
    gettimeofday(&curTime, (struct timezone *) NULL);
    printf("MakeFilesystem based on 4K filesystem blocks\n");
    status = MakeFilesystem(firstPartFID, partFID, partition, spriteID, 
	&labelPtr);
    if ((status == SUCCESS) && (dirName != NULL)) {
	Fsdm_FileDescriptor	rootDesc;
	printf("Copying %s to new filesystem.\n", dirName);
	fdBitmapPtr = ReadFileDescBitmap(partFID, headerPtr);
	cylBitmapPtr = ReadBitmap(partFID, headerPtr);
	ReadFileDesc(partFID, headerPtr, FSDM_ROOT_FILE_NUMBER, &rootDesc);
	CopyTree(partFID, dirName, FSDM_ROOT_FILE_NUMBER, &rootDesc,
	    FSDM_ROOT_FILE_NUMBER, FALSE, "/");
	WriteFileDesc(partFID, headerPtr, FSDM_ROOT_FILE_NUMBER, &rootDesc);
	if (!printOnly) {
	    status = Disk_WriteSummaryInfo(partFID, labelPtr, summaryPtr);
	    if (status != 0) {
		perror("Summary sector write failed (2)");
		exit(status);
	    }
	    WriteFileDescBitmap(partFID, headerPtr, fdBitmapPtr);
	    WriteBitmap(partFID, headerPtr);
	}
    }
    fflush(stderr);
    fflush(stdout);
    (void)close(firstPartFID);
    (void)close(partFID);
    exit(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Usage --
 *
 *	Prints out the correct command syntax and exits..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The program exits.
 *
 *----------------------------------------------------------------------
 */

static void
Usage()
{
    Opt_PrintUsage(myName, optionArray, Opt_Number(optionArray));
    exit(1);
}

/*
 *----------------------------------------------------------------------
 *
 * MakeFilesystem --
 *
 *	Format a disk partition, or domain, to be an empty filesystem.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Write all over the disk partition.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
MakeFilesystem(firstPartFID, partFID, partition, spriteID, labelPtrPtr)
    int firstPartFID;	/* Handle on the first partition of the disk */
    int partFID;	/* Handle on the partition of the disk to format */
    int partition;	/* Index of parition that is to be formatted */
    int spriteID;	/* Host ID of the machine with the disks, this
			 * gets written on the disk and used at boot time */
    Disk_Label	**labelPtrPtr; /* Ptr to ptr to disk label. */
{
    ReturnStatus 	status;
    Ofs_DomainHeader	oldDomainHeader;
    Ofs_SummaryInfo	oldSummaryInfo;
    Fsdm_FileDescriptor rootFD;
    Fsdm_FileDescriptor *rootFDPtr = &rootFD;
    Disk_Label	*labelPtr = *labelPtrPtr;

    if (labelPtr == NULL) {
	labelPtr = Disk_ReadLabel(firstPartFID);
	if (labelPtr == NULL) {
	    fprintf(stderr, "Unable to read disk label.\n");
	    return FAILURE;
	}
    }
    *labelPtrPtr = labelPtr;
    summaryPtr = Disk_ReadSummaryInfo(partFID, labelPtr);
    if (summaryPtr != NULL) {
	if (!printOnly) {
	    char answer[10];

	    printf("\nYou are about to overwrite the \"%s\" filesystem.\n", 
		summaryPtr->domainPrefix);
	    printf("Do you really want to do this?[y/n] ");
	    if (scanf("%10s",answer) != 1) {
		exit(SUCCESS);
	    }
	    if ((*answer != 'y') && (*answer != 'Y')) {
		return SUCCESS;
	    }
	}
    }
    bzero((char *) &oldDomainHeader, sizeof(oldDomainHeader));
    bzero((char *) &oldSummaryInfo, sizeof(oldSummaryInfo));
    if (!printOnly) {
	status = Disk_WriteDomainHeader(partFID, labelPtr, &oldDomainHeader);
	if (status != SUCCESS) {
	    printf("Clear of old domain header failed\n");
	}
	status = Disk_WriteSummaryInfo(partFID, labelPtr, &oldSummaryInfo);
	if (status != SUCCESS) {
	    printf("Clear of old summary info failed\n");
	}
    }
    /*
     * The user has specified the number of boot sectors.
     */
    if (bootSectors >= 0) {
	labelPtr->summarySector = bootSectors + 1;
	labelPtr->domainSector = bootSectors + 2;
	labelPtr->numBootSectors = bootSectors;
    } else {
	Disk_Label *newLabelPtr;
	newLabelPtr = Disk_NewLabel(labelPtr->labelType);
	labelPtr->summarySector = newLabelPtr->summarySector;
	labelPtr->domainSector = newLabelPtr->domainSector;
	labelPtr->numBootSectors = newLabelPtr->numBootSectors;
	free((char *) newLabelPtr);
    }
    headerPtr = (Ofs_DomainHeader *) malloc(sizeof(Ofs_DomainHeader));
    status = SetDomainHeader(labelPtr, headerPtr, spriteID, partition);
    if (status != SUCCESS) {
	return FAILURE;
    }
    Disk_PrintDomainHeader(headerPtr);

    if (!printOnly) {
	status = Disk_WriteLabel(partFID, labelPtr);
	if (status != SUCCESS) {
	    perror("Disk label write failed"); 
	    return(status);
	}
	status = Disk_WriteDomainHeader(partFID, labelPtr, headerPtr);
	if (status != SUCCESS) {
	    perror("DomainHeader write failed");
	    return(status);
	}
    }
    summaryPtr = (Ofs_SummaryInfo *) malloc(DEV_BYTES_PER_SECTOR);
    SetSummaryInfo(headerPtr, summaryPtr);
    if (!printOnly) {
	status = Disk_WriteSummaryInfo(partFID, labelPtr, summaryPtr);
	if (status != SUCCESS) {
	    perror("Summary sector write failed");
	    return(status);
	}
    }
    status = WriteAllFileDescs(partFID, headerPtr);
    if (status != SUCCESS) {
	perror("WriteAllFileDescs failed");
	return(status);
    }
    status = WriteBitmap(partFID, headerPtr);
    if (status != SUCCESS) {
	perror("WriteBitmap failed");
	return(status);
    }
    ReadFileDesc(partFID, headerPtr, FSDM_ROOT_FILE_NUMBER, rootFDPtr);
    status = WriteRootDirectory(partFID, headerPtr, rootFDPtr);
    WriteFileDesc(partFID, headerPtr, FSDM_ROOT_FILE_NUMBER, rootFDPtr);
    if (status != SUCCESS) {
	perror("WriteRootDirectory failed");
	return(status);
    }
    status = WriteLostFoundDirectory(partFID, headerPtr);
    if (status != SUCCESS) {
	perror("WriteLostFoundDirectory failed");
	return(status);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * CopyTree --
 *
 *	Copy the tree of files in the given directory
 *	the disk table.
 *
 * Results:
 *	A pointer to a disk info struct.
 *
 * Side effects:
 *	Disk info struct malloc'd and initialized.  The disk header will be
 *	written if is successfully set up.
 *
 *----------------------------------------------------------------------
 */
static void
CopyTree(partFID, dirName, dirFDNum, dirFDPtr, parentFDNum, createDir, path)
    int			partFID;	/* Handle on raw disk. */
    char		*dirName;	/* Name of directory to copy. */
    int			dirFDNum;	/* File number of directory. */
    Fsdm_FileDescriptor	*dirFDPtr;	/* File descriptor of directory. */
    int			parentFDNum;	/* File number of parent. */
    Boolean		createDir;	/* Should create the directory. */
    char		*path;
{
    DIR			*unixDirPtr;
    struct direct	*unixDirEntPtr;
    DirIndexInfo	indexInfo;
    Fslcl_DirEntry	*spriteDirEntPtr;
    char		fileName[FS_MAX_NAME_LENGTH + 1];
    int			newFDNum;
    Fsdm_FileDescriptor	newFD;
    Fsdm_FileDescriptor	*newFDPtr;
    struct	stat	statBuf;
    int			followLinks;
    char		pathName[1024];
    char		fileBlock[FS_BLOCK_SIZE];
    char		indirectBlock[FS_BLOCK_SIZE];
    int			*indIndexPtr = (int *)indirectBlock;
    char		*suffix;
    Boolean		makeDevice;
    int			devType;
    int			devUnit;
    int			devServer;
    ReturnStatus	status;


    /*
     * Get our absolute path name so we can get back if we follow a
     * symbolic link.
     */
    (void) getwd(pathName);

    if (chdir(dirName) < 0) {
	perror(dirName);
	exit(1);
    }

    /*
     * Get a pointer to the UNIX directory.
     */
    unixDirPtr = opendir(".");
    if (unixDirPtr == NULL) {
	fprintf(stderr, "Can't open directory %s\n", dirName);
	exit(1);
    }
    /*
     * Open the Sprite directory.
     */
    status = OpenDir(partFID, headerPtr, dirFDPtr, &indexInfo,&spriteDirEntPtr);
    if (status != SUCCESS) {
	if (chdir(pathName) < 0) {
	    perror(pathName);
	    exit(1);
	}
    }
    /*
     * See if there is a "follow.links" file in this directory.  If so
     * we are supposed to follow symbolic links rather than just copying
     * the links.
     */
    if (stat("follow.links", &statBuf) < 0) {
	followLinks = 0;
    } else {
	printf("Following links ...\n");
	followLinks = 1;
    }
    if (createDir) {
	CreateDir(indexInfo.dirBlock, 1, dirFDNum, parentFDNum);
    }

    while ((unixDirEntPtr = readdir(unixDirPtr)) != NULL) {
	if (unixDirEntPtr->d_namlen == 1 && 
	    strncmp(unixDirEntPtr->d_name, ".", 1) == 0) {
	    continue;
	}
	if (unixDirEntPtr->d_namlen == 2 && 
	    strncmp(unixDirEntPtr->d_name, "..", 2) == 0) {
	    continue;
	}
	strncpy(fileName, unixDirEntPtr->d_name, unixDirEntPtr->d_namlen);
	fileName[unixDirEntPtr->d_namlen] = 0;
	if (followLinks) {
	    if (stat(fileName, &statBuf) < 0) {
		perror(fileName);
		exit(1);
	    }
	} else {
	    if (lstat(fileName, &statBuf) < 0) {
		perror(fileName);
		exit(1);
	    }
	}
	makeDevice = FALSE;
	suffix = strstr(fileName, DEVICE_SUFFIX);
	if ((suffix != NULL) && (!strcmp(suffix, DEVICE_SUFFIX)) &&
	    ((statBuf.st_mode & S_GFMT) == S_GFREG)) {
	    FILE	*fp;
	    char	buffer[128];
	    int		ok = 0;

	    fp = fopen(fileName, "r");
	    if (fp == NULL) {
		fprintf(stderr, "WARNING: Can't open %s%s: ", pathName,
		    fileName);
		perror((char *) NULL);
		continue;
	    }
	    while (fgets(buffer, 128, fp) != NULL) {
		char	tmp[128];
		int	n;

		n = sscanf(buffer, " %128s", tmp);
		if (n < 1 || *tmp == '#') {
		    continue;
		}
		if (sscanf(buffer, " %d %d %d", &devServer, &devType, &devUnit)
		    != 3) {
		    fprintf(stderr, 
		    "WARNING: Device file %s%s has bad format. Skipping.\n",
			pathName, fileName);
		    break;
		}
		ok = 1;
		break;
	    }
	    fclose(fp);
	    if (!ok) {
		continue;
	    }
	    makeDevice = TRUE;
	    *suffix = '\0';
	}
	newFDNum = freeFDNum;
	freeFDNum++;
	MarkFDBitmap(newFDNum, fdBitmapPtr);
	summaryPtr->numFreeFileDesc--;
	ReadFileDesc(partFID, headerPtr, newFDNum, &newFD);
	newFDPtr = &newFD;

	status = AddToDirectory(partFID, headerPtr, !printOnly, &indexInfo, 
	    &spriteDirEntPtr, newFDNum, fileName);
	if (status != SUCCESS) {
	    fprintf(stderr, "%s is full\n", dirName);
	    break;
	}
	if (makeDevice) {
	    InitDesc(newFDPtr, FS_DEVICE, 0, devServer, devType, devUnit, 0, 0,
		(int) statBuf.st_mode & 07777, curTime.tv_sec);
	    printf("Device: %s%s (S %d, T %d, U %d)\n", path, fileName,
		devServer, devType, devUnit);
#ifdef sprite
	} else if ((statBuf.st_mode & S_GFMT) == S_IFPDEV) {
	    printf("Skipping pseudo-device %s%s\n", path, fileName);
#endif
	} else if ((statBuf.st_mode & S_GFMT) == S_GFREG ||
#ifdef sprite
		   ((statBuf.st_mode & S_GFMT) == S_IFRLNK)  || 
#endif
		   (statBuf.st_mode & S_GFMT) == S_GFLNK) {
	    int	fd = -1;
	    int	blockNum;
	    int	toRead;
	    int	len;

	    blockNum = 0;
	    if ((statBuf.st_mode & S_GFMT) == S_GFREG) {
		printf("File: %s%s\n", path, fileName);
		InitDesc(newFDPtr, FS_FILE, (int) statBuf.st_size, -1, -1, -1,
			 0, 0, (int) statBuf.st_mode & 07777, statBuf.st_mtime);
		/*
		 * Copy the file over.
		 */
		fd = open(fileName, 0);
		if (fd < 0) {
		    fprintf(stderr, "WARNING: Can't open %s%s: ", pathName,
			fileName);
		    perror((char *) NULL);
		    freeFDNum--;
		    summaryPtr->numFreeFileDesc++;
		    continue;
		}
		len = read(fd, fileBlock, FS_BLOCK_SIZE);
		if (len < 0) {
		    fprintf(stderr, "WARNING: Can't read %s%s: ", pathName,
			fileName);
		    perror((char *) NULL);
		    freeFDNum--;
		    summaryPtr->numFreeFileDesc++;
		    continue;
		}
		toRead = statBuf.st_size;
	    } else {
		len = readlink(fileName, fileBlock, FS_BLOCK_SIZE);
		if (len < 0) {
		    fprintf(stderr, "WARNING: Can't readlink %s%s: ", pathName,
			fileName);
		    perror((char *) NULL);
		    freeFDNum--;
		    summaryPtr->numFreeFileDesc++;
		    continue;
		}
		fileBlock[len] = '\0';
		InitDesc(newFDPtr, FS_SYMBOLIC_LINK, len + 1, -1, -1, -1, 
			 0, 0, 0777, statBuf.st_mtime);
#ifdef sprite
		if ((statBuf.st_mode & S_GFMT) == S_IFRLNK) { 
		    printf("Remote link: %s%s\n", path, fileName);
		} else {
		    printf("Symbolic link: %s%s -> %s\n", 
			    path, fileName, fileBlock);
		}
#else
		printf("Symbolic link: %s%s -> %s\n", 
			path, fileName, fileBlock);
#endif
		toRead = len + 1;
	    }

	    while (len > 0) {
		if (blockNum == FSDM_NUM_DIRECT_BLOCKS) {
		    int	i;
		    int	*intPtr;
		    /*
		     * Must allocate an indirect block.
		     */
		    newFDPtr->indirect[0] =
			VirtToPhys(freeBlockNum * FS_FRAGMENTS_PER_BLOCK);
		    MarkDataBitmap(headerPtr, cylBitmapPtr, freeBlockNum,
				   FS_FRAGMENTS_PER_BLOCK);
		    freeBlockNum++;
		    summaryPtr->numFreeKbytes -= FS_FRAGMENTS_PER_BLOCK;
		    for (i = 0, intPtr = (int *)indirectBlock; 
			 i < FS_BLOCK_SIZE / sizeof(int); 
			 i++, intPtr++) {
			 *intPtr = FSDM_NIL_INDEX;
		    }
		}
		if (blockNum >= FSDM_NUM_DIRECT_BLOCKS) {
		    indIndexPtr[blockNum - FSDM_NUM_DIRECT_BLOCKS] = 
					freeBlockNum * FS_FRAGMENTS_PER_BLOCK;
		    MarkDataBitmap(headerPtr, cylBitmapPtr, freeBlockNum,
				   FS_FRAGMENTS_PER_BLOCK);
		    summaryPtr->numFreeKbytes -= FS_FRAGMENTS_PER_BLOCK;
		} else {
		    newFDPtr->direct[blockNum] = 
					freeBlockNum * FS_FRAGMENTS_PER_BLOCK;
		    if (toRead > FS_BLOCK_SIZE) {
			MarkDataBitmap(headerPtr, cylBitmapPtr, freeBlockNum,
				       FS_FRAGMENTS_PER_BLOCK);
			summaryPtr->numFreeKbytes -= FS_FRAGMENTS_PER_BLOCK;
		    } else {
			MarkDataBitmap(headerPtr, cylBitmapPtr, freeBlockNum,
				       (toRead - 1) / FS_FRAGMENT_SIZE + 1);
			summaryPtr->numFreeKbytes -= 
					(toRead - 1) / FS_FRAGMENT_SIZE + 1;
		    }
		}
		/*
		 * Write the block out to disk.
		 */
		if (Disk_BlockWrite(partFID, headerPtr, 
				    headerPtr->dataOffset + freeBlockNum,
				    1, (Address)fileBlock) != 0) {
		    fprintf(stderr, "Couldn't write file block\n");
		    exit(1);
		}
		blockNum++;
		freeBlockNum++;
		if ((statBuf.st_mode & S_GFMT) == S_GFLNK) {
		    break;
		}
		toRead -= len;
		len = read(fd, fileBlock, FS_BLOCK_SIZE);
		if (len < 0) {
		    perror(fileName);
		    exit(1);
		}
	    }
	    if (newFDPtr->indirect[0] != FSDM_NIL_INDEX) {
		if (Disk_BlockWrite(partFID, headerPtr,
			    newFDPtr->indirect[0] / FS_FRAGMENTS_PER_BLOCK,
			    1, (Address)indirectBlock) != 0) {
		    fprintf(stderr, "Couldn't write indirect block\n");
		    exit(1);
		}
	    }
	    close(fd);
	} else if (statBuf.st_mode & S_GFDIR) {
	    char	newPath[FS_MAX_NAME_LENGTH];

	    /*
	     * Increment the current directories link count because once
	     * the child gets created it will point to the parent.
	     */
	    dirFDPtr->numLinks++;
	    /*
	     * Allocate the currently free file descriptor to this directory.
	     */
	    InitDesc(newFDPtr, FS_DIRECTORY, FS_BLOCK_SIZE, -1, -1, -1, 
		     0, 0, (int) statBuf.st_mode & 07777, statBuf.st_mtime);
	    /*
	     * Give the directory one full block.  The directory will
	     * be initialized by CopyTree when we call it recursively.
	     */
	    newFDPtr->direct[0] = freeBlockNum * FS_FRAGMENTS_PER_BLOCK;
	    MarkDataBitmap(headerPtr, cylBitmapPtr, freeBlockNum,
			   FS_FRAGMENTS_PER_BLOCK);
	    freeBlockNum++;
	    sprintf(newPath, "%s%s/", path, fileName);
	    printf("Directory: %s\n", newPath);
	    CopyTree(partFID, fileName, newFDNum, newFDPtr, dirFDNum, TRUE, 
		newPath);
	} else {
	    fprintf(stderr, "WARNING: %s%s is not a file or directory\n",
		pathName, fileName);
	    freeFDNum--;
	    summaryPtr->numFreeFileDesc++;
	    continue;
	}
	WriteFileDesc(partFID, headerPtr, newFDNum, newFDPtr);

    }

    CloseDir(partFID, headerPtr, !printOnly, &indexInfo);

    if (chdir(pathName) < 0) {
	perror(pathName);
	exit(1);
    }
    closedir(unixDirPtr);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * SetDomainHeader --
 *
 *	Compute the domain header based on the partition size and
 *	other basic disk parameters.
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	Fill in the domain header.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
SetDomainHeader(labelPtr, headerPtr, spriteID, partition)
    Disk_Label		*labelPtr;	/* The disk label. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header to fill in */
    int 		spriteID;	/* Host ID of machine with the disks */
    int 		partition;	/* Index of partition to format */
{
    register Ofs_Geometry *geoPtr;/* The layout information for the disk */

    headerPtr->magic = OFS_DOMAIN_MAGIC;
    headerPtr->firstCylinder = labelPtr->partitions[partition].firstCylinder;
    headerPtr->numCylinders = labelPtr->partitions[partition].numCylinders;
    if (headerPtr->numCylinders <= 0) {
	fprintf(stderr, "Invalid partition size: %d cylinders.\n", 
	    headerPtr->numCylinders);
	return FAILURE;
    }
    /*
     * The device.serverID from the disk is used during boot to discover
     * the host"s spriteID if reverse arp couldn't find a host ID.  The
     * unit number of disk indicates what partition of the disk this
     * domain header applies to.  For example, both the "a" and "c" partitions
     * typically start at sector zero, but only one is valid.  During boot
     * time the unit number is used to decide which partition should be
     * attached.
     */
    headerPtr->device.serverID = spriteID;
    headerPtr->device.type = -1;
    headerPtr->device.unit = partition;
    headerPtr->device.data = (ClientData)-1;

    geoPtr = &headerPtr->geometry;
    if (scsiDisk) {
	SetSCSIDiskGeometry(labelPtr, geoPtr);
    } else {
	SetDiskGeometry(labelPtr, geoPtr);
    }

    SetDomainParts(labelPtr, partition, headerPtr);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * SetSCSIDiskGeometry --
 *
 *	This computes the rotational set arrangment depending on the
 *	disk geometry.  No fancy block skewing is done.  The cylinders
 *	are divided into rotational sets that minimize the amount of
 *	wasted space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fill in the geometry struct.
 *
 *----------------------------------------------------------------------
 */
static void
SetSCSIDiskGeometry(labelPtr, geoPtr)
    register Disk_Label		*labelPtr;	/* The disk label. */
    register Ofs_Geometry 	*geoPtr;	/* Fancy geometry information */
{
    register int index;		/* Array index */
    int blocksPerCyl;		/* The number of blocks in a cylinder */

    geoPtr->numHeads = labelPtr->numHeads;
    geoPtr->sectorsPerTrack = labelPtr->numSectors;

    blocksPerCyl = geoPtr->sectorsPerTrack * geoPtr->numHeads /
		DISK_SECTORS_PER_BLOCK;

    printf("Disk has %d tracks/cyl, %d sectors/track\n",
	    geoPtr->numHeads, geoPtr->sectorsPerTrack);
    printf("%d 4K Blocks fit on a cylinder with %d 512 byte sectors wasted\n",
	    blocksPerCyl, geoPtr->sectorsPerTrack * geoPtr->numHeads - 
	    blocksPerCyl * DISK_SECTORS_PER_BLOCK);
    geoPtr->rotSetsPerCyl = OFS_SCSI_MAPPING;
    geoPtr->blocksPerRotSet = 0;
    geoPtr->blocksPerCylinder = blocksPerCyl;
    geoPtr->tracksPerRotSet = 0;
    /*
     * Don't use rotational sorting anyway.
     */
    for (index = 0 ; index < OFS_MAX_ROT_POSITIONS ; index++) {
	geoPtr->sortedOffsets[index] = -1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetDiskGeometry --
 *
 *	This computes the rotational set arrangment depending on the
 *	disk geometry.  The basic rules for this are that filesystem blocks
 *	are skewed on successive tracks, and that the skewing pattern
 *	repeats in either 2 or 4 tracks.  This is specific to the fact that
 *	filesystem blocks are 4Kbytes.  This means that one disk track
 *	contains N/4 filesystem blocks and that one sector per track
 *	is wasted if there are an odd number of sectors per track.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fill in the geometry struct.
 *
 *----------------------------------------------------------------------
 */
static void
SetDiskGeometry(labelPtr, geoPtr)
    register Disk_Label		*labelPtr;	/* The disk label. */
    register Ofs_Geometry 	*geoPtr;	/* Fancy geometry information */
{
    register int index;		/* Array index */
    int numBlocks;		/* The number of blocks in a rotational set */
    int tracksPerSet = 0;	/* Total number of tracks in a rotational set */
    int numTracks;		/* The number of tracks in the set so far */
    int extraSectors;		/* The number of leftover sectors in a track */
    int offset;			/* The sector offset within a track */
    int startingOffset;		/* The offset of the first block in a track */
    int offsetIncrement = 0;	/* The skew of the starting offset on each
				 * successive track of the rotational set */
    Boolean overlap = FALSE;	/* TRUE if filesystem blocks overlap tracks */

    geoPtr->numHeads = labelPtr->numHeads;
    geoPtr->sectorsPerTrack = labelPtr->numSectors;

    /*
     * Figure out some basic parameters of the rotational set.  The number
     * of tracks in the set is either 2 or 4.  If 2, then the blocks on
     * successive tracks are skewed by 1/2 a filesystem block.  If 4,
     * blocks are skewed by 1/4 block.  A 4 track rotational set is best
     * becasue there are more rotational positions.  If, however, it
     * causes 2 or 3 wasted tracks at the end, or if blocks naturally
     * overlap by 1/2 block, then only 2 tracks per rotational set are
     * used.
     */
    switch(geoPtr->numHeads % 4) {
	case 0:
	case 1: {
	    extraSectors = geoPtr->sectorsPerTrack % DISK_SECTORS_PER_BLOCK;
	    if (extraSectors < DISK_SECTORS_PER_BLOCK/4) {
		/*
		 * Not enough extra sectors to overlap blocks onto the
		 * next track.  The blocks will fit evenly on a track,
		 * but the blocks on the following tracks will be skewed.
		 */
		tracksPerSet = 4;
		overlap = FALSE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK/4;
	    } else if (extraSectors < DISK_SECTORS_PER_BLOCK/2) {
		/*
		 * Enough to overlap the first 1/4 block onto the next track.
		 */
		tracksPerSet = 4;
		overlap = TRUE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK * 3/4;
	    } else if (extraSectors < DISK_SECTORS_PER_BLOCK * 3/4) {
		/*
		 * Enough to overlap 1/2 block.
		 */
		tracksPerSet = 2;
		overlap = TRUE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK/2;
	    } else {
		/*
		 * Enough to overlap 3/4 block.
		 */
		tracksPerSet = 4;
		overlap = TRUE;
		offsetIncrement = DISK_SECTORS_PER_BLOCK/4;
	    }
	    break;
	}
	case 2:
	case 3: {
	    /*
	     * Instead of wasting 2 or 3 tracks to have a 4 track rotational
	     * set, the rotational set is only 2 tracks long.  Also see if
	     * the blocks naturally overlap by 1/2 block.
	     */
	    tracksPerSet = 2;
	    offsetIncrement = DISK_SECTORS_PER_BLOCK/2;
	    if ((geoPtr->sectorsPerTrack % DISK_SECTORS_PER_BLOCK) <
		      DISK_SECTORS_PER_BLOCK/2) {
		overlap = FALSE;
	    } else {
		overlap = TRUE;
	    }
	}
    }
    if (!overlapBlocks) {
	overlap = FALSE;
	offsetIncrement = 0;
    }
    printf("overlap %s, offsetIncrement %d\n", (overlap ? "TRUE" : "FALSE"),
		      offsetIncrement);
    /*
     * Determine rotational position of the blocks in the rotational set.
     */
    extraSectors = geoPtr->sectorsPerTrack;
    startingOffset = 0;
    offset = startingOffset;
    for (numBlocks = 0, numTracks = 0 ; ; ) {
	if (extraSectors >= DISK_SECTORS_PER_BLOCK) {
	    /*
	     * Ok to fit in another filesystem block on this track.
	     */
	    geoPtr->blockOffset[numBlocks] = offset;
	    numBlocks++;	
	    offset += DISK_SECTORS_PER_BLOCK;
	    extraSectors -= DISK_SECTORS_PER_BLOCK;
	} else {
	    /*
	     * The current block has to take up room on the next track.
	     */
	    numTracks++;
	    if (numTracks < tracksPerSet) {
		/*
		 * Ok to go to the next track.
		 */
		startingOffset += offsetIncrement;
		if (overlap) {
		    /*
		     * If the current block can overlap to the next track,
		     * use the current offset.  Because of the overlap
		     * there are fewer sectors available for blocks on
		     * the next track.
		     */
		    geoPtr->blockOffset[numBlocks] = offset;
		    numBlocks++;
		    extraSectors = geoPtr->sectorsPerTrack - startingOffset;
		}
		offset = startingOffset + numTracks * geoPtr->sectorsPerTrack;
		if (!overlap) {
		    /*
		     * If no overlap the whole next track is available.
		     */
		    extraSectors = geoPtr->sectorsPerTrack;
		}
	    } else {
		/*
		 * Done.
		 */
		for (index = numBlocks; index < OFS_MAX_ROT_POSITIONS; index++){
		    geoPtr->blockOffset[index] = -1;
		}
		break;
	    }
	}
    }
    geoPtr->blocksPerRotSet = numBlocks;
    geoPtr->tracksPerRotSet = tracksPerSet;
    geoPtr->rotSetsPerCyl = geoPtr->numHeads / tracksPerSet;
    geoPtr->blocksPerCylinder = numBlocks * geoPtr->rotSetsPerCyl;

    /*
     * Now the rotational positions have to be sorted so that rotationally
     * optimal blocks can be found.  The array sortedOffsets is set so
     * that the I'th element has the index into blockOffset which contains
     * the I'th rotational position, eg.
     *	blockOffset	sortedOffsets
     *	    0 (+0)		0
     *	    8 (+0)		2
     *	    4 (+17)		1
     *	   12 (+17)		3
     */

    offsetIncrement = DISK_SECTORS_PER_BLOCK / tracksPerSet;
    for (index = 0 ; index < OFS_MAX_ROT_POSITIONS ; index++) {
	geoPtr->sortedOffsets[index] = -1;
    }
    for (index = 0 ; index < numBlocks ; index++) {
	offset = geoPtr->blockOffset[index] % geoPtr->sectorsPerTrack;
	geoPtr->sortedOffsets[offset/offsetIncrement] = index;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetDomainParts --
 *
 *	Set up the way the domain is divided into 4 areas:  the bitmap
 *	for the file descriptors, the file descriptors, the bitmap for
 *	the data blocks, and the data blocks.
 *
 * Results:
 *	The geometry information is completed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
SetDomainParts(labelPtr, partition, headerPtr)
    register Disk_Label		*labelPtr;	/* The disk label. */
    int partition;
    register Ofs_DomainHeader *headerPtr;
{
    register Ofs_Geometry *geoPtr;
    int numFiles;		/* Number of files computed from the size */
    int numBlocks;		/* Number of blocks in the partition.  This
				 * is computed and then dolled out to the
				 * different things stored in the partition */
    int offset;			/* Block offset within partition */
    int bitmapBytes;		/* Number of bytes taken up by a bitmap */
    int reservedBlocks;		/* Number of blocks reserved at beginning
				 * of partition */
    int maxSector;
    int numCylinders;
    int bytesPerCylinder;

    /*
     * Set aside a number of blocks at the begining of the partition for
     * things like the super block, the boot program, and the domain header.
     */
    geoPtr = &headerPtr->geometry;
    maxSector = labelPtr->labelSector;
    maxSector = Max(maxSector,labelPtr->bootSector + labelPtr->numBootSectors);
    maxSector = Max(maxSector,labelPtr->summarySector
	+ labelPtr->numSummarySectors);
    maxSector = Max(maxSector,labelPtr->domainSector
	+ labelPtr->numDomainSectors);
    numCylinders = labelPtr->partitions[partition].numCylinders;
    if (scsiDisk) {
	/*
	 * For a scsi disk just reserve whole cylinders.
	 */
	int cylinders;
	cylinders = maxSector / (geoPtr->sectorsPerTrack * geoPtr->numHeads);
	if ((maxSector % (geoPtr->sectorsPerTrack * geoPtr->numHeads)) != 0) {
	    cylinders++;
	}
	reservedBlocks = cylinders * geoPtr->blocksPerCylinder;
	numBlocks = geoPtr->blocksPerCylinder * numCylinders - reservedBlocks;
     } else {
	/*
	 * If we are using rotational sets then reserve whole sets.
	 */
	int sets;
	sets = maxSector / (geoPtr->tracksPerRotSet * geoPtr->sectorsPerTrack);
	if ((maxSector % (geoPtr->tracksPerRotSet * geoPtr->sectorsPerTrack)) 
	!= 0) {
	    sets++;
	}
	reservedBlocks = sets * geoPtr->blocksPerRotSet;
	numBlocks = geoPtr->blocksPerCylinder * numCylinders - reservedBlocks;
    }
    printf("Reserving %d blocks for domain header, etc.\n", reservedBlocks);
    /*
     * Determine the number of filesystem blocks available and compute a
     * first guess at the number of file descriptors.  If at the end of
     * the computation things don't fit nicely, then the number of files
     * is changed and the computation is repeated.
     */
    numFiles = 0;
    do {
	if (numFiles == 0) {
	    numFiles = numBlocks * DISK_KBYTES_PER_BLOCK / kbytesToFileDesc;
	}
	numFiles		  &= ~(FSDM_FILE_DESC_PER_BLOCK-1);
	offset			  = reservedBlocks;

	assert(numFiles > 0);
	headerPtr->fdBitmapOffset = offset;
	bitmapBytes		  = (numFiles - 1) / BITS_PER_BYTE + 1;
	headerPtr->fdBitmapBlocks = (bitmapBytes - 1) / FS_BLOCK_SIZE + 1;
	numBlocks		  -= headerPtr->fdBitmapBlocks;
	offset			  += headerPtr->fdBitmapBlocks;

	headerPtr->fileDescOffset = offset;
	headerPtr->numFileDesc	  = numFiles;
	numBlocks		  -= numFiles / FSDM_FILE_DESC_PER_BLOCK;
	offset			  += numFiles / FSDM_FILE_DESC_PER_BLOCK;
	/*
	 * The data blocks will start on a cylinder.  Try the next
	 * cylinder boundary after the start of the bitmap. If the
	 * bitmap won't fit in a cylinder then steal a cylinder from the
	 * data blocks.
	 */
	headerPtr->bitmapOffset	  = offset;
	headerPtr->dataOffset	  = ((offset-1) / geoPtr->blocksPerCylinder + 1)
				     * geoPtr->blocksPerCylinder;
	headerPtr->dataBlocks	  = headerPtr->numCylinders *
					  geoPtr->blocksPerCylinder -
					  headerPtr->dataOffset;
	bytesPerCylinder	  = (geoPtr->blocksPerCylinder + 1) / 2;
	bitmapBytes		  = bytesPerCylinder * (headerPtr->dataBlocks / 
					geoPtr->blocksPerCylinder); 

	headerPtr->bitmapBlocks	  = (bitmapBytes - 1) / FS_BLOCK_SIZE + 1;
	while (headerPtr->bitmapBlocks > 
		headerPtr->dataOffset - headerPtr->bitmapOffset) {
	    /*
	     * If the bit map blocks won't fit in the number cylinders
	     * we've allocated to them then we need to
	     * steal a cylinder from the data blocks.
	     */
	    headerPtr->dataOffset += geoPtr->blocksPerCylinder;
	    headerPtr->dataBlocks -= geoPtr->blocksPerCylinder;
	    bitmapBytes -= geoPtr->blocksPerCylinder * bytesPerCylinder;
	    bitmapBytes		  = bytesPerCylinder * (headerPtr->dataBlocks / 
				    geoPtr->blocksPerCylinder); 
	    headerPtr->bitmapBlocks = (bitmapBytes - 1) / FS_BLOCK_SIZE + 1;
	}
	/*
	 * Check the size of the bit map against space available for it
	 * between the end of the file descriptors and the start of the
	 * data blocks.
	 */
	if (headerPtr->dataOffset - headerPtr->bitmapOffset <
	    headerPtr->bitmapBlocks) {
	    int numBlocksNeeded;
	    /*
	     * Need more blocks to hold the bitmap.  Reduce the number
	     * of file descriptors to get the blocks and re-iterate.
	     */
	    numBlocksNeeded = headerPtr->bitmapBlocks -
		(headerPtr->dataOffset - headerPtr->bitmapOffset);
	    numFiles -= numBlocksNeeded * FSDM_FILE_DESC_PER_BLOCK;
	} else if (headerPtr->dataOffset - headerPtr->bitmapOffset >
		    headerPtr->bitmapBlocks) {
	    int extraBlocks;
	    /*
	     * There are extra blocks between the end of the file descriptors
	     * and the start of the bitmap.  Increase the number of
	     * file descriptors and re-iterate.
	     */
	    extraBlocks = headerPtr->dataOffset - headerPtr->bitmapOffset -
		    headerPtr->bitmapBlocks;
	    numFiles += extraBlocks * FSDM_FILE_DESC_PER_BLOCK;
	}
    } while (headerPtr->dataOffset - headerPtr->bitmapOffset !=
		headerPtr->bitmapBlocks);
    headerPtr->dataCylinders	= headerPtr->dataBlocks /
				  geoPtr->blocksPerCylinder ;
}

/*
 *----------------------------------------------------------------------
 *
 * SetSummaryInfo --
 *
 *	Initialize the summary information for the domain.  
 *
 * Results:
 *	A return code.
 *
 * Side effects:
 *	Fill in the summary info.
 *
 *----------------------------------------------------------------------
 */
static void
SetSummaryInfo(headerPtr, summaryPtr)
    Ofs_DomainHeader *headerPtr;	/* Domain header to summarize */
    Ofs_SummaryInfo *summaryPtr;	/* Summary info to fill in */
{

    bzero((Address)summaryPtr, sizeof(Ofs_SummaryInfo));

    strcpy(summaryPtr->domainPrefix, "(new domain)");
    /*
     * 20 KB is already allocated, 4 for the root directory,
     * 8 for lost+found, and 8 for .fscheck.out
     */
    summaryPtr->numFreeKbytes = headerPtr->dataBlocks * (FS_BLOCK_SIZE / 1024)
				- 20;
    /*
     * 5 file descriptors are already used, 0 and 1 are reserved,
     * 2 is for the root, 3 is for lost+found, and 4 is for .fscheck.out
     */
    summaryPtr->numFreeFileDesc = headerPtr->numFileDesc - 5;
    /*
     * The summary state field is unused.
     */
    summaryPtr->state = 0;
    /*
     * The domain number under which this disk partition is mounted is
     * recorded on disk so servers re-attach disks under the same "name".
     * We set it to the special value so it gets a new number assigned
     * when it is first attached.
     */
    summaryPtr->domainNumber = -1;
    /*
     * The flags field is used to record whether or not the disk has been
     * safely "sync"ed to disk upon shutdown.
     */
    summaryPtr->flags = 0;
    summaryPtr->attachSeconds = 0;
    summaryPtr->detachSeconds = 0;
    summaryPtr->fixCount = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteAllFileDescs --
 *
 *	Write out all of the file descriptors to disk.
 *
 * Results:
 *	SUCCESS if the descriptors were written, FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteAllFileDescs(partFID, headerPtr)
    int partFID;	/* File handle for partition to format */
    register Ofs_DomainHeader *headerPtr;
{
    ReturnStatus status;
    char *bitmap;
    char block[FS_BLOCK_SIZE];
    register Fsdm_FileDescriptor *fileDescPtr;
    register int index;
    int j;

    bitmap = MakeFileDescBitmap(headerPtr);
    if (!printOnly) {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->fdBitmapOffset,
				headerPtr->fdBitmapBlocks, (Address)bitmap);
	if (status != SUCCESS) {
	    printf("Couldn't write bitmap\n");
	    return(status);
	}
    }
    /*
     * Make the first block of file descriptors.  This contains some
     * canned file descriptors for the root, bad block file, the
     * lost and found directory and fscheck's output file.  
     */
    bzero(block, FS_BLOCK_SIZE);
    for (index = 0;
         index < FSDM_FILE_DESC_PER_BLOCK;
	 index++ ) {
	fileDescPtr = (Fsdm_FileDescriptor *)((int)block +
					   index * FSDM_MAX_FILE_DESC_SIZE);
	fileDescPtr->magic = FSDM_FD_MAGIC;
	if (index < FSDM_BAD_BLOCK_FILE_NUMBER) {
	    fileDescPtr->flags = FSDM_FD_RESERVED;
	} else if (index == FSDM_BAD_BLOCK_FILE_NUMBER) {
	    InitDesc(fileDescPtr, FS_FILE, 0, -1, -1, -1, 0, 0, 0, 
		curTime.tv_sec);
	} else if (index == FSDM_ROOT_FILE_NUMBER) {
	    InitDesc(fileDescPtr, FS_DIRECTORY, FS_BLOCK_SIZE, -1, -1, -1, 0,
		    0, 0755, curTime.tv_sec);
	    /*
	     * Place the data in the first file system block.
	     */
	    fileDescPtr->direct[0] = 0;
	} else if (index == FSDM_LOST_FOUND_FILE_NUMBER) {
	    InitDesc(fileDescPtr, FS_DIRECTORY, 2 * FS_BLOCK_SIZE,-1, -1, -1, 0,
		    0, 0777, curTime.tv_sec);
	    for (j = 0; j < OFS_NUM_LOST_FOUND_BLOCKS ; j++) {
		fileDescPtr->direct[j] = FS_FRAGMENTS_PER_BLOCK 
		    * (j + 1);
	    }
	} else if (index == FSDM_LOST_FOUND_FILE_NUMBER+1) {
	    InitDesc(fileDescPtr, FS_FILE, 2 * FS_BLOCK_SIZE, -1, -1, -1, 0,
		    0, 0755, curTime.tv_sec);
	    for (j = 0; j < 2 ; j++) {
		fileDescPtr->direct[j] = FS_FRAGMENTS_PER_BLOCK 
		    * (j + 1 + OFS_NUM_LOST_FOUND_BLOCKS);
	    }
	} else {
	    fileDescPtr->flags = FSDM_FD_FREE;
	}
    }
    if (!printOnly) {
	/*
	 * Write out the first, specially hand crafted, block of file
	 * descriptors.
	 */
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->fileDescOffset,
				    1, (Address)block);
	if (status != SUCCESS) {
	    return(status);
	}
	/*
	 * Redo the block for the remaining file descriptors
	 */
	bzero(block, FS_BLOCK_SIZE);
	for (index = 0;
	     index < FSDM_FILE_DESC_PER_BLOCK;
	     index++ ) {
	    fileDescPtr = (Fsdm_FileDescriptor *)((int)block + index *
					       FSDM_MAX_FILE_DESC_SIZE);
	    fileDescPtr->magic = FSDM_FD_MAGIC;
	    fileDescPtr->flags = FSDM_FD_FREE;
	}
	/*
	 * Write out the remaining file descriptors.
	 */
	for (index = FSDM_FILE_DESC_PER_BLOCK;
	     index < headerPtr->numFileDesc;
	     index += FSDM_FILE_DESC_PER_BLOCK) {
	    status = Disk_BlockWrite(partFID, headerPtr,
		     headerPtr->fileDescOffset + (index/FSDM_FILE_DESC_PER_BLOCK),
		     1, (Address)block);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
    } else {
	status = SUCCESS;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * MakeFileDescBitmap --
 *
 *	Compute out the bitmap for file descriptor array to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static char *
MakeFileDescBitmap(headerPtr)
    register Ofs_DomainHeader *headerPtr;
{
    register char *bitmap;
    register int index;

    /*
     * Allocate and initialize the bitmap to all 0"s to mean all free.
     */
    bitmap = (char *)malloc((unsigned) headerPtr->fdBitmapBlocks *
				 FS_BLOCK_SIZE);
    bzero((Address)bitmap, headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE);

    /*
     * Reserve file descriptors 0, 1, 2, 3, and 4. File number 0 is not used at 
     * all in the filesystem.  File number 1 is for the file with bad blocks.
     * File number 2 (FSDM_ROOT_FILE_NUMBER) is the domain's root directory.
     * File number 3 (FS_LOST_FOUND_NUMBER) is the directory where lost
     * files are stored.
     * File number 4 is for fscheck's output.
     *
     * IF THIS CHANGES remember to fix SetSummaryInfo
     */
    bitmap[0] |= 0xf8;
    freeFDNum = 5;
    /*
     * Set the bits in the map at the end that don't correspond to
     * any existing file descriptors.
     */
    index = headerPtr->numFileDesc / BITS_PER_BYTE;
    if (headerPtr->numFileDesc % BITS_PER_BYTE) {
	register int bitIndex;
	/*
	 * Take care the last byte that only has part of its bits set.
	 */
	for (bitIndex = headerPtr->numFileDesc % BITS_PER_BYTE;
	     bitIndex < BITS_PER_BYTE;
	     bitIndex++) {
	    bitmap[index] |= 1 << ((BITS_PER_BYTE - 1) - bitIndex);
	}
	index++;
    }
    index++;
    for ( ; index < headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE; index++) {
	bitmap[index] = 0xff;
    }

    if (printOnly) {
	Disk_PrintFileDescBitmap(headerPtr, bitmap);
    }
    return(bitmap);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteBitmap --
 *
 *	Write out the bitmap for the data blocks.  This knows that the
 *	first 4K is allocated to the root directory, 8K is
 *	allocated to lost and found, and 8K to .fscheck.out.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	Write the bitmap.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteBitmap(partFID, headerPtr)
    int partFID;
    register Ofs_DomainHeader *headerPtr;
{
    ReturnStatus status;
    char *bitmap;

    bitmap = (char *)malloc((unsigned) headerPtr->bitmapBlocks * FS_BLOCK_SIZE);
    bzero(bitmap, headerPtr->bitmapBlocks * FS_BLOCK_SIZE);
    /*
     * Set the bits corresponding to the 4K used for the root directory,
     * the 8K reserved for lost and found, and the 8K reserved for
     * .fscheck.out.
     *   ________
     *	|0______7|	Bits are numbered like this in a byte.
     *
     * IF THIS CHANGES remember to fix SetSummaryInfo()
     */
    bitmap[0] |= 0xff;
    bitmap[1] |= 0xff;
    bitmap[2] |= 0xf0;
    freeBlockNum = 5;
    if (printOnly) {
	Disk_PrintDataBlockBitmap(headerPtr, bitmap);
	status = SUCCESS;
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->bitmapOffset,
			    headerPtr->bitmapBlocks, (Address)bitmap);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteRootDirectory --
 *
 *	Write the data blocks of the root directory.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	Write the root directory"s data block.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteRootDirectory(partFID, headerPtr, fdPtr)
    int 			partFID;	/* Raw disk handle. */
    register Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    Fsdm_FileDescriptor		*fdPtr;		/* Root file desc */
{
    ReturnStatus 	status;
    char 		block[FS_BLOCK_SIZE];
    Fslcl_DirEntry 	*dirEntryPtr;
    int 		i;
    DirIndexInfo	indexInfo;

    CreateDir(block, 1, FSDM_ROOT_FILE_NUMBER, FSDM_ROOT_FILE_NUMBER);
    if (printOnly) {
	int offset;
	printf("Root Directory\n");
	offset = 0;
	for (i = 0; i < 4; i++) {
	    dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	    Disk_PrintDirEntry(dirEntryPtr);
	    offset += dirEntryPtr->recordLength;
	}
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset,
	    1, block);
	if (status != 0) {
	    fprintf(stderr, "WriteRootDirectory: Couldn't write directory\n");
	}
    }
    status = OpenDir(partFID, headerPtr, fdPtr, &indexInfo, &dirEntryPtr);
    if (status != SUCCESS) {
	fprintf(stderr, "Can't open root file descriptor\n");
	return status;
    }
    status = AddToDirectory(partFID, headerPtr, !printOnly, &indexInfo, 
			&dirEntryPtr, FSDM_LOST_FOUND_FILE_NUMBER, 
			"lost+found");
    if (status != SUCCESS) {
	fprintf(stderr, "Can't add lost+found to root directory.\n");
	return status;
    }
    fdPtr->numLinks++;
    status = AddToDirectory(partFID, headerPtr, !printOnly, &indexInfo,
			&dirEntryPtr, FSDM_LOST_FOUND_FILE_NUMBER + 1, 
			".fscheck.out");
    if (status != SUCCESS) {
	fprintf(stderr, "Can't add .fscheck.out to root directory.\n");
	return status;
    }
    CloseDir(partFID, headerPtr, !printOnly, &indexInfo);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteLostFoundDirectory --
 *
 *	Write the data blocks of the lost and found directory.
 *
 * Results:
 *	A return code from the writes.
 *
 * Side effects:
 *	Write the root directory"s data block.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteLostFoundDirectory(partFID, headerPtr)
    int partFID;
    register Ofs_DomainHeader *headerPtr;
{
    ReturnStatus 	status = SUCCESS;
    char 		block[OFS_NUM_LOST_FOUND_BLOCKS * FS_BLOCK_SIZE];
    Fslcl_DirEntry 	*dirEntryPtr;
    int 		i;

    CreateDir(block, OFS_NUM_LOST_FOUND_BLOCKS, FSDM_LOST_FOUND_FILE_NUMBER, 
	FSDM_ROOT_FILE_NUMBER);
    if (printOnly) {
	int offset;
	printf("lost+found Directory\n");
	offset = 0;
	for (i = 0; i < 4; i++) {
	    dirEntryPtr = (Fslcl_DirEntry *)((int)block + offset);
	    Disk_PrintDirEntry(dirEntryPtr);
	    offset += dirEntryPtr->recordLength;
	}
    } else {
	status = Disk_BlockWrite(partFID, headerPtr, headerPtr->dataOffset + 1,
			OFS_NUM_LOST_FOUND_BLOCKS, block);
	if (status != 0) {
	    fprintf(stderr, 
		"WriteLostFoundDirectory: Couldn't write directory\n");
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * InitDesc --
 *
 *	Set up a file descriptor as allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File descriptor fields filled in.
 *
 *----------------------------------------------------------------------
 */
static void
InitDesc(fileDescPtr, fileType, numBytes, devServer, devType, devUnit, uid,
	gid, permissions, time)
    Fsdm_FileDescriptor	*fileDescPtr;
    int			fileType;
    int			numBytes;
    int			devServer;
    int			devType;
    int			devUnit;
    int			uid;
    int			gid;
    int			permissions;
    long		time;
{
    int		index;

    fileDescPtr->flags = FSDM_FD_ALLOC;
    fileDescPtr->fileType = fileType;
    fileDescPtr->permissions = permissions;
    fileDescPtr->uid = uid;
    fileDescPtr->gid = gid;
    fileDescPtr->lastByte = numBytes - 1;
    fileDescPtr->firstByte = -1;
    if (fileType == FS_DIRECTORY) {
	fileDescPtr->numLinks = 2;
    } else {
	fileDescPtr->numLinks = 1;
    }
    fileDescPtr->devServerID = devServer;
    fileDescPtr->devType = devType;
    fileDescPtr->devUnit = devUnit;

    /*
     * Set the time stamps.  This assumes that universal time,
     * not local time, is used for time stamps.
     */
    fileDescPtr->createTime = (int) time;
    fileDescPtr->accessTime = (int) time;
    fileDescPtr->descModifyTime = (int) time;
    fileDescPtr->dataModifyTime = (int) time;

    /*
     * Place the data in the first filesystem block.
     */
    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    if (numBytes > 0) {
	int	numBlocks;

	numBlocks = (numBytes - 1) / FS_BLOCK_SIZE + 1;
	if (numBlocks > FSDM_NUM_DIRECT_BLOCKS) {
	    fileDescPtr->numKbytes = (numBlocks + 1) * (FS_BLOCK_SIZE / 1024);
	} else {
	    fileDescPtr->numKbytes = (numBytes + 1023) / 1024;
	}
    } else {
	fileDescPtr->numKbytes = 0;
    }

    fileDescPtr->version = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * AddToDirectory --
 *
 *	Add the file descriptor to a directory.  
 *
 * Results:
 *	SUCCESS if the desciptor was added ok, FAILURE otherwise
 *
 * Side effects:
 *	The directory is modified to contain the file.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
AddToDirectory(fid, headerPtr, writeDisk, dirIndexPtr, dirEntryPtrPtr, 
    fileNumber, fileName)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    Boolean		writeDisk;	/* Write the disk? */
    DirIndexInfo	*dirIndexPtr;	/* Index information. */
    Fslcl_DirEntry	**dirEntryPtrPtr; /* Directory entry. */
    int		 	fileNumber;	/* File number to add. */
    char 		*fileName;	/* Name of file to add. */
{
    int		 	nameLength;
    int		 	recordLength;
    int		 	leftOver;
    int			oldRecLength;
    Fslcl_DirEntry	*dirEntryPtr;
    ReturnStatus 	status = SUCCESS;

    dirEntryPtr = *dirEntryPtrPtr;

    nameLength = strlen(fileName);
    recordLength = Fslcl_DirRecLength(nameLength);

    while (1) {
	if (dirEntryPtr->fileNumber != 0) {
	    oldRecLength = Fslcl_DirRecLength(dirEntryPtr->nameLength);
	    leftOver = dirEntryPtr->recordLength - oldRecLength;
	    if (leftOver >= recordLength) {
		dirEntryPtr->recordLength = oldRecLength;
		dirEntryPtr = 
			(Fslcl_DirEntry *) ((int) dirEntryPtr + oldRecLength);
		dirEntryPtr->recordLength = leftOver;
		dirIndexPtr->dirOffset += oldRecLength;
	    } else {
		status = NextDirEntry(fid, headerPtr, writeDisk,
			    dirIndexPtr, &dirEntryPtr);
		if (status != SUCCESS) {
		    fprintf(stderr, "Can't add to directory\n");
		    return status;
		}
		continue;
	    }
	} else if (dirEntryPtr->recordLength < recordLength) {
	    status = NextDirEntry(fid, headerPtr, writeDisk,
			dirIndexPtr, &dirEntryPtr);
	    if (status != SUCCESS) {
		fprintf(stderr, "Can't add to directory\n");
		return status;
	    }
	    continue;
	}
	dirEntryPtr->fileNumber = fileNumber;
	dirEntryPtr->nameLength = nameLength;
	(void)strcpy(dirEntryPtr->fileName, fileName);
	leftOver = dirEntryPtr->recordLength - recordLength;
	if (leftOver > FSLCL_DIR_ENTRY_HEADER) {
	    dirEntryPtr->recordLength = recordLength;
	    dirEntryPtr =(Fslcl_DirEntry *) ((int) dirEntryPtr + recordLength);
	    dirEntryPtr->fileNumber = 0;
	    dirEntryPtr->recordLength = leftOver;
	    dirIndexPtr->dirOffset += recordLength;
	} else {
	    status = NextDirEntry(fid, headerPtr, writeDisk, 
			    dirIndexPtr, &dirEntryPtr);
	    if (status != SUCCESS) {
		fprintf(stderr, "Can't add to directory\n");
		return status;
	    }
	}
	*dirEntryPtrPtr = dirEntryPtr;
	return status;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * OpenDir --
 *
 *	Set up the structure to allow moving through the given directory.
 *
 * Results:
 *	SUCCESS if the open was successful, FAILURE otherwise
 *
 * Side effects:
 *	The index structure is set up and *dirEntryPtrPtr set to point to
 *	the first directory entry.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
OpenDir(fid, headerPtr, fdPtr, indexInfoPtr, dirEntryPtrPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    Fsdm_FileDescriptor	*fdPtr;		/* The file descriptor for the
					 * directory. */
    DirIndexInfo 	*indexInfoPtr;	/* Index info struct */
    Fslcl_DirEntry	**dirEntryPtrPtr; /* Current directory entry */
{

    *dirEntryPtrPtr = NULL;
    if (fdPtr->lastByte == -1) {
	/*
	 * Empty directory.
	 */
	return SUCCESS;
    } else if ((fdPtr->lastByte + 1) % FSLCL_DIR_BLOCK_SIZE != 0) {
	fprintf(stderr, "Directory not multiple of directory block size.\n");
	return FAILURE;
    } else if (fdPtr->fileType != FS_DIRECTORY) {
	fprintf(stderr, "OpenDir: Not a directory\n");
	return FAILURE;
    }

    /*
     * Initialize the index structure.
     */
    indexInfoPtr->fdPtr = fdPtr;
    indexInfoPtr->blockNum = 0;
    indexInfoPtr->blockAddr = fdPtr->direct[0] / FS_FRAGMENTS_PER_BLOCK + 
			      headerPtr->dataOffset;
    /*
     * Read in the directory block.
     */
    if (fdPtr->lastByte != FS_BLOCK_SIZE - 1) {
	fprintf(stderr, "We created a directory that's not 4K?\n");
	return FAILURE;
    }
    if (Disk_BlockRead(fid, headerPtr,
		       indexInfoPtr->blockAddr,
		       1, indexInfoPtr->dirBlock) < 0) {
	fprintf(stderr, "OpenDir: Read failed block %d\n",
			indexInfoPtr->blockAddr);
	return FAILURE;
    } 
    indexInfoPtr->dirOffset = 0;
    *dirEntryPtrPtr = (Fslcl_DirEntry *) indexInfoPtr->dirBlock;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * NextDirEntry --
 *
 *	Get a pointer to the next directory entry.
 *
 * Results:
 *	SUCCESS if the next entry was found ok, FAILURE otherwise
 *
 * Side effects:
 *	The index structure is modified and *dirEntryPtrPtr set to point
 *	to the next directory entry.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
NextDirEntry(fid, headerPtr, writeDisk, indexInfoPtr, dirEntryPtrPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    Boolean		writeDisk;	/* Write the disk? */
    DirIndexInfo 	*indexInfoPtr;	/* Index information. */
    Fslcl_DirEntry	**dirEntryPtrPtr; /* Current directory entry */
{
    Fslcl_DirEntry	*dirEntryPtr;	

    dirEntryPtr = *dirEntryPtrPtr;
    indexInfoPtr->dirOffset += dirEntryPtr->recordLength;
    if (indexInfoPtr->dirOffset < FS_BLOCK_SIZE) {
	/*
	 * The next directory entry is in the current block.
	 */
	*dirEntryPtrPtr = (Fslcl_DirEntry *)
			&(indexInfoPtr->dirBlock[indexInfoPtr->dirOffset]);
	return SUCCESS;
    } else {
	Fsdm_FileDescriptor	*fdPtr;
	int			i;

	printf("Adding new block to directory ...\n");

	/*
	 * Write out the current block and set up the next one.
	 */
	if (writeDisk) {
	    if (Disk_BlockWrite(fid, headerPtr, indexInfoPtr->blockAddr,
				1, indexInfoPtr->dirBlock) < 0) {
		fprintf(stderr, "NextDirEntry: Write failed block %d\n",
				indexInfoPtr->blockAddr);
		return FAILURE;
	    }
	}
	fdPtr = indexInfoPtr->fdPtr;
	fdPtr->lastByte += FS_BLOCK_SIZE;
	fdPtr->numKbytes += FS_FRAGMENTS_PER_BLOCK;
	indexInfoPtr->blockNum++;
	fdPtr->direct[indexInfoPtr->blockNum] = 
				freeBlockNum * FS_FRAGMENTS_PER_BLOCK;
	MarkDataBitmap(headerPtr, cylBitmapPtr, freeBlockNum,
		       FS_FRAGMENTS_PER_BLOCK);
	indexInfoPtr->blockAddr = freeBlockNum + headerPtr->dataOffset;
	freeBlockNum++;
	for (i = 0, dirEntryPtr = (Fslcl_DirEntry *)indexInfoPtr->dirBlock; 
	     i < FS_BLOCK_SIZE / FSLCL_DIR_BLOCK_SIZE;
     i++,dirEntryPtr=(Fslcl_DirEntry *)((unsigned)dirEntryPtr+FSLCL_DIR_BLOCK_SIZE)) {
	    dirEntryPtr->fileNumber = 0;
	    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	    dirEntryPtr->nameLength = 0;
	}
	indexInfoPtr->dirOffset = 0;
	*dirEntryPtrPtr = (Fslcl_DirEntry *) indexInfoPtr->dirBlock;
	return SUCCESS;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * CloseDir --
 *
 *	Flushes the current directory block to disk, if necessary.
 *
 * Results:
 *	SUCCESS if the directory was closed ok, FAILURE otherwise
 *
 * Side effects:
 *	Stuff is written to disk.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
CloseDir(fid, headerPtr, writeDisk, indexInfoPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    Boolean		writeDisk;	/* Write the disk? */
    DirIndexInfo 	*indexInfoPtr;	/* Index information. */
{

    if (writeDisk) {
	if (Disk_BlockWrite(fid, headerPtr, indexInfoPtr->blockAddr,
			    1, indexInfoPtr->dirBlock) < 0) {
	    fprintf(stderr, "CloseDir: Write (2) failed block %d\n",
			    indexInfoPtr->blockAddr);
	    return FAILURE;
	}
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadFileDesc --
 *
 *	Return the given file descriptor.
 *
 * Results:
 *	SUCCESS if file descriptor was read ok, FAILURE otherwise
 *
 * Side effects:
 *	The file descriptor struct is filled in.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
ReadFileDesc(fid, headerPtr, fdNum, fdPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    int			fdNum;		/* File number. */
    Fsdm_FileDescriptor	*fdPtr;		/* Place to store fd. */
{
    static char		block[FS_BLOCK_SIZE];
    int			blockNum;
    int			offset;

    blockNum = headerPtr->fileDescOffset + fdNum / FSDM_FILE_DESC_PER_BLOCK;
    offset = (fdNum & (FSDM_FILE_DESC_PER_BLOCK - 1)) * FSDM_MAX_FILE_DESC_SIZE;
    if (Disk_BlockRead(fid, headerPtr, blockNum, 1, 
		       (Address) block) < 0) {
	fprintf(stderr, "ReadFileDesc: Read failed\n");
	return FAILURE;
    }
    bcopy((Address)&block[offset], (Address)fdPtr, sizeof(Fsdm_FileDescriptor));
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * WriteFileDesc --
 *
 *	Write the given file descriptor.
 *
 * Results:
 *	SUCCESS if the write was successful, FAILURE otherwise
 *
 * Side effects:
 *	Stuff is written to disk.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
WriteFileDesc(fid, headerPtr, fdNum, fdPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    int			fdNum;		/* File number. */
    Fsdm_FileDescriptor	*fdPtr;		/* Place to store fd. */
{
    static char		block[FS_BLOCK_SIZE];
    int			blockNum;
    int			offset;

    blockNum = headerPtr->fileDescOffset + fdNum / FSDM_FILE_DESC_PER_BLOCK;
    offset = (fdNum & (FSDM_FILE_DESC_PER_BLOCK - 1)) * FSDM_MAX_FILE_DESC_SIZE;
    if (Disk_BlockRead(fid, headerPtr, blockNum, 1, 
		       (Address) block) < 0) {
	fprintf(stderr, "WriteFileDesc: Read failed\n");
	return FAILURE;
    }
    bcopy((Address)fdPtr, (Address)&block[offset], sizeof(Fsdm_FileDescriptor));
    if (Disk_BlockWrite(fid, headerPtr, blockNum, 1, 
		       (Address) block) < 0) {
	fprintf(stderr, "WriteFileDesc: Write failed\n");
	return FAILURE;
    }
    return SUCCESS;
}
/*
 *----------------------------------------------------------------------
 *
 * CreateDir --
 *
 *	Create a directory out of file system blocks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File system block set up as a directory.
 *
 *----------------------------------------------------------------------
 */
static void
CreateDir(blocks, numBlocks, dot, dotDot)
    Address	blocks;		/* Blocks to create directory in. */
    int		numBlocks;	/* Number of blocks. */
    int		dot;		/* File number of directory. */
    int		dotDot;		/* File number of parent. */
{
    Fslcl_DirEntry	*dirEntryPtr;
    char	*fileName;
    int		offset;
    int		length;
    int		i;

    dirEntryPtr = (Fslcl_DirEntry *)blocks;
    fileName = ".";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = dot;
    dirEntryPtr->recordLength = Fslcl_DirRecLength(length);
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);
    offset = dirEntryPtr->recordLength;

    dirEntryPtr = (Fslcl_DirEntry *)((int)blocks + offset);
    fileName = "..";
    length = strlen(fileName);
    dirEntryPtr->fileNumber = dotDot;
    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - offset;
    dirEntryPtr->nameLength = length;
    strcpy(dirEntryPtr->fileName, fileName);
    /*
     * Fill out the rest of the directory with empty blocks.
     */
    for (dirEntryPtr = (Fslcl_DirEntry *)&blocks[FSLCL_DIR_BLOCK_SIZE], i = 1; 
	 i < (FS_BLOCK_SIZE * numBlocks) / FSLCL_DIR_BLOCK_SIZE;
	 i++,dirEntryPtr=(Fslcl_DirEntry *)((int)dirEntryPtr + FSLCL_DIR_BLOCK_SIZE)) {
	 dirEntryPtr->fileNumber = 0;
	 dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE;
	 dirEntryPtr->nameLength = 0;
    }
}
int fragMasks[FS_FRAGMENTS_PER_BLOCK + 1] = {0x0, 0x08, 0x0c, 0x0e, 0x0f};


/*
 *----------------------------------------------------------------------
 *
 * MarkDataBitmap --
 *
 *	Mark the appropriate bits in the data block bitmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data block marked.
 *
 *----------------------------------------------------------------------
 */
static void
MarkDataBitmap(headerPtr, cylBitmapPtr, blockNum, numFrags)
    Ofs_DomainHeader	*headerPtr;
    unsigned char	*cylBitmapPtr;
    int			blockNum;
    int			numFrags;
{
    unsigned char	*bitmapPtr;

    bitmapPtr = GetBitmapPtr(headerPtr, cylBitmapPtr, blockNum);
    if ((blockNum % headerPtr->geometry.blocksPerCylinder) & 0x1) {
	*bitmapPtr |= fragMasks[numFrags];
    } else {
	*bitmapPtr |= fragMasks[numFrags] << 4;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReadFileDescBitmap --
 *
 *	Read in the file descriptor bitmap.
 *
 * Results:
 *	A pointer to the file descriptor bit map.
 *
 * Side effects:
 *	Memory allocated for the bit map.
 *
 *----------------------------------------------------------------------
 */
static unsigned char *
ReadFileDescBitmap(fid, headerPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
{
    register unsigned char *bitmap;

    /*
     * Allocate the bitmap.
     */
    bitmap = (unsigned char *)malloc(
	(unsigned) headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE);
    if (Disk_BlockRead(fid, headerPtr, headerPtr->fdBitmapOffset,
		  headerPtr->fdBitmapBlocks, (Address)bitmap) < 0) {
	fprintf(stderr, "ReadFileDescBitmap: Read failed");
	exit(1);
    }
    return(bitmap);
}

/*
 *----------------------------------------------------------------------
 *
 * ReadBitmap --
 *
 *	Read the bitmap off disk.
 *
 * Results:
 *	A pointer to the bitmap.
 *
 * Side effects:
 *	Memory allocated for the bit map.
 *
 *----------------------------------------------------------------------
 */
static unsigned char *
ReadBitmap(fid, headerPtr)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
{
    unsigned char *bitmap;

    bitmap = (unsigned char *)malloc((unsigned) 
	    headerPtr->bitmapBlocks * FS_BLOCK_SIZE);
    if (Disk_BlockRead(fid, headerPtr, headerPtr->bitmapOffset,
		  headerPtr->bitmapBlocks, (Address) bitmap) < 0) {
	fprintf(stderr, "ReadBitmap: Read failed");
	exit(1);
    }
    return(bitmap);
}

/*
 *----------------------------------------------------------------------
 *
 * WriteFileDescBitmap --
 *
 *	Write out the file descriptor bitmap.
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
WriteFileDescBitmap(fid, headerPtr, bitmap)
    int			fid;		/* Handle on raw disk. */
    Ofs_DomainHeader 	*headerPtr;	/* Domain header. */
    unsigned char 	*bitmap;	/* Bitmap to write. */
{
    if (Disk_BlockWrite(fid, headerPtr, headerPtr->fdBitmapOffset,
		   headerPtr->fdBitmapBlocks, (Address)bitmap) < 0) {
	fprintf(stderr, "WriteFileDescBitmap: Write failed");
	exit(1);
    }
}

