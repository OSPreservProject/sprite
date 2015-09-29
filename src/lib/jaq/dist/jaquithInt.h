/*
 * jaquithInt.h --
 *
 *	Declarations for use by the jaquith archive system
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
 * Quote:
 *      "Our life is frittered away by detail... Simplify, simplify."
 *      -- Thoreau
 *
 * $Header: /sprite/lib/forms/RCS/jaquithInt.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */

#ifndef _JAQUITHINT
#define _JAQUITHINT

#define DEF_DETAIL 0x02
#define DEF_CLIENT 32                 /* cannot exceed FD_SETSIZE */
#define DEF_DEFARCH "default"
#define DEF_CONFIG "config"
#define DEF_CHILDDBG 0
#define DEF_DISKLOW "05"
#define DEF_DISKHIGH "10"
/* Allow UCSB sequoia folks and berkeley folks */
#define DEF_NETMASK "{192.150.216,192.150.186,128.32.*}.*" 
#define DEF_TEST 0

#define DEF_DIR_PERM 0755
#define DEF_PERM 0644

#define MAXMSGLEN 80
#define DECINTLEN 10

#define MAXPORT 65535
/* #define NUMPRIVPORTS IPPORT_RESERVED */ /* for security */
#define NUMPRIVPORTS MAXPORT /* for testing */
#define MAXCLIENT FD_SETSIZE

#define NO_SOCKET -1

typedef struct Mask {
    int parts[4];
} Mask;

typedef struct Parms {
    int logDetail;            /* level of logging detail */
    char *logFile;            /* logfile name */
    int port;                 /* port number where server should listen */
    char *root;               /* root of index tree */
    char *defArch;            /* default logical archive name */
    char *config;             /* configuration file */
    char *getExec;            /* path to get program */
    char *putExec;            /* path to put program */
    char *cleanExec;          /* path to clean program */
    char *statExec;           /* path to status program */
    int childDbg;             /* spawn children with -debug flag */
    char *diskLow;            /* Low % of disk that can be used */
    char *diskHigh;           /* High % of disk that can be used */
    char *netMask;            /* String mask for inet addresses */
    int  fsyncFreq;           /* tell jupdate to fsync every N files */
    int  test;                /* test mode. Local connections only */
} Parms;

typedef struct QInfo {
    Q_Handle *qPtr;            /* queue handle */
    char *name;               /* name of archive */
    struct QInfo *link;       /* clients waiting to write this archive */
    struct TClient *activeWriter; /* id of current archive writer */
} QInfo;

typedef struct TClient {
    T_ReqMsgHdr hdr;          /* request header */
    char *msgBuf;             /* request body buffer */
    char *msgPtr;             /* working ptr in to message body */
    int msgLen;               /* message body length */
    int socket;               /* socket number */
    int pid;                  /* process id */
    int indx;                 /* slot in client table */
    time_t date;              /* date we accepted client */
    QInfo *archInfo;          /* ptr to archive info */
    char *hostName;           /* client's host name */
    char *archName;           /* logical archive */
    char *mailName;           /* email name for sending responses */
    char *userName;           /* unvalidated user name */
    char *groupName;          /* unvalidated group name */
} TClient;

#endif /* _JAQUITHINT */

