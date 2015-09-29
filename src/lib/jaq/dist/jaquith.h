/*
 * jaquith.h --
 *
 *	DATA and declarations for use by the Jaquith archive system
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      The primary purpose of the DATA statement is to give names to con-
 *      stants; instead of referring to pi as 3.141592653589793 at every
 *      appearance, the variable PI can be given that value with a DATA
 *      statement and used instead of the longer form of the constant.
 *      This also simplifies modifying the program, should the value of
 *      pi change.
 *      -- FORTRAN manual for Xerox Computers
 *
 * $Header: /sprite/lib/forms/RCS/jaquith.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */
 

#ifndef _JAQUITH
#define _JAQUITH

#ifdef OSF1
#define _BSD
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#ifdef sunos
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#ifdef SYSV
#include <strings.h>
#include <utime.h>
#else
#include <string.h>
#endif
#ifdef HASLIMITSH
#include <limits.h>
#else
#define INT_MAX 2147483647
#define SHRT_MAX 64535
#endif
#ifdef HASSTDLIBH
#include <stdlib.h>
#else
extern int	exit();
extern int	free();
extern char *	getenv();
extern char *	malloc();
extern int	qsort();
#endif
#ifdef sprite
#include <bstring.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <varargs.h>
#include <signal.h>
#include <ctype.h>
#include "cfuncproto.h"
#include "regexp.h"

#ifndef HASSTRTOK
extern char *strtok();
#endif

#define DEF_SERVER "covet.CS.Berkeley.EDU"
#define DEF_PORT 10001
#define DEF_MGRSERVER DEF_SERVER
#define DEF_MGRPORT 20001
#define DEF_DEVFILE "/jaquith/dev/devconfig"
#define DEF_VOLFILE "/jaquith/dev/volconfig"
#define DEF_ROBOT "/dev/exbjbox5"
#define DEF_ARCH "default.arch"
#define DEF_ROOT "/jaquith/arch"
#define DEF_FSYNCFREQ 10
#define DEF_JAQLOG "/jaquith/arch/jaq.log"
#define DEF_MGRLOG "/jaquith/arch/jmgr.log"
#define DEF_GET "/jaquith/cmds/jfetch"
#define DEF_PUT "/jaquith/cmds/jupdate"
#define DEF_CLEAN "/jaquith/cmds/jclean"
#define DEF_STATUS "/jaquith/cmds/jquery"
#define DEF_TBUFSIZE (1024*2048)  /* default tbuf size */

#define T_MAXMSGLEN 8192      /* arbitrary constant */
#define T_MAXSTRINGLEN 1024   /* arbitrary constant */
#define T_MAXLABELLEN 80      /* arbitrary constant */
#define T_MAXPATHLEN 255      /* limited by POSIX tar format */
#define T_MAXLINELEN 256      /* for config files */
#define T_VOLSIZEK 4550000    /* 8500 Exabyte P6-120 */
#define T_TAPEUNIT 1024       /* min write unit for device */
#define T_FILEMARKSIZE (48*1024)  /* for Exb 8500 drive */
#define T_BUFSIZE (32*T_TAPEUNIT) /*  socket/disk I/O operation size */
#define T_TARSIZE (32*T_TAPEUNIT)  /* tar tape I/O operation size */
#define T_TBLOCK 512          /* tar unit. not adjustable */

#define ARCHMASTER "mottsmth"

#define ROOT_LOGIN "root"
#define COMMENT_CHAR '#'      /* For config file comments */
#define ARCHLOGFILE "log"
#define MSGVERSION 1          
#define ENV_ARCHIVE_VAR "JARCHIVE"
#define ENV_SERVER_VAR  "JSERVER"
#define ENV_PORT_VAR    "JPORT"

typedef int AuthHandle;       /* for kerberos someday */

#ifdef MEMDEBUG
#define MEM_ALLOC(name,amount) Mem_Alloc(name, __LINE__, amount)
#define MEM_FREE(name, ptr) Mem_Free(name, __LINE__, (char *)ptr)
#define MEM_CONTROL(size, stream, flags, freeMax) \
    Mem_Control(size, stream, flags, freeMax)
#define MEM_REPORT(name, owner, flags) \
    Mem_Report(name, __LINE__, owner, flags)
