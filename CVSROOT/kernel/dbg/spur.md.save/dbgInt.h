/*
 * dbgInt.h --
 *
 *	Declarations of the network protocol for the SPUR kernel debugger and
 *	downloader.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBGINT
#define _DBGINT

/*
 *  LED values.
 */

#define LED_ERROR  0x00		/* Turn on the leds for an error*/
#define LED_OK     0x0f		/* Turn off the leds for a progress report*/
#define LED_ONE	   ~0x01
#define LED_TWO	   ~0x02
#define LED_THREE  ~0x04
#define LED_FOUR   ~0x08

/*
 * 7 segment display values.
 */
#define GET_STOP_INFO_LED 	1
#define READ_ALL_REGS_LED 	2
#define GET_VERSION_STRING_LED 	3
#define DATA_READ_LED	 	4
#define SET_PID_LED	 	5
#define REBOOT_LED	 	6
#define DATA_WRITE_LED	 	7
#define WRITE_REG_LED		8
#define DIVERT_SYSLOG_LED	9
#define BEGIN_CALL_LED		10
#define END_CALL_LED		11
#define CALL_FUNCTION_LED	12
#define CONTINUE_LED		13
#define SINGLESTEP_LED		14
#define	DETACH_LED		15
#define SET_PROCESSOR_LED	16

#define RUNNING_LED        	0x20
#define REQUEST_LED        	0x22
#define REPLY_LED      		0x23
#define PROCESS_LED		0x24
#define GOT_PACKET_LED		0x25
#define BAD_PACKET_TYPE_LED	0x26
#define EXTRA_PACKET_LED	0x27
#define BAD_PACKET_MAGIC_LED	0x28
#define BAD_PACKET_SIZE_LED	0x29
#define TRAPPED_LED    		0x40    /* or'ed with signal number */


typedef struct {
    Net_EtherHdr	etherHeader;
    Net_IPHeader	ipHeader;
    Net_UDPHeader	udpHeader;
    Dbg_Request		request;
} Dbg_RawRequest;

typedef struct {
    Net_EtherHdr	etherHeader;
    Net_IPHeader	ipHeader;
    Net_UDPHeader	udpHeader;
    Dbg_Reply		reply;
} Dbg_RawReply;

#define	FillReplyField(field, value) {\
    dbgReplyPtr->data.field = (value); \
    replyEndPtr = (char *) (((int) &dbgReplyPtr->data.field) + \
    sizeof(dbgReplyPtr->data.field)); \
    strcpy(replyContents, contentStrings.field);\
}

extern	int	dbgTraceLevel;

extern	Dbg_RawReply		dbgRawReply;
extern	Dbg_RawRequest		dbgRawRequest;
extern	Dbg_Reply		*dbgReplyPtr;
extern	Dbg_Request		*dbgRequestPtr;
extern	Boolean			dbgGotPacket;
#endif _DBGINT
