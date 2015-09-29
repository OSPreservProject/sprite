/*
 * td.h --
 *
 *	Declarations of externally-visible things provided by the
 *	terminal driver library.  This includes both the basic tty
 *	driver and an interface between it and a pseudo-device.
 *
 * Copyright 1987, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/td.h,v 1.7 90/09/11 14:39:48 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _TD
#define _TD

#include <sprite.h>
#include <fmt.h>

typedef struct Td_Terminal *Td_Terminal;
typedef struct Td_Pdev *Td_Pdev;

/*
 * Command arguments to Td_ControlRaw:
 */

#define TD_BREAK		1
#define TD_GOT_CARRIER		2
#define TD_LOST_CARRIER		3

/*
 * Command arguments to the device's raw control procedure:
 */

#define TD_RAW_START_BREAK	1
#define TD_RAW_STOP_BREAK	2
#define TD_RAW_SET_DTR		3
#define TD_RAW_CLEAR_DTR	4
#define TD_RAW_SHUTDOWN		5
#define TD_RAW_OUTPUT_READY	6
#define TD_RAW_FLUSH_OUTPUT	7
#define TD_RAW_FLOW_CHARS	8
#define TD_RAW_SET_BAUD_RATE	9
#define TD_RAW_GET_BAUD_RATE	10

/*
 * Data structure passed from the terminal driver to the raw
 * control procedure for TD_RAW_FLOW_CHARS:
 */

typedef struct {
    char stop;			/* Character to stop output to raw
				 * terminal;  -1 means no stop character. */
    char start;			/* Character to restart output to raw
				 * terminal;  -1 means no start character. */
} Td_FlowChars;

/*
 * Data structure passed from the terminal driver to the raw
 * control procedure for TD_RAW_SET_BAUD_RATE and TD_RAW_GET_BAUD_RATE:
 */

typedef struct {
    char ispeed;		/* New input baud rate for terminal
				 * (B9600, etc.). */
    char ospeed;		/* New output baud rate for terminal
				 * (B9600, etc.). */
} Td_BaudRate;

/*
 * Command arguments for the cooked control procedure:
 */

#define TD_COOKED_SIGNAL	21
#define TD_COOKED_READS_OK	22
#define TD_COOKED_WRITES_OK	23

/*
 * Data structure passed from the terminal driver to the cooked
 * control procedure for TD_COOKED_SIGNAL:
 */

typedef struct {
    int sigNum;			/* Signal number to generate. */
    int groupID;		/* ID of controlling process group
				 * for terminal. */
} Td_Signal;

/*
 * Exported procedures:
 */

extern Td_Terminal 
    Td_Create _ARGS_((int bufferSize,
		      int (*cookedProc)_ARGS_((ClientData, int operation,
					       int inBufSize, char *inBuffer,
					       int outBufSize,
					       char *outBuffer)),
		      ClientData cookedData,
		      int (*rawProc)_ARGS_((ClientData, int operation,
					    int inBufSize, char *inBuffer,
					    int outBufSize, char *outBuffer)),
		      ClientData rawData));
extern void Td_Delete _ARGS_((Td_Terminal terminal));
extern int Td_Open _ARGS_((Td_Terminal terminal, int *selectBitsPtr));
extern void Td_Close _ARGS_((Td_Terminal terminal));
extern int Td_GetCooked _ARGS_((Td_Terminal terminal, int pID, int familyID,
    int *numCharsPtr, char *buffer, int *sigNumPtr, int *selectBitsPtr));
extern int Td_PutCooked _ARGS_((Td_Terminal terminal, int *numBytesPtr,
    register char *buffer, int *sigNumPtr, int *selectBitsPtr));
extern int Td_ControlCooked _ARGS_((Td_Terminal terminal, int command,
    Fmt_Format format, int inputSize, char *input, int *outputSizePtr,
    char *output, int *sigNumPtr, int *selectBitsPtr));
extern int Td_GetRaw _ARGS_((Td_Terminal terminal, int numChars,
    register char *buffer));
extern void Td_PutRaw _ARGS_((Td_Terminal terminal,
    int numChars, char *buffer));
extern void Td_ControlRaw _ARGS_((Td_Terminal terminal, int operation));

extern Td_Pdev Td_CreatePdev _ARGS_((char *name, char **realNamePtr,
    Td_Terminal *termPtr, int (*rawProc)(), ClientData clientData));
extern void Td_DeletePdev _ARGS_((Td_Pdev ttyPdev));

#endif /* _TD */
