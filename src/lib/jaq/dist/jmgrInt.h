/*
 * jmgrInt.h --
 *
 *	Declarations for use by the Jaquith lock manager.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/jmgrInt.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */

#ifndef _JMGRINT
#define _JMGRINT

#define PROGNAME "jmgr"
#define PROGVERSION 1
#define DEF_DETAIL 0x01
#define DEF_CLIENT 32                 /* cannot exceed FD_SETSIZE */
#define DEF_RESET 0
#define DEF_SKIPLABEL 0

#define MAXDEVICES 10
#define MAXMSGLEN 80

#define MAXPORT 65535
#define MAXCLIENT FD_SETSIZE
#define MINCMD 0
#define MAXCMD 2

#define ACTIVE_MSG "Jaquith"
#define READY_MSG " READY"
#define MSG_FLASH 0
#define MSG_STEADY 1
#define MSG_SCROLL 2
#define NOVOLUME -1

typedef struct Parms {
    int logDetail;            /* level of logging detail */
    char *logFile;            /* logfile name */
    int port;                 /* port number where server should listen */
    char *devFile;            /* Device configuration file */
    char *volFile;            /* Volume configuration file */
    char *robot;              /* device name of robot */
    int reset;                /* reset device at startup; 1=yes, do it */
    int skipLabel;            /* Don't check volume label before use */
} Parms;

typedef struct SDev {
    int stream;               /* device's file descriptor */
    int location;             /* device location in jukebox*/
    char *name;               /* name of device */
    int volId;                /* id num of cur volume */
    struct SClient *clientPtr; /* id of current archive user */
    int skipCnt;              /* number of priority skips we've allowed */
} SDev;

typedef struct SClient {
    T_ReqMsgHdr hdr;          /* request header */
    int cmd;                  /* lock or unlock command */
    int volId;                /* device id  */
    int socket;               /* socket number */
    int indx;                 /* slot in client table */
    SDev *devPtr;             /* ptr to device info */
    int retCode;              /* client condition */
    char *hostName;           /* client's host for stat purposes */
    char *userName;           /* client's user id for stat purposes */
} SClient;

/* Global globals */
extern Parms parms;

#endif /* _JMGRINT */
