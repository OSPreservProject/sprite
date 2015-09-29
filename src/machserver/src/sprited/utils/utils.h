/*
 * utils.h --
 *
 *	Declarations for miscellaneous utility routines.
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
 * $Header: /user5/kupfer/spriteserver/src/sprited/utils/RCS/utils.h,v 1.10 92/04/16 11:25:32 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _UTILS
#define _UTILS

#include <cfuncproto.h>
#include <mach.h>
#include <sprite.h>

/* 
 * These are well-known "process IDs" that can be passed to the 
 * unix_task_by_pid UX routine to get back privileged system ports. 
 */
#define UTILS_PRIV_HOST_PID	(-1) /* host privileged request port */
#define UTILS_DEVICE_PID	(-2) /* device server request port */

/* 
 * Note: don't use valloc.  It causes memory leaks.
 */

extern ReturnStatus	Utils_MapMachStatus _ARGS_((kern_return_t kernStatus));
extern vm_prot_t	Utils_MapSpriteProtect _ARGS_((int accessType));
extern int		Utils_UnixPidToTask _ARGS_((int pid, task_t *taskPtr));

#endif /* _UTILS */