#else
#define MEM_ALLOC(name,amount) malloc(amount)
#define MEM_FREE(name, ptr) free((char *)ptr)
#define MEM_CONTROL(size, stream, flags, freeMax) {}
#define MEM_REPORT(name, owner, flags) {}
#endif

#ifdef ultrix
#define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)
#endif

#ifdef sprite
#define S_ISADIR(x) ((S_ISDIR(x)) || (S_ISRLNK(x)))
#define S_ISALNK(x) ((S_ISLNK(x)) || (S_ISRLNK(x)))
#define MAKERMTLINK(target, name, flag) Fs_SymLink(target, name, flag)
#else
#define S_ISADIR(x) (((x) & S_IFMT) == S_IFDIR)
#define S_ISALNK(x) (((x) & S_IFMT) == S_IFLNK)
#define S_IFRLNK 0160000
#define MAKERMTLINK(target, name, flag) -1; \
    printf("Don't know how to make remote link: %s\n", name);
#endif

#if ((defined sunos) || (defined OSF1))
typedef struct dirent DirObject;
#else
typedef struct direct DirObject;
#endif

#if (defined(SYSV) && !defined(dynix))
#define GETWD(path, pathLenPtr) getcwd(path, pathLenPtr)
#else
#define GETWD(path, pathLenPtr) getwd(path)
#endif

extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];

#ifdef SYSV
#define STRCHR index
#define STRRCHR rindex
#else
#define STRCHR strchr
#define STRRCHR strrchr
#endif

/*
 * Message layouts
 * 
 * Each message from a T_CLIENT has a header and a body
 * which is a descrimated union.
 */
 
typedef struct T_ReqMsgHdr {
    int version;              /* message version number */
    int cmd;                  /* command type */
    int len;                  /* length of following command data */
    int flags;                /* misc flags */
    AuthHandle ticket;        /* kerberos ticket */
} T_ReqMsgHdr;

typedef struct T_RespMsgHdr {
    int version;              /* message version number */
    int len;                  /* length of following command data */
    int status;               /* command status */
    int errno;                /* system error number */
} T_RespMsgHdr;

/*
 * defines for command types. Jaquith depends on this order.
 */
#define T_MAXCMDS 5

#define T_CMDNULL 0
#define T_CMDLS   1
#define T_CMDPUT  2
#define T_CMDGET  3
#define T_CMDSTAT 4

/*
 * defines for flags word
 */
#define T_LOCAL      0x01     /* Read data from local file */
#define T_LINK       0x02     /* Return link itself */
#define T_FOLLOWLINK 0x04     /* Return link's target */
#define T_FORCE      0x08     /* Force data out to archive */
#define T_SYNC       0x10     /* Write data synchronously to storage */
#define T_NODATA     0x20     /* Return metadata only */
#define T_NEWVOL     0x40     /* Force data to a new volume. */
#define T_COMPRESS   0x80     /* Compress data before storing */

/*
 * defines for status. Interpreted by MakeErrorMsg
 */
#define T_ACTIVE    -2
#define T_FAILURE   -1
#define T_SUCCESS    0
#define T_BADVERSION 1
#define T_BADMSGFMT  2
#define T_BADCMD     3
#define T_NOACCESS   4
#define T_IOFAILED   5
#define T_INDXFAILED 6
#define T_BUFFAILED  7
#define T_ADMFAILED  8
#define T_ROBOTFAILED 9
#define T_NOVOLUME   10
#define T_EXECFAILED 11
#define T_NOARCHIVE  12
#define T_MAXERR   13

typedef struct T_FileStat {
    char *fileName;           /* file name */
    char *linkName;           /* target file name if this is a symlink */
    char *uname;              /* user name */
    char *gname;              /* group name */
    char *abstract;           /* arbitrary user text string */
    char *fileList;           /* List of filenames in a directory */
    time_t vtime;             /* archive date/time */
    time_t atime;             /* access date/time */
    int volId;                /* volume number */
    int filemark;             /* filemark number */
    int tBufId;               /* buffer number */
    int offset;               /* offset of file within buffer */
    int mode;                 /* protection and type */
    int uid;                  /* user id */
    int gid;                  /* group id */
    time_t mtime;             /* modification time */
    int size;                 /* size in bytes. Should be larger */
    dev_t rdev;               /* device id if this is a device */
} T_FileStat;

typedef struct Caller {
    char *userName;           /* User login name */
    char *groupName;          /* User group name */
    char *hostName;           /* Host name */
} Caller;

