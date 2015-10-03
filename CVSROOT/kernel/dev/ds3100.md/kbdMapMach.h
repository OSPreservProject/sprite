/*
 * kbdMapMach.h --
 *
 *	Hardware-dependent declarations of data structures to 
 *	map raw keyboard input to ASCII characters.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _KBDMAPMACH
#define _KBDMAPMACH

/* constants */

/*
 * Ascii values of command keys.
 */

#define KBD_ESC		27
#define KBD_TAB		'\t'
#define KBD_DEL		127
#define KBD_BACKSP	'\b'
#define KBD_RET		'\r'
#define KBD_LF		'\n'

/*
 *  Define "hardware-independent" codes for the control, shift, meta and 
 *  function keys.  Codes start after the last 7-bit ASCII code (0x7F) 
 *  and are assigned in an arbitrary order.
 */

#define KBD_NOKEY	128
#define KBD_UNKNOWN	129

#define KBD_F1		201
#define KBD_F2		202
#define KBD_F3		203
#define KBD_F4		204
#define KBD_F5		205
#define KBD_F6		206
#define KBD_F7		207
#define KBD_F8		208
#define KBD_F9		209
#define KBD_F10		210
#define KBD_F11		211
#define KBD_F12		212
#define KBD_F13		213
#define KBD_F14		214
#define KBD_HELP	215
#define KBD_DO		216
#define KBD_F17		217
#define KBD_F18		218
#define KBD_F19		219
#define KBD_F20		220

#define KBD_FIND	221
#define KBD_INSERT	222
#define KBD_REMOVE	223
#define KBD_SELECT	224
#define KBD_PREVIOUS	225
#define KBD_NEXT	226

#define KBD_KP_ENTER	227
#define KBD_KP_F1	228
#define KBD_KP_F2	229
#define KBD_KP_F3	230
#define KBD_KP_F4	231
#define KBD_LEFT	232
#define KBD_RIGHT	233
#define KBD_DOWN	234
#define KBD_UP		235

#define KBD_CONTROL	236
#define KBD_SHIFT	237
#define KBD_CAPSLOCK	238
#define KBD_ALTERNATE	239

#define KBD_MAX_VALUE	KBD_ALTERNATE

#define KBD_SPECIAL	KBD_CONTROL


extern unsigned short	devKbdPmaxToShiftedAscii[];
extern unsigned short	devKbdPmaxToUnshiftedAscii[];

#endif _KBDMAPMACH
