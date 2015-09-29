/*
 * hppi.h --
 *
 *	Declarations for commands specific to the Thinking Machines
 *	HIPPI-S and HIPPI-D boards.
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
 * $Header: /sprite/src/lib/include/dev/RCS/hppi.h,v 1.8 91/08/07 15:19:19 elm Exp Locker: elm $ SPRITE (Berkeley)
 */

#ifndef _HPPI
#define _HPPI

/*
 * IOControl calls for the HPPI interface.
 */
#define IOC_HPPI		(20 << 16)

#define IOC_HPPI_LOAD			(IOC_HPPI | 2)
#define IOC_HPPI_GO			(IOC_HPPI | 3)
#define IOC_HPPI_DEBUG			(IOC_HPPI | 4)
#define IOC_HPPI_TRACE			(IOC_HPPI | 5)
#define IOC_HPPI_MAP_THRESHOLD		(IOC_HPPI | 6)
#define IOC_HPPI_ECHO			(IOC_HPPI | 7)
#define IOC_HPPI_RESET			(IOC_HPPI | 8)
#define IOC_HPPI_SRC_RESET		(IOC_HPPI | 9)
#define IOC_HPPI_DST_RESET		(IOC_HPPI | 10)
#define IOC_HPPI_HARD_RESET		(IOC_HPPI | 11)
#define IOC_HPPI_SETUP			(IOC_HPPI | 12)
#define IOC_HPPI_GET_ADAP_INFO		(IOC_HPPI | 13)
#define IOC_HPPI_DIAG			(IOC_HPPI | 14)
#define IOC_HPPI_EXTENDED_DIAG		(IOC_HPPI | 15)
#define IOC_HPPI_START			(IOC_HPPI | 16)
#define IOC_HPPI_COLLECT_STATS		(IOC_HPPI | 17)
#define IOC_HPPI_GET_STATS		(IOC_HPPI | 18)
#define IOC_HPPI_CLEAR_STATS		(IOC_HPPI | 19)
#define IOC_HPPI_SET_FLAGS		(IOC_HPPI | 20)
#define IOC_HPPI_GET_FLAGS		(IOC_HPPI | 21)
#define IOC_HPPI_RESET_FLAGS		(IOC_HPPI | 22)
#define IOC_HPPI_SOURCE			(IOC_HPPI | 23)
#define IOC_HPPI_SINK			(IOC_HPPI | 24)
#define IOC_HPPI_SEND_DGRAM		(IOC_HPPI | 25)
#define IOC_HPPI_ADDRESS		(IOC_HPPI | 26)
#define IOC_HPPI_SRC_ROM_CMD		(IOC_HPPI | 27)
#define IOC_HPPI_DST_ROM_CMD		(IOC_HPPI | 28)
#define IOC_HPPI_WRITE_REG		(IOC_HPPI | 29)
#define IOC_HPPI_READ_REG		(IOC_HPPI | 30)
#define IOC_HPPI_READ_BOARD_WORD	(IOC_HPPI | 31)
#define IOC_HPPI_WRITE_BOARD_WORD	(IOC_HPPI | 32)
#define IOC_HPPI_SET_BOARD_FLAGS	(IOC_HPPI | 33)

typedef unsigned short uint16;
typedef unsigned long uint32;

typedef struct Dev_HppiHdr {
    unsigned char opcode;
    unsigned char serial;
    uint16 magic;
} Dev_HppiCmdHdr;

typedef struct Dev_HppiSetTrace {
    Dev_HppiCmdHdr hdr;
    uint32 level;
} Dev_HppiSetTrace;

typedef struct Dev_HppiDumpTrace {
    Dev_HppiCmdHdr hdr;
} Dev_HppiDumpTrace;

typedef struct Dev_HppiClearTrace {
    Dev_HppiCmdHdr hdr;
} Dev_HppiClearTrace;

typedef struct Dev_HppiOutput {
    Dev_HppiCmdHdr hdr;
    uint32 fifoDataSize;
    uint32 iopDataSize;
} Dev_HppiOutput;

typedef struct Dev_HppiOutputTrace {
    Dev_HppiCmdHdr hdr;
    uint32 fifoDataSize;
    uint32 iopDataSize;
} Dev_HppiOutputTrace;

typedef struct Dev_HppiReset {
    Dev_HppiCmdHdr hdr;
} Dev_HppiReset;

