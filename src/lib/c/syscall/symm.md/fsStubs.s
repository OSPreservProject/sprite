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
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/fsStubs.s,v 1.1 90/01/19 10:18:46 fubar Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

/*
 * For Symmetry, include argument counts, the syscall interface needs
 * to know...
 */	
	
SYS_CALL(3,	Fs_AttachDisk,		SYS_FS_ATTACH_DISK)
SYS_CALL(1,	Fs_ChangeDir, 		SYS_FS_CHANGE_DIR)
SYS_CALL(3,	Fs_CheckAccess,		SYS_FS_CHECKACCESS)
SYS_CALL(1,	Fs_Close, 		SYS_FS_CLOSE)
SYS_CALL(3,	Fs_Command, 		SYS_FS_COMMAND)
SYS_CALL(2,	Fs_CreatePipe,		SYS_FS_CREATEPIPE)
SYS_CALL(3,	Fs_GetAttributes, 	SYS_FS_GET_ATTRIBUTES)
SYS_CALL(2,	Fs_GetAttributesID, 	SYS_FS_GET_ATTR_ID)
SYS_CALL(2,	Fs_GetNewID, 		SYS_FS_GET_NEW_ID)
SYS_CALL(2,	Fs_HardLink, 		SYS_FS_HARDLINK)
SYS_CALL(6,	Fs_RawIOControl, 	SYS_FS_IO_CONTROL)
SYS_CALL(3,	Fs_MakeDevice,		SYS_FS_MAKE_DEVICE)
SYS_CALL(2,	Fs_MakeDir, 		SYS_FS_MAKE_DIR)
SYS_CALL(4,	Fs_Open, 		SYS_FS_OPEN)
SYS_CALL(4,	Fs_RawRead, 		SYS_FS_READ)
SYS_CALL(4,	Fs_RawWrite, 		SYS_FS_WRITE)
SYS_CALL(4,	Fs_ReadLink, 		SYS_FS_READLINK)
SYS_CALL(1,	Fs_Remove, 		SYS_FS_REMOVE)
SYS_CALL(1,	Fs_RemoveDir, 		SYS_FS_REMOVE_DIR)
SYS_CALL(2,	Fs_Rename, 		SYS_FS_RENAME)
SYS_CALL(6,	Fs_RawSelect, 		SYS_FS_SELECT)
SYS_CALL(3,	Fs_SetAttributes, 	SYS_FS_SET_ATTRIBUTES)
SYS_CALL(2,	Fs_SetAttributesID, 	SYS_FS_SET_ATTR_ID)
SYS_CALL(2,	Fs_SetDefPerm, 		SYS_FS_SET_DEF_PERM)
SYS_CALL(3,	Fs_SymLink, 		SYS_FS_SYMLINK)
/*SYS_CALL(4,	Fs_ReadVector, 		SYS_FS_READVECTOR)*/
/*SYS_CALL(4,	Fs_WriteVector, 	SYS_FS_WRITEVECTOR)*/
SYS_CALL(4,	Fs_WriteBackID, 	SYS_FS_WRITEBACKID)
SYS_CALL(4,	Fs_SetAttr, 		SYS_FS_SET_ATTR_NEW)
SYS_CALL(3,	Fs_SetAttrID, 		SYS_FS_SET_ATTR_ID_NEW)
