/***********************************************************************
 *
 * devXbus.h
 *
 *	Include file detailing the exports from the xbus driver module
 *	to the rest of the dev module.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun4.md/devXbus.h,v 9.2 92/10/23 15:04:44 elm Exp $
 *
 ***********************************************************************
 */

#ifndef _DEV_XBUS_
#define	_DEV_XBUS_

#include <dev/xbus.h>

#define	Dev_XbusAddressBase(boardId)	((boardId) << DEV_XBUS_ID_ADDR_SHIFT)

extern ClientData	DevXbusInit _ARGS_((DevConfigController *ctrlPtr));

extern ReturnStatus	DevXbusIOControl _ARGS_((Fs_Device *devicePtr,
				Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern ReturnStatus	DevXbusOpen _ARGS_((Fs_Device *devicePtr, int useFlags,
					    Fs_NotifyToken notifyToken,
					    int *flagsPtr));
extern Boolean		DevXbusIntr _ARGS_((ClientData data));

extern ReturnStatus	Dev_XbusXor _ARGS_((unsigned int boardId,
					    unsigned int destBuf,
					    unsigned int numBufs,
					    unsigned int* bufArrayPtr,
					    unsigned int bufLen,
					    void (*callbackProc)(),
					    ClientData clientData));
extern ReturnStatus	Dev_XbusHippiBuffer _ARGS_((int boardNum,
						    int which,
						    unsigned int size,
						    unsigned int addr));


#endif	/* _DEV_XBUS_ */