/**************************** utils ****************************/

#define BAIL_PRINT  0
#define BAIL_PERROR 1
#define BAIL_HERROR 2

extern void  Utils_Bailout       _ARGS_ ((char *msg, int disposition));
extern void  Utils_GetWorkingDir _ARGS_ ((char *path, int *pathLen));
extern char *Utils_MakeFullPath  _ARGS_ ((char *name));
extern int   Utils_CheckName     _ARGS_ ((char *name, int noWhiteSpace));
extern char *Utils_ReadLine      _ARGS_ ((FILE *stream, int stripFlag));
extern int   Utils_StringHashProc _ARGS_ ((Hash_Key key,int keyLen, int size));
extern int   Utils_IntegerHashProc _ARGS_ ((Hash_Key key,int keyLen,int size));
extern int   Utils_CvtInteger    _ARGS_ ((char *string, int low,
				      int high, int *valPtr));
extern char *Utils_GetLoginByUid _ARGS_ ((int uid));
extern char *Utils_GetGroupByGid _ARGS_ ((int gid));
extern int   Utils_GetUidByLogin _ARGS_ ((char *login));
extern int   Utils_GetGidByGroup _ARGS_ ((char *group));
extern int   Utils_SendMail      _ARGS_ ((char *recipient, char *msg,
				      char *type));
extern void  Utils_FreeFileStat  _ARGS_ ((T_FileStat *statInfoPtr, int flag));
extern T_FileStat *Utils_CopyFileStat  _ARGS_ ((T_FileStat *statInfoPtr));
extern int   Utils_GetOk         _ARGS_ ((char *msg));
extern int   Utils_GetInteger    _ARGS_ ((char *msg, int low, int high));

/**************************** mem ****************************/

#define MEM_MAXALLOC 8*1024

extern char *Mem_Alloc   _ARGS_ ((char *callerName, int line, int blockSize));
extern int   Mem_Free    _ARGS_ ((char *callerName, int line, char *blockPtr));
extern char *Mem_Dup     _ARGS_ ((char *callerName, int line, char *string));
extern char *Mem_Cat     _ARGS_ ((char *callerName, int line, int argCnt,...));
extern void  Mem_Report  _ARGS_ ((char *callerName, int line,
				  char *owner, int reportFlags));
extern void  Mem_Control _ARGS_ ((int maxBlockSize, FILE *outStream,
				  int traceFlags, int freeMax));

/*
 * Bit definitions for traceFlags parameter
 */

/*
 * Enable tracing.
 * Default: off
 */
#define TRACEMEM         0x01

/*
 * print msg at each alloc and free call.
 * Default: off
 */
#define TRACECALLS       0x02

/*
 * print statistics for calling routine after each alloc and free call
 * Default: off
 */
#define TRACESTATS       0x04
/*
 * check border around blocks for overwrite after each alloc/free call
 * Default: off (each block is checked when it's freed.)
 */
#define CHECKALLBLKS     0x08

/*
 * Blocks allocated while TRACEMEM == 0 will not be in the database.
 * If later tracing is enabled and one of these blocks is freed,
 * there will be no record of it in the database and JmsFree will abort.
 * A similar problem occurs if only some of the code is instrumented so
 * that only some allocs are traced.
 * 
 * Set IGNOREMISSINGBLK == 1 to ignore such blocks or WARNMISSINGBLK
 * to print a message but continue. The bits are mutually exclusive.
 * Default: issue message and abort (Both bits off).
 */
#define IGNOREMISSINGBLK 0x10
#define WARNMISSINGBLK 0x20

/*
 * for ownerName parameter
 */
#define ALLROUTINES "*"

/*
 * report flags
 */
#define SORTBYREQ   0x01
#define SORTBYADDR  0x02
#define SORTBYOWNER 0x04
#define SORTBYSIZE  0x08

/*
 * Include freed blocks in report.
 * default: off
 */
#define REPORTALLBLKS 0x10


/**************************** str ****************************/

extern int   Str_Match     _ARGS_ ((char *s1, char *s2));
extern char **Str_Split    _ARGS_ ((char *string, char splitChar, 
				    int *partCnt, int elide,
				    char **insidePtrPtr));