typedef struct Dev_HppiScatterGatherElement {
    uint32 address;
    uint32 size;
} Dev_HppiScatterGatherElement;

typedef struct Dev_HppiScatterGather {
    Dev_HppiCmdHdr hdr;
    uint16 size;
    uint16 tag;
    Dev_HppiScatterGatherElement element[128];
} Dev_HppiScatterGather;

typedef struct Dev_HppiScatterGatherTrace {
    Dev_HppiCmdHdr hdr;
    uint16 size;
    uint16 tag;
    Dev_HppiScatterGatherElement element[128];
} Dev_HppiScatterGatherTrace;

typedef struct Dev_HppiSetup {
    Dev_HppiCmdHdr hdr;
    uint32 reqBlockSize;
    uint32 queueAddress;
    uint32 queueSize;
} Dev_HppiSetup;

typedef struct Dev_HppiDMAInfo {
    unsigned char	cmd;		/* The DMA command.  See below. */
    unsigned char	id;		/* Transaction id. Set to 0. */
    unsigned short	VCref;		/* VC reference. Set to 0. */
    unsigned int	offset;		/* Buffer offset. Set to 0. */
    unsigned int	reference;	/* Unused. */
    unsigned int	length;		/* Length of the DMA (size of the
					 * XRB minus this structure). */
} Dev_HppiDMAInfo;

typedef struct Dev_HppiErrorMsg {
    uint32 magic;
    uint32 errorType;
    uint32 errorInfoLength;		/* # of words that follow which
					 * describe the error that occurred */
} Dev_HppiErrorMsg;

typedef struct Dev_HppiCopyDataMsg {
    uint16 size;
    uint16 magic;
    Dev_HppiDMAInfo dmaWord;
    Dev_HppiScatterGatherElement element[128];
} Dev_HppiCopyDataMsg;

typedef struct Dev_HppiDoDMA {
    uint16 size;
    uint16 magic;
    uint32 vmeAddress;
} Dev_HppiDoDMA;

typedef struct Dev_HppiRegCmd {
    int		board;
    uint32	offset;
    uint32	value;
} Dev_HppiRegCmd;

typedef struct Dev_HppiGo {
    int		board;
    uint32	startAddress;
} Dev_HppiGo;

typedef struct Dev_HppiLoadHdr {
    int		board;
    uint32	startAddress;
    uint32	size;
} Dev_HppiLoadHdr;

typedef struct Dev_HppiMemory {
    Dev_HppiCmdHdr hdr;
    uint32 address;
    uint32 value;
} Dev_HppiMemory;

typedef struct Dev_HppiSetBoardFlags {
    Dev_HppiCmdHdr hdr;
    uint32 flags;
} Dev_HppiSetBoardFlags;

#define DEV_HPPI_FLAG_DEBUG		(1 << 0)
#define DEV_HPPI_FLAG_LOOPBACK_PORT	(1 << 1)
#define DEV_HPPI_FLAG_STANDARD_PORT	(1 << 2)
#define DEV_HPPI_FLAG_ABORT_CONN_ON_ERROR (1 << 3)
#define	DEV_HPPI_FLAG_DEBUG_DATA	(1 << 4)
#define	DEV_HPPI_FLAG_DEBUG_PROC	(1 << 5)
#define DEV_HPPI_FLAG_DEBUG_MISC	(1 << 6)
#define DEV_HPPI_FLAG_XBOARD_SLOT_SHIFT	(7)
#define	DEV_HPPI_FLAG_XBOARD_SLOT_MASK	(7 << DEV_HPPI_FLAG_XBOARD_SLOT_SHIFT)

#define DEV_HPPI_SRC_BOARD		0x1
#define DEV_HPPI_DST_BOARD		0x2
#define DEV_HPPI_IOP_BOARD		0x10

#define	DEV_HPPI_SRC_MAGIC		0xfade
#define DEV_HPPI_DEST_MAGIC		0xcafe
#define DEV_HPPI_ERR_MAGIC		0xdeadfad
#define DEV_HPPI_COPY_MAGIC		0xface
#define DEV_HPPI_DMA_MAGIC		0xaced

