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
 * $Header$
 *
 ***********************************************************************
 */

#ifndef _XBUS_
#define	_XBUS_

#include <dev/xbus.h>

extern ClientData	DevXbusInit _ARGS_((DevConfigController *ctrlPtr));

extern ReturnStatus	DevXbusIOControl _ARGS_((Fs_Device *devicePtr,
				Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
extern Boolean		DevXbusIntr _ARGS_((ClientData data));

#endif	/* _XBUS_ */
