/*
 * db.h --
 *
 *	Declarations of constants and structures for the db module.
 *
 * Copyright 1987, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/db.h,v 1.7 89/06/23 11:27:53 rab Exp $ SPRITE (Berkeley) 
 */

#ifndef _DB
#define _DB

/*
 * A file can be locked for the entire duration it is open (for example,
 * scanning the entire database for records), each time it is written or
 * read (for example, when updating the shared load average file one might
 * want to leave it open but lock it only during updates), or not at all
 * for example, the file specific to a host that is shared between one process
 * updating it and one process reading it, each atomic).
 */
typedef enum {
    DB_LOCK_OPEN,		/* lock the file on open and keep it locked
				 * until close. */
    DB_LOCK_ACCESS,		/* lock should be obtained each time
				 * the file is accessed. */
    DB_LOCK_NEVER,		/* do not lock the file at all */
    DB_LOCK_READ_MOD_WRITE,	/* lock should be obtained when the file
				 * is read and released when it is
				 * written. */
} Db_LockWhen;

/*
 * Action to take when obtaining a lock: abort immediately, poll for a
 * timeout period, poll forever, or poll but break the lock upon timeout.
 * DB_LOCK_NONE is equivalent to DB_LOCK_NEVER but is used for the single
 * open + read/write + close interface.
 */
typedef enum {
    DB_LOCK_NO_BLOCK,		/* return immediately with error */
    DB_LOCK_POLL,		/* poll the lock; time out if necessary */
    DB_LOCK_WAIT,		/* wait indefinitely */
    DB_LOCK_BREAK,		/* poll, plus break the lock if needed */
    DB_LOCK_NONE,		/* do not lock the file at all */
} Db_LockHow;

/*
 * Information used for repeated scanning through database.
 */

typedef struct {
    int		  streamID;	/* stream identifier for the open file */
    int		  lockType;	/* LOCK_{SH,EX} */
    Db_LockWhen	  lockWhen;	/* See above */
    Db_LockHow 	  lockHow;	/* See above */
    int		  structSize;	/* size of one entry */
    int		  index;	/* current index into file */
    int		  firstRec;  	/* index of first record stored in buffer */
    int		  numBuf;  	/* number of records in buffer */
    char	  *buffer;	/* pointer to buffer containing records */
    char	  *fileName;    /* name of database file, for error messages */
} Db_Handle;

extern int 	Db_Open();
extern int 	Db_Close();

/*
 * The following two routines perform (optional lock)/io/(optional unlock)
 * together.
 */
extern int 	Db_Get();
extern int 	Db_Put();

/*
 * The following two routines perform open/lock/io/unlock/close together.
 */
extern int 	Db_WriteEntry();
extern int 	Db_ReadEntry();

#endif /* _DB */