#define DEV_HPPI_SET_TRACE		0x00
#define DEV_HPPI_DUMP_TRACE		0x01
#define DEV_HPPI_CLEAR_TRACE		0x02
#define DEV_HPPI_OUTPUT			0x03
#define DEV_HPPI_OUTPUT_TRACE		0x43
#define DEV_HPPI_RESET			0x04
#define DEV_HPPI_SCATTER_GATHER		0x05
#define	DEV_HPPI_SCATTER_GATHER_TRACE	0x45
#define DEV_HPPI_SETUP			0x06
#define DEV_HPPI_READ_MEMORY		0X07
#define DEV_HPPI_WRITE_MEMORY		0x08
#define DEV_HPPI_SET_BOARD_FLAGS	0x09
#define	DEV_HPPI_OUTPUT_TO_IOP		0x0a
#define	DEV_HPPI_INPUT_FROM_IOP		0x0b
#define DEV_HPPI_MAX_ROM_CMD_SIZE	1000
#define DEV_HPPI_LOAD_REQUEST		0
#define DEV_HPPI_GO_REQUEST		0

/*
 * Interrupt bits for the HPPI-D board
 */
#define DEV_HPPI_DST_INTR_CBIAVAIL	0x40	/* CBI cmd register avail */
#define DEV_HPPI_DST_INTR_GEN		0x20	/* generic interrupt */
#define DEV_HPPI_DST_INTR_PARITY	0x10	/* parity error occurred */
#define DEV_HPPI_DST_INTR_ACCESS	0x08	/* access error occurred */
#define DEV_HPPI_DST_INTR_IFIFO_EMPTY	0x04	/* data input fifo empty */
#define DEV_HPPI_DST_INTR_OFIFO_READY	0x02	/* data output fifo has data */
#define DEV_HPPI_DST_INTR_CBIRESPONSE	0x01	/* CBI response reg written */
#define DEV_HPPI_DST_INTR_ALL		(DEV_HPPI_DST_INTR_CBIAVAIL | \
					 DEV_HPPI_DST_INTR_GEN | \
					 DEV_HPPI_DST_INTR_PARITY | \
					 DEV_HPPI_DST_INTR_ACCESS | \
					 DEV_HPPI_DST_INTR_IFIFO_EMPTY | \
					 DEV_HPPI_DST_INTR_OFIFO_READY | \
					 DEV_HPPI_DST_INTR_CBIRESPONSE)

#define DEV_HPPI_OFIFO_HF		(1 << 21)
#define DEV_HPPI_OFIFO_FULL		(1 << 20)
#define DEV_HPPI_OFIFO_EMPTY		(1 << 19)
#define DEV_HPPI_IFIFO_HF		(1 << 18)
#define DEV_HPPI_IFIFO_FULL		(1 << 17)
#define DEV_HPPI_IFIFO_EMPTY		(1 << 16)

#define DEV_HPPI_MAX_ROM_REPLY		0x8

#define DEV_HPPI_IFIFO_DEPTH		1024

/*
 * Bits in the configuration register that enable various
 * interrupts.
 */
#define DEV_HPPI_INTR_ENB_29K		(1 << 28)	/* 29K interrupts */
#define DEV_HPPI_INTR_ENB_PARITY	(1 << 27)	/* SM DOUT parity */
#define DEV_HPPI_INTR_ENB_ACCESS_ERROR	(1 << 26)	/* VMEbus access error
							 * has occurred */
#define DEV_HPPI_INTR_ENB_IFIFO_EMPTY	(1 << 25)	/* input fifo is
							 * empty */
#define DEV_HPPI_INTR_ENB_OFIFO_DATA	(1 << 24)	/* output fifo has
							 * data */
#define DEV_HPPI_INTR_ENB		(1 << 11)	/* This bit must be
							 * set to enable
							 * any interrupts */
#define DEV_HPPI_INTR_ROAK		(1 << 21)	/* interrupts should
							   release on ack */
#define DEV_HPPI_BUS_REQ_SHIFT		19		/* shift left this
							 * many places for
							 * bus req level */
/*
 * These are the ROM commands.
 */

