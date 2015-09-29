#ifndef	_spriteSrv_server_
#define	_spriteSrv_server_

/* Module spriteSrv */

#include <mach/kern_return.h>
#if	(defined(__STDC__) || defined(c_plusplus)) || defined(LINTLIBRARY)
#include <mach/port.h>
#include <mach/message.h>
#endif

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <sprite.h>
#include <spriteTime.h>
#include <sys/types.h>
#include <user/proc.h>
#include <user/fs.h>
#include <user/test.h>
#include <user/sig.h>
#include <user/sys.h>

/* Routine Test_PutDecimalStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_PutDecimalStub
#if	defined(LINTLIBRARY)
    (server, value)
	mach_port_t server;
	int value;
{ return Test_PutDecimalStub(server, value); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int value
);
#else
    ();
#endif
#endif

/* Routine Test_PutHexStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_PutHexStub
#if	defined(LINTLIBRARY)
    (server, value)
	mach_port_t server;
	int value;
{ return Test_PutHexStub(server, value); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int value
);
#else
    ();
#endif
#endif

/* Routine Test_PutOctalStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_PutOctalStub
#if	defined(LINTLIBRARY)
    (server, value)
	mach_port_t server;
	int value;
{ return Test_PutOctalStub(server, value); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int value
);
#else
    ();
#endif
#endif

/* Routine Test_PutMessageStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_PutMessageStub
#if	defined(LINTLIBRARY)
    (server, value)
	mach_port_t server;
	Test_MessageBuffer value;
{ return Test_PutMessageStub(server, value); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Test_MessageBuffer value
);
#else
    ();
#endif
#endif

/* Routine Test_PutStringStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_PutStringStub
#if	defined(LINTLIBRARY)
    (server, buffer, bufLength)
	mach_port_t server;
	vm_address_t buffer;
	int bufLength;
{ return Test_PutStringStub(server, buffer, bufLength); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	vm_address_t buffer,
	int bufLength
);
#else
    ();
#endif
#endif

/* Routine Test_GetStringStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_GetStringStub
#if	defined(LINTLIBRARY)
    (server, buffer, bufLength)
	mach_port_t server;
	vm_address_t buffer;
	int bufLength;
{ return Test_GetStringStub(server, buffer, bufLength); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	vm_address_t buffer,
	int bufLength
);
#else
    ();
#endif
#endif

/* Routine Sys_ShutdownStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sys_ShutdownStub
#if	defined(LINTLIBRARY)
    (server, flags, pendingSig)
	mach_port_t server;
	int flags;
	boolean_t *pendingSig;
{ return Sys_ShutdownStub(server, flags, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int flags,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sys_GetTimeOfDayStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sys_GetTimeOfDayStub
#if	defined(LINTLIBRARY)
    (server, time, localOffset, dstOk, status, pendingSig)
	mach_port_t server;
	Time *time;
	int *localOffset;
	boolean_t *dstOk;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Sys_GetTimeOfDayStub(server, time, localOffset, dstOk, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Time *time,
	int *localOffset,
	boolean_t *dstOk,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sys_StatsStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sys_StatsStub
#if	defined(LINTLIBRARY)
    (server, command, option, argPtr, status, pendingSig)
	mach_port_t server;
	int command;
	int option;
	vm_address_t argPtr;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Sys_StatsStub(server, command, option, argPtr, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int command,
	int option,
	vm_address_t argPtr,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sys_GetMachineInfoStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sys_GetMachineInfoStub
#if	defined(LINTLIBRARY)
    (server, size, buffer, status, pendingSig)
	mach_port_t server;
	int size;
	vm_address_t buffer;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Sys_GetMachineInfoStub(server, size, buffer, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int size,
	vm_address_t buffer,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sys_SetTimeOfDayStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sys_SetTimeOfDayStub
#if	defined(LINTLIBRARY)
    (server, time, localOffset, dstOk, status, pendingSig)
	mach_port_t server;
	Time time;
	int localOffset;
	boolean_t dstOk;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Sys_SetTimeOfDayStub(server, time, localOffset, dstOk, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Time time,
	int localOffset,
	boolean_t dstOk,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Vm_MapFileStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Vm_MapFileStub
#if	defined(LINTLIBRARY)
    (server, fileName, fileNameCnt, readOnly, offset, length, status, buffer, pendingSig)
	mach_port_t server;
	Fs_PathName fileName;
	mach_msg_type_number_t fileNameCnt;
	boolean_t readOnly;
	off_t offset;
	vm_size_t length;
	ReturnStatus *status;
	vm_address_t *buffer;
	boolean_t *pendingSig;
{ return Vm_MapFileStub(server, fileName, fileNameCnt, readOnly, offset, length, status, buffer, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName fileName,
	mach_msg_type_number_t fileNameCnt,
	boolean_t readOnly,
	off_t offset,
	vm_size_t length,
	ReturnStatus *status,
	vm_address_t *buffer,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Vm_CreateVAStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Vm_CreateVAStub
#if	defined(LINTLIBRARY)
    (server, address, bytes, status, pendingSig)
	mach_port_t server;
	vm_address_t address;
	vm_size_t bytes;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Vm_CreateVAStub(server, address, bytes, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	vm_address_t address,
	vm_size_t bytes,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Vm_CmdStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Vm_CmdStub
#if	defined(LINTLIBRARY)
    (server, command, length, arg, status, pendingSig)
	mach_port_t server;
	int command;
	vm_size_t length;
	vm_address_t arg;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Vm_CmdStub(server, command, length, arg, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int command,
	vm_size_t length,
	vm_address_t arg,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Vm_CmdInbandStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Vm_CmdInbandStub
#if	defined(LINTLIBRARY)
    (server, command, option, inBuf, inBufCnt, outBuf, outBufCnt, outBufDealloc, status, pendingSig)
	mach_port_t server;
	int command;
	int option;
	Sys_CharArray inBuf;
	mach_msg_type_number_t inBufCnt;
	vm_offset_t *outBuf;
	mach_msg_type_number_t *outBufCnt;
	boolean_t *outBufDealloc;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Vm_CmdInbandStub(server, command, option, inBuf, inBufCnt, outBuf, outBufCnt, outBufDealloc, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int command,
	int option,
	Sys_CharArray inBuf,
	mach_msg_type_number_t inBufCnt,
	vm_offset_t *outBuf,
	mach_msg_type_number_t *outBufCnt,
	boolean_t *outBufDealloc,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_AccessStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_AccessStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, mode, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	int mode;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_AccessStub(server, pathName, pathNameCnt, mode, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	int mode,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_AttachDiskStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_AttachDiskStub
#if	defined(LINTLIBRARY)
    (server, devName, devNameCnt, localName, localNameCnt, flags, status, pendingSig)
	mach_port_t server;
	Fs_PathName devName;
	mach_msg_type_number_t devNameCnt;
	Fs_PathName localName;
	mach_msg_type_number_t localNameCnt;
	int flags;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_AttachDiskStub(server, devName, devNameCnt, localName, localNameCnt, flags, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName devName,
	mach_msg_type_number_t devNameCnt,
	Fs_PathName localName,
	mach_msg_type_number_t localNameCnt,
	int flags,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_ChangeDirStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_ChangeDirStub
#if	defined(LINTLIBRARY)
    (server, newDir, newDirCnt, status, pendingSig)
	mach_port_t server;
	Fs_PathName newDir;
	mach_msg_type_number_t newDirCnt;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_ChangeDirStub(server, newDir, newDirCnt, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName newDir,
	mach_msg_type_number_t newDirCnt,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_GetAttributesStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_GetAttributesStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, fileOrLink, status, attributes, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	int fileOrLink;
	ReturnStatus *status;
	Fs_Attributes *attributes;
	boolean_t *pendingSig;
{ return Fs_GetAttributesStub(server, pathName, pathNameCnt, fileOrLink, status, attributes, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	int fileOrLink,
	ReturnStatus *status,
	Fs_Attributes *attributes,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_HardLinkStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_HardLinkStub
#if	defined(LINTLIBRARY)
    (server, fileName, fileNameCnt, newName, newNameCnt, status, pendingSig)
	mach_port_t server;
	Fs_PathName fileName;
	mach_msg_type_number_t fileNameCnt;
	Fs_PathName newName;
	mach_msg_type_number_t newNameCnt;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_HardLinkStub(server, fileName, fileNameCnt, newName, newNameCnt, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName fileName,
	mach_msg_type_number_t fileNameCnt,
	Fs_PathName newName,
	mach_msg_type_number_t newNameCnt,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_MakeDeviceStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_MakeDeviceStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, devAttr, permissions, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	Fs_Device devAttr;
	int permissions;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_MakeDeviceStub(server, pathName, pathNameCnt, devAttr, permissions, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	Fs_Device devAttr,
	int permissions,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_MakeDirStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_MakeDirStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, permissions, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	int permissions;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_MakeDirStub(server, pathName, pathNameCnt, permissions, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	int permissions,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_OpenStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_OpenStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, usageFlags, permissions, status, streamID, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	int usageFlags;
	int permissions;
	ReturnStatus *status;
	int *streamID;
	boolean_t *pendingSig;
{ return Fs_OpenStub(server, pathName, pathNameCnt, usageFlags, permissions, status, streamID, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	int usageFlags,
	int permissions,
	ReturnStatus *status,
	int *streamID,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_ReadLinkStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_ReadLinkStub
#if	defined(LINTLIBRARY)
    (server, linkName, linkNameCnt, bufSize, buffer, status, linkLength, pendingSig)
	mach_port_t server;
	Fs_PathName linkName;
	mach_msg_type_number_t linkNameCnt;
	int bufSize;
	vm_address_t buffer;
	ReturnStatus *status;
	int *linkLength;
	boolean_t *pendingSig;
{ return Fs_ReadLinkStub(server, linkName, linkNameCnt, bufSize, buffer, status, linkLength, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName linkName,
	mach_msg_type_number_t linkNameCnt,
	int bufSize,
	vm_address_t buffer,
	ReturnStatus *status,
	int *linkLength,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_RemoveDirStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_RemoveDirStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_RemoveDirStub(server, pathName, pathNameCnt, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_RemoveStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_RemoveStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_RemoveStub(server, pathName, pathNameCnt, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_RenameStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_RenameStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, newName, newNameCnt, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	Fs_PathName newName;
	mach_msg_type_number_t newNameCnt;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_RenameStub(server, pathName, pathNameCnt, newName, newNameCnt, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	Fs_PathName newName,
	mach_msg_type_number_t newNameCnt,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_SetAttrStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_SetAttrStub
#if	defined(LINTLIBRARY)
    (server, pathName, pathNameCnt, fileOrLink, newAttr, flags, status, pendingSig)
	mach_port_t server;
	Fs_PathName pathName;
	mach_msg_type_number_t pathNameCnt;
	int fileOrLink;
	Fs_Attributes newAttr;
	int flags;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_SetAttrStub(server, pathName, pathNameCnt, fileOrLink, newAttr, flags, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName pathName,
	mach_msg_type_number_t pathNameCnt,
	int fileOrLink,
	Fs_Attributes newAttr,
	int flags,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_SymLinkStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_SymLinkStub
#if	defined(LINTLIBRARY)
    (server, targetName, targetNameCnt, linkName, linkNameCnt, remote, status, pendingSig)
	mach_port_t server;
	Fs_PathName targetName;
	mach_msg_type_number_t targetNameCnt;
	Fs_PathName linkName;
	mach_msg_type_number_t linkNameCnt;
	boolean_t remote;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_SymLinkStub(server, targetName, targetNameCnt, linkName, linkNameCnt, remote, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName targetName,
	mach_msg_type_number_t targetNameCnt,
	Fs_PathName linkName,
	mach_msg_type_number_t linkNameCnt,
	boolean_t remote,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_CloseStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_CloseStub
#if	defined(LINTLIBRARY)
    (server, streamID, status, pendingSig)
	mach_port_t server;
	int streamID;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_CloseStub(server, streamID, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int streamID,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_CreatePipeStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_CreatePipeStub
#if	defined(LINTLIBRARY)
    (server, status, inStream, outStream, pendingSig)
	mach_port_t server;
	ReturnStatus *status;
	int *inStream;
	int *outStream;
	boolean_t *pendingSig;
{ return Fs_CreatePipeStub(server, status, inStream, outStream, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	ReturnStatus *status,
	int *inStream,
	int *outStream,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_GetAttributesIDStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_GetAttributesIDStub
#if	defined(LINTLIBRARY)
    (server, stream, status, attr, pendingSig)
	mach_port_t server;
	int stream;
	ReturnStatus *status;
	Fs_Attributes *attr;
	boolean_t *pendingSig;
{ return Fs_GetAttributesIDStub(server, stream, status, attr, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	ReturnStatus *status,
	Fs_Attributes *attr,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_GetNewIDStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_GetNewIDStub
#if	defined(LINTLIBRARY)
    (server, stream, newStream, status, pendingSig)
	mach_port_t server;
	int stream;
	int *newStream;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_GetNewIDStub(server, stream, newStream, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	int *newStream,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_IOControlStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_IOControlStub
#if	defined(LINTLIBRARY)
    (server, stream, command, inBufSize, inBuf, outBufSize, outBuf, status, pendingSig)
	mach_port_t server;
	int stream;
	int command;
	int inBufSize;
	vm_address_t inBuf;
	int outBufSize;
	vm_address_t outBuf;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_IOControlStub(server, stream, command, inBufSize, inBuf, outBufSize, outBuf, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	int command,
	int inBufSize,
	vm_address_t inBuf,
	int outBufSize,
	vm_address_t outBuf,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_ReadStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_ReadStub
#if	defined(LINTLIBRARY)
    (server, stream, bufSize, buffer, status, bytesRead, pendingSig)
	mach_port_t server;
	int stream;
	int bufSize;
	vm_address_t buffer;
	ReturnStatus *status;
	int *bytesRead;
	boolean_t *pendingSig;
{ return Fs_ReadStub(server, stream, bufSize, buffer, status, bytesRead, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	int bufSize,
	vm_address_t buffer,
	ReturnStatus *status,
	int *bytesRead,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_SetAttrIDStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_SetAttrIDStub
#if	defined(LINTLIBRARY)
    (server, stream, newAttr, flags, status, pendingSig)
	mach_port_t server;
	int stream;
	Fs_Attributes newAttr;
	int flags;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_SetAttrIDStub(server, stream, newAttr, flags, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	Fs_Attributes newAttr,
	int flags,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_WriteStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_WriteStub
#if	defined(LINTLIBRARY)
    (server, stream, bufSize, buffer, status, bytesWritten, pendingSig)
	mach_port_t server;
	int stream;
	int bufSize;
	vm_address_t buffer;
	ReturnStatus *status;
	int *bytesWritten;
	boolean_t *pendingSig;
{ return Fs_WriteStub(server, stream, bufSize, buffer, status, bytesWritten, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	int bufSize,
	vm_address_t buffer,
	ReturnStatus *status,
	int *bytesWritten,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_CommandStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_CommandStub
#if	defined(LINTLIBRARY)
    (server, command, bufSize, buffer, status, pendingSig)
	mach_port_t server;
	int command;
	vm_size_t bufSize;
	vm_address_t buffer;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_CommandStub(server, command, bufSize, buffer, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int command,
	vm_size_t bufSize,
	vm_address_t buffer,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_SetDefPermStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_SetDefPermStub
#if	defined(LINTLIBRARY)
    (server, permissions, oldPermissions, pendingSig)
	mach_port_t server;
	int permissions;
	int *oldPermissions;
	boolean_t *pendingSig;
{ return Fs_SetDefPermStub(server, permissions, oldPermissions, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int permissions,
	int *oldPermissions,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_SelectStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_SelectStub
#if	defined(LINTLIBRARY)
    (server, numStreams, useTimeout, timeout, readMask, writeMask, exceptMask, status, numReady, pendingSig)
	mach_port_t server;
	int numStreams;
	boolean_t useTimeout;
	Time *timeout;
	vm_address_t readMask;
	vm_address_t writeMask;
	vm_address_t exceptMask;
	ReturnStatus *status;
	int *numReady;
	boolean_t *pendingSig;
{ return Fs_SelectStub(server, numStreams, useTimeout, timeout, readMask, writeMask, exceptMask, status, numReady, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int numStreams,
	boolean_t useTimeout,
	Time *timeout,
	vm_address_t readMask,
	vm_address_t writeMask,
	vm_address_t exceptMask,
	ReturnStatus *status,
	int *numReady,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Fs_FileWriteBackStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Fs_FileWriteBackStub
#if	defined(LINTLIBRARY)
    (server, stream, firstByte, lastByte, shouldBlock, status, pendingSig)
	mach_port_t server;
	int stream;
	int firstByte;
	int lastByte;
	boolean_t shouldBlock;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Fs_FileWriteBackStub(server, stream, firstByte, lastByte, shouldBlock, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int stream,
	int firstByte,
	int lastByte,
	boolean_t shouldBlock,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Net_InstallRouteStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Net_InstallRouteStub
#if	defined(LINTLIBRARY)
    (server, size, routeInfo, status, pendingSig)
	mach_port_t server;
	int size;
	vm_address_t routeInfo;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Net_InstallRouteStub(server, size, routeInfo, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int size,
	vm_address_t routeInfo,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_ForkStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_ForkStub
#if	defined(LINTLIBRARY)
    (server, childStart, childStateAddr, status, processID, pendingSig)
	mach_port_t server;
	vm_address_t childStart;
	vm_address_t childStateAddr;
	ReturnStatus *status;
	Proc_PID *processID;
	boolean_t *pendingSig;
{ return Proc_ForkStub(server, childStart, childStateAddr, status, processID, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	vm_address_t childStart,
	vm_address_t childStateAddr,
	ReturnStatus *status,
	Proc_PID *processID,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_GetIDsStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_GetIDsStub
#if	defined(LINTLIBRARY)
    (server, pid, parentPid, userID, effUserID, pendingSig)
	mach_port_t server;
	Proc_PID *pid;
	Proc_PID *parentPid;
	int *userID;
	int *effUserID;
	boolean_t *pendingSig;
{ return Proc_GetIDsStub(server, pid, parentPid, userID, effUserID, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Proc_PID *pid,
	Proc_PID *parentPid,
	int *userID,
	int *effUserID,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_OldExecEnvStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_OldExecEnvStub
#if	defined(LINTLIBRARY)
    (server, name, nameCnt, argArray, envArray, debugMe, status, pendingSig)
	mach_port_t server;
	Fs_PathName name;
	mach_msg_type_number_t nameCnt;
	vm_address_t argArray;
	vm_address_t envArray;
	boolean_t debugMe;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_OldExecEnvStub(server, name, nameCnt, argArray, envArray, debugMe, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName name,
	mach_msg_type_number_t nameCnt,
	vm_address_t argArray,
	vm_address_t envArray,
	boolean_t debugMe,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_RawExitStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_RawExitStub
#if	defined(LINTLIBRARY)
    (server, status)
	mach_port_t server;
	int status;
{ return Proc_RawExitStub(server, status); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int status
);
#else
    ();
#endif
#endif

/* Routine Proc_WaitStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_WaitStub
#if	defined(LINTLIBRARY)
    (server, numPids, pidArray, flags, procID, reason, procStatus, subStatus, usage, status, pendingSig)
	mach_port_t server;
	int numPids;
	vm_address_t pidArray;
	int flags;
	Proc_PID *procID;
	int *reason;
	int *procStatus;
	int *subStatus;
	vm_address_t usage;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_WaitStub(server, numPids, pidArray, flags, procID, reason, procStatus, subStatus, usage, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int numPids,
	vm_address_t pidArray,
	int flags,
	Proc_PID *procID,
	int *reason,
	int *procStatus,
	int *subStatus,
	vm_address_t usage,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_DetachStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_DetachStub
#if	defined(LINTLIBRARY)
    (server, detachStatus, pendingSig)
	mach_port_t server;
	int detachStatus;
	boolean_t *pendingSig;
{ return Proc_DetachStub(server, detachStatus, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int detachStatus,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_GetFamilyIDStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_GetFamilyIDStub
#if	defined(LINTLIBRARY)
    (server, pid, status, family, pendingSig)
	mach_port_t server;
	Proc_PID pid;
	ReturnStatus *status;
	Proc_PID *family;
	boolean_t *pendingSig;
{ return Proc_GetFamilyIDStub(server, pid, status, family, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Proc_PID pid,
	ReturnStatus *status,
	Proc_PID *family,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_GetGroupIDsStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_GetGroupIDsStub
#if	defined(LINTLIBRARY)
    (server, numGIDs, gidArray, status, pendingSig)
	mach_port_t server;
	int *numGIDs;
	vm_address_t gidArray;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_GetGroupIDsStub(server, numGIDs, gidArray, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int *numGIDs,
	vm_address_t gidArray,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_GetPCBInfoStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_GetPCBInfoStub
#if	defined(LINTLIBRARY)
    (server, firstPid, lastPid, hostID, infoSize, buffers, argStrings, status, buffersUsed, pendingSig)
	mach_port_t server;
	Proc_PID firstPid;
	Proc_PID lastPid;
	int hostID;
	int infoSize;
	vm_address_t buffers;
	vm_address_t argStrings;
	ReturnStatus *status;
	int *buffersUsed;
	boolean_t *pendingSig;
{ return Proc_GetPCBInfoStub(server, firstPid, lastPid, hostID, infoSize, buffers, argStrings, status, buffersUsed, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Proc_PID firstPid,
	Proc_PID lastPid,
	int hostID,
	int infoSize,
	vm_address_t buffers,
	vm_address_t argStrings,
	ReturnStatus *status,
	int *buffersUsed,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_SetFamilyIDStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_SetFamilyIDStub
#if	defined(LINTLIBRARY)
    (server, pid, family, status, pendingSig)
	mach_port_t server;
	Proc_PID pid;
	Proc_PID family;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_SetFamilyIDStub(server, pid, family, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Proc_PID pid,
	Proc_PID family,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_SetGroupIDsStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_SetGroupIDsStub
#if	defined(LINTLIBRARY)
    (server, numGIDs, gidArray, status, pendingSig)
	mach_port_t server;
	int numGIDs;
	vm_address_t gidArray;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_SetGroupIDsStub(server, numGIDs, gidArray, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int numGIDs,
	vm_address_t gidArray,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_SetIDsStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_SetIDsStub
#if	defined(LINTLIBRARY)
    (server, userID, effUserID, status, pendingSig)
	mach_port_t server;
	int userID;
	int effUserID;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_SetIDsStub(server, userID, effUserID, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int userID,
	int effUserID,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_GetIntervalTimerStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_GetIntervalTimerStub
#if	defined(LINTLIBRARY)
    (server, timerType, status, timer, pendingSig)
	mach_port_t server;
	int timerType;
	ReturnStatus *status;
	Proc_TimerInterval *timer;
	boolean_t *pendingSig;
{ return Proc_GetIntervalTimerStub(server, timerType, status, timer, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int timerType,
	ReturnStatus *status,
	Proc_TimerInterval *timer,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_SetIntervalTimerStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_SetIntervalTimerStub
#if	defined(LINTLIBRARY)
    (server, timerType, newTimer, status, oldTimer, pendingSig)
	mach_port_t server;
	int timerType;
	Proc_TimerInterval newTimer;
	ReturnStatus *status;
	Proc_TimerInterval *oldTimer;
	boolean_t *pendingSig;
{ return Proc_SetIntervalTimerStub(server, timerType, newTimer, status, oldTimer, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int timerType,
	Proc_TimerInterval newTimer,
	ReturnStatus *status,
	Proc_TimerInterval *oldTimer,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_GetHostIDsStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_GetHostIDsStub
#if	defined(LINTLIBRARY)
    (server, virtHost, physHost, pendingSig)
	mach_port_t server;
	int *virtHost;
	int *physHost;
	boolean_t *pendingSig;
{ return Proc_GetHostIDsStub(server, virtHost, physHost, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int *virtHost,
	int *physHost,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Proc_ExecEnvStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Proc_ExecEnvStub
#if	defined(LINTLIBRARY)
    (server, name, nameCnt, argTable, argTableCnt, argStrings, argStringsCnt, envTable, envTableCnt, envStrings, envStringsCnt, debugMe, status, pendingSig)
	mach_port_t server;
	Fs_PathName name;
	mach_msg_type_number_t nameCnt;
	Proc_OffsetTable argTable;
	mach_msg_type_number_t argTableCnt;
	Proc_Strings argStrings;
	mach_msg_type_number_t argStringsCnt;
	Proc_OffsetTable envTable;
	mach_msg_type_number_t envTableCnt;
	Proc_Strings envStrings;
	mach_msg_type_number_t envStringsCnt;
	boolean_t debugMe;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Proc_ExecEnvStub(server, name, nameCnt, argTable, argTableCnt, argStrings, argStringsCnt, envTable, envTableCnt, envStrings, envStringsCnt, debugMe, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	Fs_PathName name,
	mach_msg_type_number_t nameCnt,
	Proc_OffsetTable argTable,
	mach_msg_type_number_t argTableCnt,
	Proc_Strings argStrings,
	mach_msg_type_number_t argStringsCnt,
	Proc_OffsetTable envTable,
	mach_msg_type_number_t envTableCnt,
	Proc_Strings envStrings,
	mach_msg_type_number_t envStringsCnt,
	boolean_t debugMe,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sig_PauseStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sig_PauseStub
#if	defined(LINTLIBRARY)
    (server, holdMask, status, pendingSig)
	mach_port_t server;
	int holdMask;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Sig_PauseStub(server, holdMask, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int holdMask,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sig_SendStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sig_SendStub
#if	defined(LINTLIBRARY)
    (server, sigNum, id, isFamily, status, pendingSig)
	mach_port_t server;
	int sigNum;
	Proc_PID id;
	boolean_t isFamily;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Sig_SendStub(server, sigNum, id, isFamily, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int sigNum,
	Proc_PID id,
	boolean_t isFamily,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sig_SetActionStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sig_SetActionStub
#if	defined(LINTLIBRARY)
    (server, sigNum, newAction, sigtramp, status, oldAction, pendingSig)
	mach_port_t server;
	int sigNum;
	Sig_Action newAction;
	vm_address_t sigtramp;
	ReturnStatus *status;
	Sig_Action *oldAction;
	boolean_t *pendingSig;
{ return Sig_SetActionStub(server, sigNum, newAction, sigtramp, status, oldAction, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int sigNum,
	Sig_Action newAction,
	vm_address_t sigtramp,
	ReturnStatus *status,
	Sig_Action *oldAction,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* Routine Sig_SetHoldMaskStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sig_SetHoldMaskStub
#if	defined(LINTLIBRARY)
    (server, newMask, oldMask, pendingSig)
	mach_port_t server;
	int newMask;
	int *oldMask;
	boolean_t *pendingSig;
{ return Sig_SetHoldMaskStub(server, newMask, oldMask, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int newMask,
	int *oldMask,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

/* SimpleRoutine Sig_ReturnStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sig_ReturnStub
#if	defined(LINTLIBRARY)
    (server, sigContext)
	mach_port_t server;
	vm_address_t sigContext;
{ return Sig_ReturnStub(server, sigContext); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	vm_address_t sigContext
);
#else
    ();
#endif
#endif

/* Routine Sig_GetSignalStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Sig_GetSignalStub
#if	defined(LINTLIBRARY)
    (server, status, handler, sigNum, sigCode, sigContext, sigAddr)
	mach_port_t server;
	ReturnStatus *status;
	vm_address_t *handler;
	int *sigNum;
	int *sigCode;
	vm_address_t sigContext;
	vm_address_t *sigAddr;
{ return Sig_GetSignalStub(server, status, handler, sigNum, sigCode, sigContext, sigAddr); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	ReturnStatus *status,
	vm_address_t *handler,
	int *sigNum,
	int *sigCode,
	vm_address_t sigContext,
	vm_address_t *sigAddr
);
#else
    ();
#endif
#endif

/* Routine Test_PutTimeStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_PutTimeStub
#if	defined(LINTLIBRARY)
    (server, time, overwrite)
	mach_port_t server;
	int time;
	boolean_t overwrite;
{ return Test_PutTimeStub(server, time, overwrite); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int time,
	boolean_t overwrite
);
#else
    ();
#endif
#endif

/* Routine Test_MemCheckStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_MemCheckStub
#if	defined(LINTLIBRARY)
    (server)
	mach_port_t server;
{ return Test_MemCheckStub(server); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server
);
#else
    ();
#endif
#endif

/* Routine Test_Return1Stub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_Return1Stub
#if	defined(LINTLIBRARY)
    (server)
	mach_port_t server;
{ return Test_Return1Stub(server); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server
);
#else
    ();
#endif
#endif

/* Routine Test_Return2Stub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_Return2Stub
#if	defined(LINTLIBRARY)
    (server)
	mach_port_t server;
{ return Test_Return2Stub(server); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server
);
#else
    ();
#endif
#endif

/* Routine Test_RpcStub */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t Test_RpcStub
#if	defined(LINTLIBRARY)
    (server, command, args, status, pendingSig)
	mach_port_t server;
	int command;
	vm_address_t args;
	ReturnStatus *status;
	boolean_t *pendingSig;
{ return Test_RpcStub(server, command, args, status, pendingSig); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	mach_port_t server,
	int command,
	vm_address_t args,
	ReturnStatus *status,
	boolean_t *pendingSig
);
#else
    ();
#endif
#endif

#endif	_spriteSrv_server_
