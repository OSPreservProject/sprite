/*
 * devSyslog.h --
 *
 *	Declarations of data and procedures for the system log device.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSYSLOG
#define _DEVSYSLOG

/*
 * Forward Declarations.
 */
extern ReturnStatus Dev_SyslogOpen();
extern ReturnStatus Dev_SyslogRead();
extern ReturnStatus Dev_SyslogWrite();
extern ReturnStatus Dev_SyslogIOControl();
extern ReturnStatus Dev_SyslogClose();
extern ReturnStatus Dev_SyslogSelect();
extern void	    Dev_SyslogPutChar();
extern void	    Dev_SyslogDebug();

#endif _DEVSYSLOG