#define	DEV_HPPI_FIRST_DIAG_CODE	0x80
#define	DEV_HPPI_REPORT_PU_STATUS	REQ_ID_FIRST_DIAG_CODE
#define	DEV_HPPI_GET_MEMORY		0x81
#define	DEV_HPPI_PUT_MEMORY		0x82
#define	DEV_HPPI_DO_CHECKSUM		0x83
#define	DEV_HPPI_TEST_CHECKSUM		0x84
#define	DEV_HPPI_FILL_MEMORY		0x85
#define	DEV_HPPI_VERIFY_MEMORY		0x86
#define	DEV_HPPI_START_CODES		0x87
#define	DEV_HPPI_ECHO			0x88
#define	DEV_HPPI_LOOP_WRITE		0x89
#define	DEV_HPPI_LOOP_READ		0x8a
#define	DEV_HPPI_TEST_RAM		0x8b
#define	DEV_HPPI_TEST_AL		0x8c	/* test address lines	*/
#define	DEV_HPPI_BYTE_TEST		0x8d	/* test byte access	*/
#define	DEV_HPPI_MONITOR_REGISTER	0x8e	/* continuously monitor	*/
#define DEV_HPPI_SIZE_SMO_FIFO		0x8f	/* measure SM Data Out Fifo */

#define	DEV_HPPI_END_CONNECTION		0xa0	/* event bit 13		*/
#define	DEV_HPPI_MAKE_CONNECTION	0xa1	/* event bit 12		*/
#define	DEV_HPPI_SEND_BURST		0xa2	/* send test burst data	*/
#define	DEV_HPPI_ENTER_RECEIVE_MODE	0xa3
#define	DEV_HPPI_BURST_DATA		0xa4
#define	DEV_HPPI_HPPIS_SESSION		0xa5	/* do a source session	*/

#define	DEV_HPPI_WRITE_FIFO		0xa6	/* write to fifo	*/
#define	DEV_HPPI_READ_FIFO		0xa7	/* read from fifo	*/

#define	DEV_HPPI_NAK			-1


typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	status;
int	error_code;
int	content [1]; /* up to fifo size */
}PACKET_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_id;
int	content [1]; /* up to fifo size */
}PACKET_COMMAND;
/*
 * Structures for the various ROM commands and responses.
 */
typedef	struct Dev_HppiRomCmdHdr {
    int		length;
    int		commandId;
    int		callerPid;
} Dev_HppiRomCmdHdr;

typedef	struct Dev_HppiRomReplyHdr {
    int		length;
    int		responseId;
    int		callerPid;
    int		status;
    int		errorCode;
} Dev_HppiRomReplyHdr;

#define	ID_SIZE		64
#define	TYPE_ROM	1
#define	TYPE_RAM	2

typedef	struct Dev_HppiRomStatusReport {
    Dev_HppiRomReplyHdr	hdr;
    int			executiveType;
    int			version;
    char		id[ID_SIZE];
} Dev_HppiRomStatusReport;

typedef	struct Dev_HppiRomWrite {
    Dev_HppiRomCmdHdr	hdr;
    int			writeAddress;
    int			writeWords;
    int			data[1];		/* up to fifo size */
} Dev_HppiRomWrite;

typedef	struct Dev_HppiRomWriteReply {
    Dev_HppiRomReplyHdr	hdr;
    int			addressWritten;
    int			wordsWritten;
} Dev_HppiRomWriteReply;

typedef	struct Dev_HppiRomRead {
    Dev_HppiRomCmdHdr	hdr;
    int			readAddress;
    int			readWords;
} Dev_HppiRomRead;

typedef	struct Dev_HppiRomReadReply {
    Dev_HppiRomReplyHdr	hdr;
    int			addressRead;
    int			wordsRead;
    int			dataRead [1];		/* up to fifo size */
} Dev_HppiRomReadReply;

typedef	struct Dev_HppiRomGo {
    Dev_HppiRomCmdHdr	hdr;
    int			startAddress;
} Dev_HppiRomGo;

typedef	struct Dev_HppiRomGoReply {
    Dev_HppiRomReplyHdr	hdr;
    int			startAddress;
} Dev_HppiRomGoReply;

#if 0
typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	address_to_fill;
int	words_to_fill;
int	pattern_to_fill;
int	fill_increment;
}FILL_COMMAND;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	status;
int	error_code;
int	address_filled;
int	words_filled;
int	pattern_filled;
int	fill_increment;
}FILL_RESPONSE;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	address_to_verify;
int  	words_to_verify;
int	pattern_to_verify;
int	verify_increment;
}VERIFY_COMMAND;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	status;
int	error_code;
int	address_verified;
int	words_verified;
int	pattern_verified;
int	verify_increment;
int	bad_data;
}VERIFY_RESPONSE;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	start_address;
int	size_to_check;
}CHECKSUM_COMMAND;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	status;
int	error_code;
int	start_address;
int	size_to_check;
int	checksum;
}CHECKSUM_RESPONSE;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	start_address;
int	size_to_check;
}TEST_CHECKSUM_COMMAND;

