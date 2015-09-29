/*
 * ulog.h --
 *
 *	Declarations of structures, constants, and procedures to manage
 *	the global database of user login/logouts.
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
 * $Header: /sprite/src/lib/include/RCS/ulog.h,v 1.6 89/07/14 09:10:11 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _ULOG
#define _ULOG

/*
 * It's not clear how to handle ports.  We could use a shared file with indexes
 * corresponding to "ttys", but there are other issues (like the fact that
 * the kernel only maintains idle time measurements for the console, not
 * for rlogins).  Therefore, define ULOG_LOC_CONSOLE as a single index
 * that distinguishes local logins from remote, and use the rloginN numbers
 * as non-zero indexes for the remote logins.
 */

#define ULOG_LOC_CONSOLE 0


/*
 * Since each host has a region in the userLog file allocated to it,
 * the number of entries for each host is (unfortunately) fixed.  This is
 * done for simplicity.  The routines may later be changed to lock the
 * file and find a free entry at any location rather than basing the location
 * on hostID and portID.
 */

#define ULOG_MAX_PORTS 10

/*
 * Define the maximum length of a location entry.
 */
#define ULOG_LOC_LENGTH 33

/*
 * Define the database files used for storing the per-user and per-host/port
 * logs.
 */

#define LASTLOG_FILE_NAME "/sprite/admin/lastLog"
#define ULOG_FILE_NAME "/sprite/admin/userLog"

/*
 * Define the structure used internally by user programs.
 */
typedef struct {
    int	uid;		/* user identifier */
    int hostID;		/* host for which data is valid */
    int portID;		/* port within host */
    int updated;	/* time updated (in seconds since 1/1/70); 0 if
			   invalid */
    char location[ULOG_LOC_LENGTH];	/* location of user */
} Ulog_Data;

extern int		Ulog_RecordLogin();
extern int		Ulog_RecordLogout();
extern Ulog_Data *	Ulog_LastLogin();
extern int		Ulog_ClearLogins();
#endif /* _ULOG */
