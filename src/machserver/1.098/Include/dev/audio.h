/*
 * audio.h --
 *
 *	This file defines the device-dependent IOControl calls and related
 *	structures for audio devices, which are used on Sparc's.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/audio.h,v 1.2 89/07/18 17:21:30 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _AUDIO
#define _AUDIO

/*
 * Constants:  these are the IOControl operations defined for audio.
 */

#define IOC_AUDIO (13 << 16)

#define IOC_AUDIO_GETREG	(IOC_AUDIO | 0x1)
#define IOC_AUDIO_SETREG	(IOC_AUDIO | 0x2)
#define IOC_AUDIO_READSTART	(IOC_AUDIO | 0x3)
#define IOC_AUDIO_STOP		(IOC_AUDIO | 0x4)
#define IOC_AUDIO_PAUSE		(IOC_AUDIO | 0x5)
#define IOC_AUDIO_RESUME	(IOC_AUDIO | 0x6)
#define IOC_AUDIO_READQ		(IOC_AUDIO | 0x7)
#define IOC_AUDIO_WRITEQ	(IOC_AUDIO | 0x8)
#define IOC_AUDIO_GETQSIZE	(IOC_AUDIO | 0x9)
#define IOC_AUDIO_SETQSIZE	(IOC_AUDIO | 0xa)
#endif _AUDIO
