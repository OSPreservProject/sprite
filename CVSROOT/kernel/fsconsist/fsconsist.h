/*
 * fsconsist.h --
 *
 *	Declarations for cache consistency routines.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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

#ifndef _FSCONSIST
#define _FSCONSIST

#include "fs.h"
#include "fsio.h"

/*
 * Flags to determine what type of consistency operation is required.
 *
 *    	FSCONSIST_WRITE_BACK_BLOCKS	Write back all dirty blocks.
 *    	FSCONSIST_INVALIDATE_BLOCKS	Invalidate all block in the cache for this
 *				file.  This means that the file is no longer
 *				cacheable.
 *    	FSCONSIST_DELETE_FILE		Delete the file from the local cache and
 *				the file handle table.
 *    	FSCONSIST_CANT_CACHE_NAMED_PIPE	The named pipe is no longer read cacheable
 *				on the client.  This would happen if two
 *				separate clients tried to read the named pipe
 *				at the same time.
 *	FSCONSIST_WRITE_BACK_ATTRS	Write back the cached attributes.
 *	FSCONSIST_DEBUG	Forces machine into debugger
 */

#define	FSCONSIST_WRITE_BACK_BLOCKS		0x01
#define	FSCONSIST_INVALIDATE_BLOCKS		0x02
#define	FSCONSIST_DELETE_FILE			0x04
#define	FSCONSIST_CANT_CACHE_NAMED_PIPE		0x08
#define	FSCONSIST_WRITE_BACK_ATTRS		0x10
#define FSCONSIST_DEBUG		0x100

/*
 * The client use state needed to allow remote client access and to
 * enforce network cache consistency.  A list of state for each client
 * using the file is kept, including the name server itself.  This
 * is used to determine what cache consistency actions to take.
 * There is synchronization over this list between subsequent open
 * operations and the cache consistency actions themselves.
 */

typedef struct Fsconsist_Info {
    Sync_Lock	lock;		/* Monitor lock used to synchronize access
				 * to cache consistency routines and the
				 * consistency list. */
    int		flags;		/* Flags defined in fsCacheConsist.c */
    int		lastWriter;	/* Client id of last client to have it open
				   for write caching. */
    int		openTimeStamp;	/* Generated on the server when the file is
				 * opened.  Checked on clients to catch races
				 * between open replies and consistency
				 * messages */
    Fs_HandleHeader *hdrPtr;	/* Back pointer to handle header needed to
				 * identify the file. */
    List_Links	clientList;	/* List of clients of this file.  Scanned
				 * to determine cachability conflicts */
    List_Links	msgList;	/* List of outstanding cache
				 * consistency messages. */
    Sync_Condition consistDone;	/* Opens block on this condition
				 * until ongoing cache consistency
				 * actions have completed */
    Sync_Condition repliesIn;	/* This condition is notified after
				 * all the clients told to take
				 * consistency actions have replied. */
} Fsconsist_Info;

/*
 * Structure to contain information for each client that is using a file.
 */

typedef struct Fsconsist_ClientInfo {
    List_Links	links;		/* This hangs in a list off the I/O handle */
    int		clientID;	/* The sprite ID of this client. */
    Fsutil_UseCounts use;		/* Usage info for the client.  Used to clean
				 * up summary counts when client crashes. */
    /*
     * The following fields are only used by regular files.
     */
    Boolean	cached;		/* TRUE if the file is cached on this client. */
    int		openTimeStamp;	/* The most recent open of this file. */
    Boolean	locked;		/* TRUE when a pointer is held to this client
				 * list element.  It is not appropriate to
				 * garbage collect the element when set. */
} Fsconsist_ClientInfo;

/*
 * INSERT_CLIENT takes care of initializing a new client list entry.
 */
#define INSERT_CLIENT(clientList, clientPtr, clientID) \
    fs_Stats.object.fileClients++;		\
    clientPtr = mnew(Fsconsist_ClientInfo);	\
    clientPtr->clientID = clientID;		\
    clientPtr->use.ref = 0;			\
    clientPtr->use.write = 0;			\
    clientPtr->use.exec = 0;			\
    clientPtr->openTimeStamp = 0;		\
    clientPtr->cached = FALSE;			\
    clientPtr->locked = FALSE;			\
    List_InitElement((List_Links *)clientPtr);	\
    List_Insert((List_Links *) clientPtr, LIST_ATFRONT(clientList));

/*
 * REMOVE_CLIENT takes care of removing a client list entry.
 */
#define REMOVE_CLIENT(clientPtr) \
	fs_Stats.object.fileClients--;		\
	List_Remove((List_Links *) clientPtr);	\
	free((Address) clientPtr);

/*
 * Client list routines.
 */
extern void		Fsconsist_ClientInit();
extern Fsconsist_ClientInfo	*Fsconsist_IOClientOpen();
extern Boolean		Fsconsist_IOClientClose();
extern Boolean		Fsconsist_IOClientRemoveWriter();
extern void		Fsconsist_IOClientKill();
extern void		Fsconsist_IOClientStatus();


extern void		Fsconsist_ClientScavenge();

/*
 * Cache consistency routines.
 */
extern void		Fsconsist_Init();
extern void		Fsconsist_SyncLockCleanup();
extern ReturnStatus	Fsconsist_FileConsistency();
extern void		Fsconsist_ReopenClient();
extern ReturnStatus	Fsconsist_ReopenConsistency();
extern ReturnStatus	Fsconsist_MigrateConsistency();
extern void		Fsconsist_GetClientAttrs();
extern Boolean		Fsconsist_Close();
extern void		Fsconsist_DeleteLastWriter();
extern void		Fsconsist_ClientRemoveCallback();
extern void		Fsconsist_Kill();
extern void		Fsconsist_GetAllDirtyBlocks();
extern void		Fsconsist_FetchDirtyBlocks();

extern ReturnStatus	Fsconsist_RpcConsist();
extern ReturnStatus	Fsconsist_RpcConsistReply();

extern int Fsconsist_NumClients();
extern Boolean		Fsconsist_IOClientReopen();
extern void Fsconsist_AddClient();

#endif _FSCONSIST
