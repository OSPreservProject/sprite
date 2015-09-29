
/*	@(#)buserr.h 1.3 88/02/08 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * buserr.h
 *
 * definitions for stacked structure showing details of Bus Error
 * or Address Error on M68010 or M68020.
 */

#ifndef fvo_form

/*
 * All the possible values of the Format/Vector Offset format nibble.
 *
 * By definition, not all of these will occur on a single processor chip.
 * The 68020 uses all but the bus10 (68010 bus error) format though.
 */
enum fvo_format {
	fvo_format_short	= 0,
	fvo_format_throwaway	= 1,
	fvo_format_sixword	= 2,
	fvo_format_bus10	= 8,
	fvo_format_coproc	= 9,
	fvo_format_bus20short	= 0xA,
	fvo_format_bus20long	= 0xB,
};

/* 
 * The Format/Vector Offset word appears in all stack frames.
 */
union fvo {
	unsigned short fvo;
	struct {
		enum fvo_format	fvo_p_format:4;
		unsigned short	fvo_p_offset:12;
	} fvo_parts;
};
#define	fvo_form	fvo_parts.fvo_p_format
#define	fvo_off		fvo_parts.fvo_p_offset

/*
 * In the 68010, the BusErrInfo structure starts at the stack pointer upon
 * entry to the bus error routine and proceeds up the stack.  The info described
 * herein will automatically be popped off the stack by the RTE instruction.
 */
struct buserr_stack_10 {
	unsigned short	be10_sr;
	unsigned long	be10_pc;
	union fvo 	be10_fvo;
	unsigned char	be10_ssw_rr:1;	/* Rerun? (0=yes, 1=done by s/ware) */
	unsigned char		:1;
	unsigned char	be10_ssw_if:1;	/* Access was ifetch to IRC. */
	unsigned char	be10_ssw_df:1;	/* Access was data fetch to DIB. */
	unsigned char	be10_ssw_rm:1;	/* Access was read-modify-write */
	unsigned char	be10_ssw_hb:1;	/* High byte=1, low byte=0, iff BY */
	unsigned char	be10_ssw_by:1;	/* Byte.  1=byte, 0=word xfer. */
	unsigned char	be10_ssw_rw:1;	/* Read/Write, 0=Write 1=Read. */
	unsigned char		:5;
	unsigned char	be10_ssw_fcode:3; /* Function codes from access. */
	unsigned long	be10_aob;	/* Address out buffer: failing addr */
	unsigned short	be10_reg_e;
	unsigned short	be10_dob;	/* Data output buffer */
	unsigned short	be10_reg_12;
	unsigned short	be10_dib;	/* Data input buffer */
	unsigned short	be10_reg_16;
	unsigned short	be10_irc;	/* Instruction register */
	unsigned short	be10_upc;	/* Micro program counter */
	unsigned short	be10_reg_1c[15];
};

/*
 * The 68020 generates two kinds of bus error frames, short (between
 * instructions) and long (mid instruction).  Both use the same Special
 * Status Word format and location.  The field names are taken from the
 * 68020 User's Manual, section 6.4.1.
 */
struct ssw20 {
	unsigned char	ssw20_fc:1;	/* Fault on stage C of instr pipe */
	unsigned char	ssw20_fb:1;	/* Fault on stage B of instr pipe */
	unsigned char	ssw20_rc:1;	/* Rerun    stage C of instr pipe */
	unsigned char	ssw20_rb:1;	/* Rerun    stage B of instr pipe */
	unsigned char	:3;
	unsigned char	ssw20_df:1;	/* Fault/Rerun of data cycle */
	unsigned char	ssw20_rm:1;	/* Read-Modify-Write data cycle */
	unsigned char	ssw20_rw:1;	/* Read=1, write=0 data cycle */
	unsigned char	ssw20_siz:2;	/* Size code for data cycle */
	unsigned char	:1;
	unsigned char	ssw20_fcode:3;	/* Function code for data cycle */
};


struct buserr_stack_20_common {
	unsigned short	be20_sr;
	unsigned long	be20_pc;
	union fvo	be20_fvo;
	unsigned short	be20_reg_8[1];
	struct ssw20	be20_ssw;
	unsigned short	be20_instr_pipe_stage_c;
	unsigned short	be20_instr_pipe_stage_b;
	unsigned long	be20_data_fault_addr;
	unsigned short	be20_reg_14[2];
	unsigned long	be20_data_output_buf;
	unsigned short	be20_reg_1c[2];
};


struct buserr_stack_20_extension {
	unsigned short	be20_reg_20[2];
	unsigned long	be20_stage_b_addr;
	unsigned short	be20_reg_28[2];
	unsigned long	be20_data_input_buf;
	unsigned short	be20_reg_30[22];
};

#endif fvo_form