extern char *Str_Dup       _ARGS_ ((char *string));
extern char *Str_Cat       _ARGS_ ((int argCnt,...));
extern char *Str_Quote     _ARGS_ ((char *src, char *metachars));
extern int   Str_Unquote   _ARGS_ ((char *src));
extern int   Str_StripDots _ARGS_ ((char *src));


/**************************** tbuf ****************************/

#define MAXTBUFSIZE 1024*1024
#define READING 0
#define WRITING 1

extern int   TBuf_Open        _ARGS_ ((char *path, int name, int *fdPtr,
				       int *hdrFdPtr, int mode));
extern int   TBuf_Close       _ARGS_ ((int tBufFd, int tHdrFd,
				       int termFlag));
extern int   TBuf_Pad         _ARGS_ ((int tBufFd, int len, int blockSize));
extern int   TBuf_WriteTarHdr _ARGS_ ((int fd, T_FileStat *inData));
extern int   TBuf_ParseTarHdr _ARGS_ ((char *inBuf, T_FileStat *outData));
extern int   TBuf_FindFile    _ARGS_ ((int tBufFd, T_FileStat *statInfoPtr));
extern int   TBuf_Delete      _ARGS_ ((char *rootPath, char *arch,
				       int firstTBuf, int lastTBuf));

extern int   TBuf_Terminate   _ARGS_ ((int tBufStream, int tBufSize,
				       int unit));


/**************************** lock ****************************/

typedef struct lock_handle {
    int lockFd;
    int lockMode;
    char lockName[T_MAXPATHLEN];
} Lock_Handle;

#define LOCK_BLOCK 0
#define LOCK_NOBLOCK 1
#define LOCK_RMCONFIRM 1

extern int  Lock_Acquire   _ARGS_ ((char *name, int blockFlag,
				    Lock_Handle *handlePtr));
extern int  Lock_Release   _ARGS_ ((Lock_Handle *handlePtr));
extern void Lock_RemoveAll _ARGS_ ((char *dirPath, int confirmFlag));

/**************************** admin ****************************/

/*
 * Archive's configuration state
 */
typedef struct archConfig {
    char mgrServer[T_MAXSTRINGLEN]; /* name of jmgr server */
    int mgrPort;              /* port where jmgr listens */
    int tBufSize;             /* tbuf target size */
} ArchConfig;

/*
 * Archive's volume state
 */
typedef struct volinfo {
    int volId;                /* current volume id */
    int filemark;             /* Last filemark written. i.e EOD */
    int volSpace;             /* remaining volume space in K bytes */
    int lastTBuf;             /* last tBuf written. Should be 2*filemark*/
} VolInfo;

/*
 * Archive's buffer state
 */
typedef struct metaInfo {
    int tBufSize;             /* Current size of buffer */
    int tHdrSize;             /* Current size of buffer hdr */
    int fileCnt;              /* Number of files in tBuf */
    int tBufPad;              /* Pad on buffer for tar */
} MetaInfo;

/*
 * Volume's owner
 */
typedef struct volowner {
    char *owner;              /* owning archive name */
    int minTBuf;              /* first tbuf on volume */
    int maxTBuf;              /* last tbuf on volume */
} VolOwner;

/*
 * Volume config file entry
 */
typedef struct volConfig {
    int volId;                /* volume id */
    int location;             /* location */
    char volLabel[T_MAXLABELLEN]; /* name */
} VolConfig;

/*
 * Device config file entry
 */
typedef struct devConfig {
    char name[T_MAXPATHLEN];  /* name of device */
    int location;             /* location */
} DevConfig;

extern int   Admin_CheckAuth     _ARGS_ ((char *archName, Caller callerPtr,
					  char *permPtr));
extern int   Admin_AddAuth       _ARGS_ ((char *archName, Caller callerPtr,
					  char perm));
extern int   Admin_ReadArchConfig _ARGS_ ((char *archName,
					   ArchConfig *archConfigPtr));
extern int   Admin_WriteArchConfig _ARGS_ ((char *archName,
					    ArchConfig *archConfigPtr));
extern int   Admin_GetCurTBuf    _ARGS_ ((char *archName, int *tBufIdPtr));
extern int   Admin_SetCurTBuf    _ARGS_ ((char *archName, int tBufId));
extern FILE *Admin_OpenVolInfo   _ARGS_ ((char *archName,
					  Lock_Handle *lockHandlePtr));
