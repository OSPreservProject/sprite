/*
 * mount.c --
 * 
 *	Utility procedures for using the Sun Mount protocol.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#ifndef lint
static char rcsid[] = "$Header: /a/newcmds/nfsmount/RCS/mount.c,v 1.5 89/10/10 13:17:09 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"

#include "nfs.h"


/*
 *----------------------------------------------------------------------
 *
 * Nfs_MountInitClient --
 *
 *	Set up the CLIENT data structure needed to do SUN RPC to the
 *	mount program running on a particular host.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls clnt_create which sets up sockets and other related state.
 *
 *----------------------------------------------------------------------
 */
CLIENT *
Nfs_MountInitClient(host)
    char *host;
{
    register CLIENT *clnt;
    int voidArg;
    VoidPtr voidRes;

    clnt = clnt_create(host, MOUNTPROG, MOUNTVERS, "udp");
    if (clnt == (CLIENT *)NULL) {
	clnt_pcreateerror(host);
    } else {
	clnt->cl_auth = authunix_create_default();
	voidRes = mountproc_null_1(&voidArg, clnt);
	if (voidRes == (VoidPtr)NULL) {
	    clnt_perror(clnt, "mountproc_null_1");
	} else if (pdev_Trace) {
	    printf("Null RPC to Mount service at %s succeeded\n", host);
	}
    }
    return(clnt);
}

/*
 *----------------------------------------------------------------------
 *
 * Nfs_MountTest --
 *
 *	Test the mount protocol by making a null rpc to the mount server.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error and exits the process if the RPC fails.
 *
 *----------------------------------------------------------------------
 */
void
Nfs_MountTest(clnt, host)
    CLIENT *clnt;
    char *host;
{
    char voidArg;
    VoidPtr voidRes;

    voidRes = mountproc_null_1(&voidArg, clnt);
    if (voidRes == (VoidPtr)NULL) {
	clnt_perror(clnt, "mountproc_null_1");
	exit(1);
    } else {
	extern int pdev_Trace;
	if (pdev_Trace) {
	    printf("Null RPC to MOUNTPROG at %s succeeded\n", host);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Nfs_Mount --
 *
 *	Called to mount a NFS filesystem.  This does a mount RPC and returns
 *	the nfs_fh that is returned from the NFS server.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None here, but the server does remember that we've mounted from it.
 *
 *----------------------------------------------------------------------
 */
nfs_fh *
Nfs_Mount(clnt, rfs)
    CLIENT *clnt;
    dirpath rfs;
{
    register fhstatus *statusPtr;
    register nfs_fh *handlePtr;

    statusPtr = mountproc_mnt_1(&rfs, clnt);
    if (statusPtr == (fhstatus *)NULL) {
	clnt_perror(clnt, "mountproc_mnt_1");
	handlePtr = (nfs_fh *)NULL;
    } else if (statusPtr->fhs_status != 0) {
	errno = statusPtr->fhs_status;
	perror(rfs);
	handlePtr = (nfs_fh *)NULL;
    } else {
	/*
	 * The mount.x protocol definition and the nfs_prot.x protocol def
	 * have slightly different ways of defining a "file handle".  Both
	 * are FHSIZE bytes of opaque data, however, so we can safely copy
	 * the "fhandle" returned by the mount in to a "nfs_fh" used
	 * by the nfs routines.
	 */
	handlePtr = (nfs_fh *)malloc(sizeof(nfs_fh));
	bcopy((char *)statusPtr->fhstatus_u.fhs_fhandle, (char *)handlePtr,
		FHSIZE);
    }

    return(handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Nfs_MountDump --
 *
 *	This dumps out what filesystems have been mounted by the client.
 *	To find out, this does an RPC to the server to which we are bound.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Print statements.
 *
 *----------------------------------------------------------------------
 */
void
Nfs_MountDump(clnt)
    CLIENT *clnt;
{
    register mountlist *listPtr;
    VoidPtr voidArg;

    listPtr = mountproc_dump_1(&voidArg, clnt);
    if (listPtr == (mountlist *)NULL) {
	clnt_perror(clnt, "mountproc_dump_1");
	return;
    }
    printf("Mount List\n");
    do {
	printf("%s:%s\n", listPtr->ml_hostname, listPtr->ml_directory);
	listPtr = listPtr->ml_next;
    } while (listPtr);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Nfs_Exports --
 *
 *	This dumps out what filesystems are exported by the server
 *	to which we are bound.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Print statements.
 *
 *----------------------------------------------------------------------
 */
void
Nfs_Exports(clnt, host)
    CLIENT *clnt;
    char *host;
{
    register exports listPtr, *listPtrPtr;
    register groups groupPtr;
    VoidPtr voidArg;

    listPtrPtr = mountproc_export_1(&voidArg, clnt);
    if (listPtrPtr == (exports *)NULL) {
	clnt_perror(clnt, "mountproc_export_1");
	return;
    }
    listPtr = *listPtrPtr;
    printf("Export List of %s\n", host);
    do {
	printf("%s exported to: ", listPtr->ex_dir);
	groupPtr = listPtr->ex_groups;
	while(groupPtr) {
	    printf("%s ", groupPtr->gr_name);
	    groupPtr = groupPtr->gr_next;
	}
	printf("\n");
	listPtr = listPtr->ex_next;
    } while (listPtr);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Nfs_UnmountAll --
 *
 *	Called to unmount all our NFS filesystems..
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Ideally invalidates our fhandles for mounted filesystems,
 *	but probably just erases our name from a list on the server.
 *
 *----------------------------------------------------------------------
 */
void
Nfs_UnmountAll(clnt)
    CLIENT *clnt;
{
    char voidArg;
    VoidPtr voidRes;

    voidRes = mountproc_null_1(&voidArg, clnt);
    if (voidRes == (VoidPtr)NULL) {
	clnt_perror(clnt, "mountproc_umntall_1");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Nfs_Unmount --
 *
 *	Called to unmount a NFS filesystem.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Nuke ourselves from the server's mount list.
 *
 *----------------------------------------------------------------------
 */
void
Nfs_Unmount(clnt, rfs)
    CLIENT *clnt;
    dirpath rfs;
{
    VoidPtr voidRes;

    voidRes = mountproc_umnt_1(&rfs, clnt);
    if (voidRes == (VoidPtr)NULL) {
	clnt_perror(clnt, "mountproc_umnt_1");
    }
}