typedef	struct
{
int	length;
int	response_id;
int	caller_pid;
int	status;
int	error_code;
int	start_address;
int	size_to_check;
int	checksum;
int	old_checksum;
}TEST_CHECKSUM_RESPONSE;

typedef	struct
{
COMMAND_HEADER	header;
int		address_to_write;
int		write_pattern;
int		delay_between_write;
} WRITE_LOOP_COMMAND;


typedef	struct
{
RESPONSE_HEADER	header;
int		address_to_write;
int		write_pattern;
int		delay_between_write;
} WRITE_LOOP_RESPONSE;

typedef	struct
{
COMMAND_HEADER	header;
int		address_to_read;
int		delay_between_read;
BOOLEAN		send_read_data_to_host;
} READ_LOOP_COMMAND;

typedef	struct
{
RESPONSE_HEADER	header;
int		address_to_read;
int	        delay_between_read;
int		data_just_read;
} READ_LOOP_RESPONSE;

typedef	struct
{
COMMAND_HEADER	header;
int		start_addr;
int		stop_addr;
} TEST_RAM_COMMAND;

typedef	struct
{
RESPONSE_HEADER	header;
int		failed_addr;
int		read_back;
int		expected;
} TEST_RAM_RESPONSE;

typedef	struct
{
COMMAND_HEADER	header;
int		base_address;
int		stop_address;
} TEST_AL_COMMAND;

typedef	struct
{
RESPONSE_HEADER	header;
int		failed_addr;
} TEST_AL_RESPONSE;

typedef	struct
{
COMMAND_HEADER	header;
int		location_to_test;
} BYTE_TEST_COMMAND;

typedef struct
{
RESPONSE_HEADER	header;
int		failed_byte;
int		expected;
int		got;
} BYTE_TEST_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_pid;
int	address_to_monitor;
} MONITOR_COMMAND;

typedef	struct
{
RESPONSE_HEADER	response;
int	address_to_monitor;
int	value_of_register;
} MONITOR_RESPONSE;

typedef struct
{
COMMAND_HEADER header;
int smi_fifo_size;
} SIZE_SMO_FIFO_COMMAND;

typedef struct
{
RESPONSE_HEADER response;
int smo_fifo_size;
} SIZE_SMO_FIFO_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_pid;
int	i_field;
int	usleep_counts;
} MAKE_CONNECTION_COMMAND;

typedef	struct
{
RESPONSE_HEADER	response;
int	hppi_status_register;
} MAKE_CONNECTION_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_pid;
int	priority;
} BREAK_CONNECTION_COMMAND;

typedef	struct
{
RESPONSE_HEADER	response;
int	hppi_status_register;
} BREAK_CONNECTION_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_pid;
int	length_of_short_burst;
int	short_burst_goes_first;
int	full_burst_count;
int	first_data_to_send;
int	increment_factor;
int	ready_timeout_limit;
} SEND_BURST_COMMAND;

typedef	struct
{
RESPONSE_HEADER	response;
int	hppi_status_register;
int	burst_sent;
int	burst_number_when_failure_occured;
} SEND_BURST_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_pid;
int	i_field;
int	length_of_short_burst;
int	short_burst_goes_first;
int	full_burst_count;
int	first_data_to_send;
int	increment_factor;
int	ready_timeout_limit;
} HPPIS_SESSION_COMMAND;

typedef	struct
{
RESPONSE_HEADER	response;
int	hppi_status_register;
int	burst_sent;
int	burst_number_when_failure_occured;
} HPPIS_SESSION_RESPONSE;

typedef	struct
{
int	length;
int	command_id;
int	caller_pid;
} HPPI_RECEIVE_COMMAND;

typedef	struct
{
RESPONSE_HEADER	response;
} HPPI_RECEIVE_RESPONSE;

#define	WORDS_IN_FULL_BURST	256

typedef	struct
{
RESPONSE_HEADER	response;
int	length_of_burst_recieved;
int	burst_data [WORDS_IN_FULL_BURST];
} BURST_DATA;
#endif

#endif /* _HPPI */