extern int   Admin_CloseVolInfo   _ARGS_ ((Lock_Handle *lockHandlePtr,
					   FILE *volStream));
extern int   Admin_ReadVolInfo   _ARGS_ ((FILE *volStream, VolInfo *archPtr));
extern int   Admin_WriteVolInfo  _ARGS_ ((FILE *volStream, VolInfo *archPtr));
extern int   Admin_AvailVolInfo  _ARGS_ ((char *archPath));
extern FILE *Admin_OpenMetaInfo  _ARGS_ ((char *archName, int tBufId));
extern int   Admin_CloseMetaInfo _ARGS_ ((FILE *metaStream));
extern int   Admin_ReadMetaInfo  _ARGS_ ((FILE *metaStream, MetaInfo *archPtr));
extern int   Admin_WriteMetaInfo _ARGS_ ((FILE *metaStream, MetaInfo *archPtr));
extern int   Admin_CvtTBufId     _ARGS_ ((char *archPath, int tBufId,
					  int *volIdPtr, int *blkIdPtr));
extern int   Admin_UpdateLRU     _ARGS_ ((char *rootPath, char *archive,
					  int tBufId));
extern int   Admin_RemoveLRU     _ARGS_ ((char *rootPath, char *archive,
					  int *tBufIdPtr));
extern int   Admin_GetDiskUse    _ARGS_ ((char *filename,
					  int *percentFreePtr,
					  long *blocksFreePtr));
extern VolOwner *Admin_FindVolOwner _ARGS_ ((int volId, char *root,
					     char *archPattern));
extern int   Admin_ReadDevConfig _ARGS_ ((char *root, DevConfig *list,
					  int *cntPtr));
extern int   Admin_WriteDevConfig _ARGS_ ((char *file, DevConfig *list,
					   int cnt));
extern int   Admin_ReadVolConfig _ARGS_ ((char *file, VolConfig *list,
					  int *cntPtr));
extern int   Admin_WriteVolConfig _ARGS_ ((char *file, VolConfig *list,
					   int cnt));
extern int   Admin_GetFreeVol     _ARGS_ ((char *root, int *volIdPtr));
extern int   Admin_PutFreeVol     _ARGS_ ((char *root, int volId));

/**************************** hash ****************************/

#define MAXHASHSIZE 1000

typedef int Hash_ClientData;
typedef char *Hash_Key;

typedef struct Hash_Node {
    Hash_Key key;
    int keyLen;
    Hash_ClientData datum;
} Hash_Node;

typedef struct Hash_Handle {
    char *name;               /* name of table for debugging */
    int tabSize;              /* # entries in hash table */
    Hash_Node *tab;           /* hash table */
    int stringSize;           /* size of string table */
    char *stringTab;          /* ptr to string space */
    int stringUsed;           /* amount of string space in use */
    int (*hashFunc)();        /* hashing function */
    int freeData;             /* 1==give dead data values to free() */
    int tabGrowCnt;           /* statistics */
    float tabFill;
    int stringGrowCnt;
    float stringFill;
    int insertCnt;
    int deleteCnt;
    int lookupCnt;
    int updateCnt;
    int probeCnt;         
} Hash_Handle;

typedef struct Hash_Stat {
    int tabSize;              /* table space */
    int tabGrowCnt;           /* number table reallocations */
    float avgTabFill;         /* avg use at time of reallocation */
    int stringSize;           /* string space */
    int stringGrowCnt;        /* # of string table reallocations */
    float avgStringFill;      /* avg use at time of reallocation */
    int insertCnt;            /* # inserts done */
    int deleteCnt;            /* # deletes done */
    int lookupCnt;            /* # lookups done */
    int updateCnt;            /* # updates done */
    float avgProbeCnt;        /* avg # probes per operation */
} Hash_Stat;

extern Hash_Handle *Hash_Create  _ARGS_ ((char *name, int size,
					  int (*hashFunc)(),
					  int freeData));
extern void     Hash_Destroy _ARGS_ ((Hash_Handle *hashPtr));
extern int      Hash_Insert  _ARGS_ ((Hash_Handle *hashPtr,
				      Hash_Key key,
				      int keyLen,
				      Hash_ClientData datum));
extern int      Hash_Delete  _ARGS_ ((Hash_Handle *hashPtr,
				      Hash_Key key,
				      int keyLen));
