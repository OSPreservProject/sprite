/*
 * dbgRs232.h --
 *
 *     Exported types and procedure headers for the rs232 driver.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBGRS232
#define _DBGRS232

/*
 * Which channel to use.
 */

typedef enum {
    DBG_RS232_CHANNELA, 
    DBG_RS232_CHANNELB
} Dbg_Rs232Channel;

extern	void		DbgRs232Init();
extern	char 		DbgRs232ReadChar();
extern	void 		DbgRs232WriteChar();

#endif _DBGRS232
