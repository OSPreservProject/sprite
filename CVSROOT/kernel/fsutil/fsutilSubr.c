/* 
 * fsutilSubr.c --
 *
 *	Miscellaneous routines.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>

#include <fs.h>
#include <vm.h>
#include <rpc.h>
#include <fsutil.h>
#include <fsdm.h>
#include <fslcl.h>
#include <fsprefix.h>
#include <fsNameOps.h>
#include <fsutilTrace.h>
#include <fspdev.h>
#include <fsStat.h>
#include <devDiskLabel.h>
#include <dev.h>
#include <sync.h>
#include <timer.h>
#include <proc.h>
#include <trace.h>
#include <hash.h>
#include <fsrmt.h>

#include <stdio.h>



/*
 *----------------------------------------------------------------------
 *
 * Fsutil_DomainInfo --
 *
 *	Return info about the given domain.
 *	FIX ME FIX ME FIX ME
 *	This should be replaced by a call through the domain switch.
 *	The prefix table module has the domain type, so can do this.
 *	For now, we infer the domain type from the stream type.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus 
Fsutil_DomainInfo(fileIDPtr, domainInfoPtr)
    Fs_FileID		*fileIDPtr;	/* FileID from the prefix table,
					 * This can be changed to make
					 * it match with what a user sees
					 * when it stats the file.  This
					 * is important when computing
					 * the current directory in getwd(). */
    Fs_DomainInfo	*domainInfoPtr;	/* Fill in with # free blocks, etc */
{
    ReturnStatus	status;

    switch (fileIDPtr->type) {
	case FSIO_LCL_FILE_STREAM:
	    status = Fsdm_DomainInfo(fileIDPtr, domainInfoPtr);
	    break;
	case FSIO_PFS_NAMING_STREAM:
	case FSIO_RMT_FILE_STREAM:
	    status = Fsrmt_DomainInfo(fileIDPtr, domainInfoPtr);
	    break;
	case FSIO_LCL_PSEUDO_STREAM:
	    status = FspdevPfsDomainInfo(fileIDPtr, domainInfoPtr);
	    break;
	default:
	    printf("Fsutil_DomainInfo: Unexpected stream type <%d>\n",
		    fileIDPtr->type);
	    status = FS_DOMAIN_UNAVAILABLE;
	    break;
    }
    if (status != SUCCESS) {
	domainInfoPtr->maxKbytes = -1;
	domainInfoPtr->freeKbytes = -1;
	domainInfoPtr->maxFileDesc = -1;
	domainInfoPtr->freeFileDesc = -1;
	domainInfoPtr->blockSize = -1;
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_FileError --
 *
 *	Print an error message about a file.
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
Fsutil_FileError(hdrPtr, string, status)
    Fs_HandleHeader *hdrPtr;
    char *string;
    int status;
{
    if (hdrPtr == (Fs_HandleHeader *)NIL) {
	printf("(NIL handle) %s: ", string);
    } else {
	Net_HostPrint(hdrPtr->fileID.serverID,
		      Fsutil_FileTypeToString(hdrPtr->fileID.type));
	printf(" \"%s\" <%d,%d> %s: ", Fsutil_HandleName(hdrPtr),
		hdrPtr->fileID.major, hdrPtr->fileID.minor, string);
    }
    Fsutil_PrintStatus(status);
    printf("\n");
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_PrintStatus --
 *
 *	Print out an error status, using a mnemonic if possible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A print statement.
 *
 *----------------------------------------------------------------------
 */
void
Fsutil_PrintStatus(status)
    int status;
{
    switch (status) {
	case SUCCESS:
	    break;
	case FS_DOMAIN_UNAVAILABLE:
	    printf("domain unavailable");
	    break;
	case FS_VERSION_MISMATCH:
	    printf("version mismatch");
	    break;
	case FAILURE:
	    printf("cacheable/busy conflict");
	    break;
	case RPC_TIMEOUT:
	    printf("rpc timeout");
	    break;
	case RPC_SERVICE_DISABLED:
	    printf("server rebooting");
	    break;
	case FS_STALE_HANDLE:
	    printf("stale handle");
	    break;
	case DEV_RETRY_ERROR:
	case DEV_HARD_ERROR:
	    printf("DISK ERROR");
	    break;
	case FS_NO_DISK_SPACE:
	    printf("out of disk space");
	default:
	    printf("<%x>", status);
	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_FileTypeToString --
 *
 *	Map a stream type to a string.  Used for error messages.
 *
 * Results:
 *	A string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
Fsutil_FileTypeToString(type)
    int type;
{
    register char *fileType;

    switch (type) {
	case FSIO_STREAM:
	    fileType = "Stream";
	    break;
	case FSIO_LCL_FILE_STREAM:
	    fileType = "File";
	    break;
	case FSIO_RMT_FILE_STREAM:
	    fileType = "RmtFile";
	    break;
	case FSIO_LCL_DEVICE_STREAM:
	    fileType = "Device";
	    break;
	case FSIO_RMT_DEVICE_STREAM:
	    fileType = "RmtDevice";
	    break;
	case FSIO_LCL_PIPE_STREAM:
	    fileType = "Pipe";
	    break;
	case FSIO_RMT_PIPE_STREAM:
	    fileType = "RmtPipe";
	    break;
#ifdef notdef
	case FS_LCL_NAMED_PIPE_STREAM:
	    fileType = "NamedPipe";
	    break;
	case FS_RMT_NAMED_PIPE_STREAM:
	    fileType = "RmtNamedPipe";
	    break;
#endif
	case FSIO_CONTROL_STREAM:
	    fileType = "PdevControlStream";
	    break;
	case FSIO_SERVER_STREAM:
	    fileType = "SrvStream";
	    break;
	case FSIO_LCL_PSEUDO_STREAM:
	    fileType = "LclPdev";
	    break;
	case FSIO_RMT_PSEUDO_STREAM:
	    fileType = "RmtPdev";
	    break;
	case FSIO_PFS_CONTROL_STREAM:
	    fileType = "PfsControlStream";
	    break;
	case FSIO_PFS_NAMING_STREAM:
	    fileType = "PfsNamingStream";
	    break;
	case FSIO_LCL_PFS_STREAM:
	    fileType = "LclPfs";
	    break;
	case FSIO_RMT_PFS_STREAM:
	    fileType = "RmtPfs";
	    break;
#ifdef INET
	case FSIO_RAW_IP_STREAM:
	    fileType = "RawIp Socket";
	    break;
	case FSIO_UDP_STREAM:
	    fileType = "UDP Socket";
	    break;
	case FSIO_TCP_STREAM:
	    fileType = "TCP Socket";
	    break;
#endif
#ifdef notdef
	case FS_RMT_UNIX_STREAM:
	    fileType = "UnixFile";
	    break;
	case FS_RMT_NFS_STREAM:
	    fileType = "NFSFile";
	    break;
#endif
	default:
	    fileType = "<unknown file type>";
	    break;
    }
    return(fileType);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_GetFileName --
 *
 *	Return a pointer to the file name for the given stream.
 *
 * Results:
 *	Pointer to file name from handle of given stream.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
Fsutil_GetFileName(streamPtr)
    Fs_Stream	*streamPtr;
{
    if (streamPtr->hdr.name != (char *)NIL) {
	return(streamPtr->hdr.name);
    } else if (streamPtr->ioHandlePtr != (Fs_HandleHeader *)NIL) {
	return(streamPtr->ioHandlePtr->name);
    } else {
	return("(noname)");
    }
}