extern int      Hash_Lookup  _ARGS_ ((Hash_Handle *hashPtr,
				      Hash_Key key,
				      int keyLen,
				      Hash_ClientData *datumPtr));
extern int      Hash_Update  _ARGS_ ((Hash_Handle *hashPtr,
				      Hash_Key key,
				      int keyLen,
				      Hash_ClientData datum));
extern void     Hash_Iterate _ARGS_ ((Hash_Handle *hashPtr, 
				      int (*func)(),
				      int *callVal));
extern void     Hash_Stats   _ARGS_ ((Hash_Handle *hashPtr,
				      Hash_Stat *statPtr));

#define HASH_ITER_REMOVE_STOP -3
#define HASH_ITER_REMOVE_CONT -2
#define HASH_ITER_STOP -1
#define HASH_ITER_CONTINUE 0

/**************************** queue ****************************/

typedef int Q_ClientData;

typedef struct Q_Node {
    struct Q_Node *link;
    int priority;
    Q_ClientData datum;
} Q_Node;

typedef struct Q_Handle {
    Q_Node *head;
    Q_Node *tail;
    int count;
    int freeData;
    char *name;
} Q_Handle;

extern Q_Handle *   Q_Create  _ARGS_ ((char *qName, int freeData));
extern void         Q_Destroy _ARGS_ ((Q_Handle *qPtr));
extern void         Q_Add     _ARGS_ ((Q_Handle *qPtr,
				       Q_ClientData clientData,
				       int priority));
extern Q_ClientData Q_Remove  _ARGS_ ((Q_Handle *qPtr));
extern int          Q_Count   _ARGS_ ((Q_Handle *qPtr));
extern Q_ClientData Q_Peek    _ARGS_ ((Q_Handle *qPtr));
extern void         Q_Print   _ARGS_ ((Q_Handle *qPtr, FILE *fd));
extern int          Q_Iterate _ARGS_ ((Q_Handle *qPtr,
				       int (*func)
				       (Q_Handle *,
					Q_ClientData *clientdata,
					int *,
					int *),
				       int *callVal));

#define Q_TAILQ (int)INT_MAX
#define Q_HEADQ (int)0

#define Q_ITER_REMOVE_STOP -3
#define Q_ITER_REMOVE_CONT -2
#define Q_ITER_STOP -1
#define Q_ITER_CONTINUE 0


/**************************** dev ****************************/

typedef struct VolStatus {
    int speed;
    int density;
    int remaining;
    int position;
    int writeProtect;
} VolStatus;

extern int   Dev_MoveVolume   _ARGS_ ((int stream, int src, int dest));
extern int   Dev_OpenVolume   _ARGS_ ((char *name, int flags));
extern int   Dev_CloseVolume  _ARGS_ ((int stream));
extern int   Dev_ReadVolume   _ARGS_ ((int stream, char *buf, int cnt));
extern int   Dev_WriteVolume  _ARGS_ ((int stream, char *buf, int cnt));
extern int   Dev_UnloadVolume _ARGS_ ((char *devName));
extern int   Dev_SeekVolume   _ARGS_ ((int stream, int blkId, int absolute));
extern int   Dev_WriteEOF     _ARGS_ ((int stream, int count));
extern int   Dev_ReadVolLabel _ARGS_ ((int stream, int location, char *label));
extern int   Dev_CvtVolLabel  _ARGS_ ((char *label));
extern int   Dev_InitRobot    _ARGS_ ((int stream));
extern int   Dev_GetVolStatus _ARGS_ ((int stream, VolStatus *volStatusPtr));
extern int   Dev_DisplayMsg   _ARGS_ ((int stream, char *msg, int style));
extern int   Dev_OpenDoor     _ARGS_ ((int stream));
extern int   Dev_RemoveVolume _ARGS_ ((int stream, int src));
extern int   Dev_InsertVolume _ARGS_ ((int stream, int dest));

#define DEV_RELATIVE 0
#define DEV_ABSOLUTE 1

/**************************** sock ****************************/

extern int  Sock_SetupSocket  _ARGS_ ((int port, char *server,
				       int dieFlag));
extern int  Sock_SetupEarSocket  _ARGS_ ((int *portPtr));
extern void Sock_SetSocket    _ARGS_ ((int sock, char *bufPtr, int bufSize));
extern int  Sock_ReadSocket   _ARGS_ ((int sock));
extern int  Sock_SendRespHdr  _ARGS_ ((int sock, int, int));
extern void Sock_ReadRespHdr  _ARGS_ ((int sock, T_RespMsgHdr *resp));

