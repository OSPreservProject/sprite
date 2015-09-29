/*
 *
 * fsStubs.s --
 *
 *     Stubs for the Fs_ system calls.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/fsStubs.s,v 1.2 89/09/12 11:44:56 douglis Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

SYS_CALL(Fs_AttachDisk,		SYS_FS_ATTACH_DISK)
SYS_CALL(Fs_ChangeDir, 		SYS_FS_CHANGE_DIR)
SYS_CALL(Fs_CheckAccess,	SYS_FS_CHECKACCESS)
SYS_CALL(Fs_Close, 		SYS_FS_CLOSE)
SYS_CALL(Fs_Command, 		SYS_FS_COMMAND)
SYS_CALL(Fs_CreatePipe,		SYS_FS_CREATEPIPE)
SYS_CALL(Fs_GetAttributes, 	SYS_FS_GET_ATTRIBUTES)
SYS_CALL(Fs_GetAttributesID, 	SYS_FS_GET_ATTR_ID)
SYS_CALL(Fs_GetNewID, 		SYS_FS_GET_NEW_ID)
SYS_CALL(Fs_HardLink, 		SYS_FS_HARDLINK)
SYS_CALL(Fs_RawIOControl,		SYS_FS_IO_CONTROL)
SYS_CALL(Fs_MakeDevice,		SYS_FS_MAKE_DEVICE)
SYS_CALL(Fs_MakeDir, 		SYS_FS_MAKE_DIR)
SYS_CALL(Fs_Open, 		SYS_FS_OPEN)
SYS_CALL(Fs_RawRead, 		SYS_FS_READ)
SYS_CALL(Fs_RawWrite, 		SYS_FS_WRITE)
SYS_CALL(Fs_ReadLink, 		SYS_FS_READLINK)
SYS_CALL(Fs_Remove, 		SYS_FS_REMOVE)
SYS_CALL(Fs_RemoveDir, 		SYS_FS_REMOVE_DIR)
SYS_CALL(Fs_Rename, 		SYS_FS_RENAME)
SYS_CALL(Fs_RawSelect, 		SYS_FS_SELECT)
SYS_CALL(Fs_SetAttributes, 	SYS_FS_SET_ATTRIBUTES)
SYS_CALL(Fs_SetAttributesID, 	SYS_FS_SET_ATTR_ID)
SYS_CALL(Fs_SetDefPerm, 	SYS_FS_SET_DEF_PERM)
SYS_CALL(Fs_SymLink, 		SYS_FS_SYMLINK)
/*SYS_CALL(Fs_ReadVector, 	SYS_FS_READVECTOR)*/
/*SYS_CALL(Fs_WriteVector, 	SYS_FS_WRITEVECTOR)*/
SYS_CALL(Fs_WriteBackID, 	SYS_FS_WRITEBACKID)
SYS_CALL(Fs_SetAttr, 		SYS_FS_SET_ATTR_NEW)
SYS_CALL(Fs_SetAttrID, 		SYS_FS_SET_ATTR_ID_NEW)
