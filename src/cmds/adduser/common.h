/*
 * common.h --
 *
 *	Shared declarations for adduser and deleteuser.
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
 * $Header: /sprite/src/admin/adduser/RCS/common.h,v 1.2 91/06/04 16:52:10 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _COMMON
#define _COMMON

#ifdef __STDC__
#define _HAS_PROTOTYPES
#define _HAS_VOIDPTR
#endif

#include <cfuncproto.h>
#include <pwd.h>

/* 
 * Don't use const yet; Sprite isn't really converted to use it yet.
 */
#define CONST 

#define BUFFER_SIZE         0x100

#ifdef TEST
#define PASSWD_FILE	"test/passwd"
#define PASSWD_BAK	"test/passwd.addeluser.BAK"
#define PTMP_FILE	"test/ptmp"
#define	MASTER_PASSWD_FILE	"test/master.passwd"
#define MASTER_BAK	"test/master.addeluser.BAK"
#define ALIASES		"test/aliases"
#define ALIASES_TMP	"test/aliases.addeluser.tmp"
#define ALIASES_BAK	"test/aliases.addeluser.BAK"

#else /* !TEST */
#define PASSWD_FILE	_PATH_PASSWD
#define PASSWD_BAK	"/etc/passwd.BAK"
#define PTMP_FILE	_PATH_PTMP
#define	MASTER_PASSWD_FILE	_PATH_MASTERPASSWD
#define MASTER_BAK	"/etc/master.addeluser.BAK"
#define ALIASES		"/sprite/lib/sendmail/aliases"
#define ALIASES_TMP	"/sprite/lib/sendmail/aliases.addeluser.tmp"
#define ALIASES_BAK	"/sprite/lib/sendmail/aliases.addeluser.BAK"
#endif /* !TEST */

/* 
 * This is the directory where everyone's home directory is 
 * registered.  However, instead of actually putting the home directory 
 * here, we put the home directory on a separate partition and put a 
 * symbolic link here.
 */
#ifdef TEST
#define USER_DIR	"test"
#else
#define USER_DIR	"/users"
#endif

extern int checkNumber _ARGS_((char *buf));
extern void getPasswd _ARGS_((char *p));
extern void getNumber _ARGS_((CONST char *prompt, char *buf));
extern char *getShell _ARGS_((void));
extern void getString _ARGS_((CONST char *forbid, CONST char *prompt,
			      char *buf));
extern int makedb _ARGS_((char *file));
extern int raw_getchar _ARGS_((void));
extern int rcsCheckIn _ARGS_((char *file, char *logMsg));
extern int rcsCheckOut _ARGS_((char *file));
extern void SecurityCheck _ARGS_((void));
extern int yes _ARGS_((char *prompt));

#endif /* _COMMON */