extern int  Sock_ReadInteger  _ARGS_ ((int sock, int *intPtr));
extern int  Sock_ReadShort    _ARGS_ ((int sock, short *shortPtr));
extern int  Sock_ReadString   _ARGS_ ((int sock, char **bufPtr,
				       int bufLen));
extern int  Sock_ReadNBytes   _ARGS_ ((int sock, char *buf, int bufLen));

extern int  Sock_WriteInteger _ARGS_ ((int sock, int val));
extern int  Sock_WriteShort   _ARGS_ ((int sock, int shortVal));
extern int  Sock_WriteString  _ARGS_ ((int sock, char *buf, int bufLen));
extern int  Sock_WriteNBytes  _ARGS_ ((int sock, char *buf, int bufLen));
extern char *Sock_PackData    _ARGS_ ((char *fmt, char **objArray,
				       int *packedLenPtr));
extern int  Sock_UnpackData   _ARGS_ ((char *fmt, char *buf,
				       int *countPtr, char **objArray));
extern void Sock_SendReqHdr   _ARGS_ ((int sock, int cmd,
				       AuthHandle ticket, int recurse,
				       char *mail, char *arch,int force));

/**************************** ttime ****************************/

long   Time_Stamp       _ARGS_ (());
time_t Time_GetCurDate  _ARGS_ (());
int    Time_Compare     _ARGS_ ((time_t time1, time_t time2,
				 int dateOnly));
char  *Time_CvtToString _ARGS_ ((time_t *time));
int    getindate        _ARGS_ ((char *datePtr, struct timeb *timebPtr));
int    getindatepair    _ARGS_ ((char *datePtr, struct timeb *timebPtr1,
				 struct timeb *timePtr2));

/**************************** tlog ****************************/

#define LOG_FAIL 0x01
#define LOG_MAJOR 0x02
#define LOG_MINOR 0x04
#define LOG_TRACE 0x08
#define LOG_MAX_DETAIL 4

extern int   Log_Event      _ARGS_ ((char *src, char *msg, int level));
extern int   Log_Open       _ARGS_ ((char *logName));
extern int   Log_Close      _ARGS_ (());
extern void  Log_SetDetail  _ARGS_ ((int detail));
extern int   Log_AtomicEvent _ARGS_ ((char *src, char *msg, char *logName));

/**************************** indx ****************************/

typedef struct QuerySpec {
	int firstVer;
	int lastVer;
	time_t firstDate;
	time_t lastDate;
	char *owner;
	char *group;
	int flags;
	int recurse;
	regexp *compAbstract;
	char *fileName;
} QuerySpec;

/* queryspec.flags definitions */
#define QUERY_MODDATE 0x01

int   Indx_Open     _ARGS_ ((char *pathName, char *openOptions,
			     FILE **indxStrPtr));
int   Indx_Close    _ARGS_ ((FILE *str));
int   Indx_Read     _ARGS_ ((char *fileName, T_FileStat *statInfoPtr,
			     FILE *indxStream));
int   Indx_MakeIndx _ARGS_ ((char *archPath, char *filePath, char *indxPath));
int   Indx_MakeIndxList _ARGS_ ((char *archPath, char *filePath,
				 Q_Handle *indxQueue));
int   Indx_Match    _ARGS_ ((QuerySpec *spec, char *indxPath, char *archPath,
			     char *userName, char *groupName, 
			     int (*receiveProc)(), ReceiveBlk *receiveBlkPtr,
			     int ignoreDir));
int   Indx_ReadIndxEntry _ARGS_ ((FILE *indxStream, T_FileStat *statInfoPtr));
int   Indx_WriteIndxEntry _ARGS_ ((T_FileStat *statInfoPtr, int thdrStream,
			       FILE *indxStream));


/**************************** jmgr ****************************/

#define S_CMDNULL 0
#define S_CMDLOCK 1
#define S_CMDFREE 2
#define S_CMDSTAT 3

typedef struct s_devstat {
    int count;
    char *devName;
    int volId;
    char *hostName;
    char *userName;
} S_DevStat;

typedef struct s_qstat {
    int count;
    int volId;
    char *hostName;
} S_QStat;

#endif /* _JAQUITH */

