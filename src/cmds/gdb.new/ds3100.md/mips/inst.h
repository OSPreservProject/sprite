/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: inst.h,v 1.13.1.1 89/01/21 07:58:08 wje Exp $ */

#ifndef	_SYS_INST_
#define	_SYS_INST_	1

/*
 * inst.h -- instruction format defines
 */

#ifdef LANGUAGE_C
#ifdef MIPSEB
union mips_instruction {
	unsigned word;
	unsigned char byte[4];
	struct {
		unsigned opcode : 6;
		unsigned target : 26;
	} j_format;
	struct {
		unsigned opcode : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		signed simmediate : 16;
	} i_format;
	struct {
		unsigned opcode : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned uimmediate : 16;
	} u_format;
	struct {
		unsigned opcode : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned rd : 5;
		unsigned re : 5;
		unsigned func : 6;
	} r_format;
	struct {
		unsigned opcode : 6;
		unsigned : 1;
		unsigned fmt : 4;
		unsigned rt : 5;
		unsigned rd : 5;
		unsigned re : 5;
		unsigned func : 6;
	} f_format;
};
#endif

#ifdef MIPSEL
union mips_instruction {
	unsigned word;
	unsigned char byte[4];
	struct {
		unsigned target : 26;
		unsigned opcode : 6;
	} j_format;
	struct {
		signed simmediate : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned opcode : 6;
	} i_format;
	struct {
		unsigned uimmediate : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned opcode : 6;
	} u_format;
	struct {
		unsigned func : 6;
		unsigned re : 5;
		unsigned rd : 5;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned opcode : 6;
	} r_format;
	struct {
		unsigned func : 6;
		unsigned re : 5;
		unsigned rd : 5;
		unsigned rt : 5;
		unsigned fmt : 4;
		unsigned : 1;
		unsigned opcode : 6;
	} f_format;
};
#endif

/* major opcodes */
#define spec_op		0x00
#define bcond_op	0x01
#define j_op		0x02
#define jal_op		0x03

#define beq_op		0x04
#define bne_op		0x05
#define blez_op		0x06
#define bgtz_op		0x07

#define addi_op		0x08
#define addiu_op	0x09
#define slti_op		0x0A
#define sltiu_op	0x0B

#define andi_op		0x0C
#define ori_op		0x0D
#define xori_op		0x0E
#define lui_op		0x0F

#define cop0_op		0x10
#define cop1_op		0x11
#define cop2_op		0x12
#define cop3_op		0x13

#define beql_op		0x14
#define bnel_op		0x15
#define blezl_op	0x16
#define bgtzl_op	0x17

#define lb_op		0x20
#define lh_op		0x21
#define lwl_op		0x22
/* #define lcache_op	0x22 */
#define lw_op		0x23

#define lbu_op		0x24
#define lhu_op		0x25
#define lwr_op		0x26
/* #define flush_op	0x26 */
#define ld_op		0x27

#define sb_op		0x28
#define sh_op		0x29
#define swl_op		0x2A
/* #define scache_op	0x2A */
#define sw_op		0x2B

#define swr_op		0x2E
/* #define inval_op	0x2E */
#define sd_op		0x2F

#define ll_op		0x30
#define lwc1_op		0x31
#define lwc2_op		0x32
#define lwc3_op		0x33

#define ldc1_op		0x35
#define ldc2_op		0x36
#define ldc3_op		0x37

#define sc_op		0x38
#define swc1_op		0x39
#define swc2_op		0x3A
#define swc3_op		0x3B

#define sdc1_op		0x3D
#define sdc2_op		0x3E
#define sdc3_op		0x3F

/* func field of spec opcode */
#define sll_op		0x00
#define srl_op		0x02
#define sra_op		0x03

#define sllv_op		0x04
#define srlv_op		0x06
#define srav_op		0x07

#define jr_op		0x08
#define jalr_op		0x09

#define syscall_op	0x0C
#define break_op	0x0D
#define spim_op		0x0E

#define mfhi_op		0x10
#define mthi_op		0x11
#define mflo_op		0x12
#define mtlo_op		0x13

#define mult_op		0x18
#define multu_op	0x19
#define div_op		0x1A
#define divu_op		0x1B

#define add_op		0x20
#define addu_op		0x21
#define sub_op		0x22
#define subu_op		0x23

#define and_op		0x24
#define or_op		0x25
#define xor_op		0x26
#define nor_op		0x27

#define slt_op		0x2A
#define sltu_op		0x2B

#define tge_op		0x30
#define tgeu_op		0x31
#define tlt_op		0x32
#define tltu_op		0x33

#define teq_op		0x34
#define tne_op		0x36

/* rt field of bcond opcodes */
#define bltz_op		0x00
#define bgez_op		0x01
#define bltzl_op	0x02
#define bgezl_op	0x03

#define spimi_op	0x04

#define tgei_op		0x08
#define tgeiu_op	0x09
#define tlti_op		0x0A
#define tltiu_op	0x0B

#define teqi_op		0x0C
#define tnei_op		0x0E

#define bltzal_op	0x10
#define bgezal_op	0x11
#define bltzall_op	0x12
#define bgezall_op	0x13

/* rs field of cop opcodes */
#define bc_op		0x08
#define mfc_op		0x00
#define cfc_op		0x02
#define mtc_op		0x04
#define ctc_op		0x06
#define cop_op		0x10

/* func field of cop0/op opcodes */
#define tlbr_op		0x1
#define tlbwi_op	0x2
#define tlbwr_op	0x6
#define tlbp_op		0x8
#define rfe_op		0x10

/* rs field of cop1 opcode */
#define	s_fmt		0
#define	d_fmt		1
#define	e_fmt		2
#define	w_fmt		4

/* func field of cop1/{s,d,w} opcodes */
#define fadd_op		0x00
#define fsub_op		0x01
#define fmul_op		0x02
#define fdiv_op		0x03

#define fsqrt_op	0x04
#define fabs_op		0x05
#define fmov_op		0x06
#define fneg_op		0x07

#define fround_op	0x0c
#define ftrunc_op	0x0d
#define fceil_op	0x0e
#define ffloor_op	0x0f

#define fcvts_op	0x20
#define fcvtd_op	0x21
#define fcvte_op	0x22
#define fcvtw_op	0x24

#define fcmp_op		0x30


/* compatability entries with past inst.h's */
#define lwc0_op		0x30
#define ldc0_op		0x34
#define swc0_op		0x38
#define sdc0_op		0x3C
#define vcall_op	0x0E
#define fmpy_op		0x02

#endif /* LANGUAGE_C */

#ifdef LANGUAGE_PASCAL

#ifdef MIPSEB
type
    mips_instruction =
      packed record
        case cardinal of
	  0: (
	    word: cardinal;
	    );
	  1: (
	    byte: packed array[0..3] of 0..255;
	    );
	  2: (
	    opcode: 0..63;
	    target: 0..67108863;
	    );
	  3: (
	    opcode3: 0..63;
	    rs: 0..31;
	    rt: 0..31;
	    simmediate: -32768..32767;
	    );
	  4: (
	    opcode4: 0..63;
	    rs4: 0..63;
	    rt4: 0..63;
	    uimmediate: 0..65535;
	    );
	  5: (
	    opcode5: 0..63;
	    rs5: 0..63;
	    rt5: 0..63;
	    rd5: 0..63;
	    re5: 0..63;
	    func: 0..63;
	    );
      end {record};
#endif

#ifdef MIPSEL
type
    mips_instruction =
      packed record
        case cardinal of
	  0: (
	    word: cardinal;
	    );
	  1: (
	    byte: packed array[0..3] of 0..255;
	    );
	  2: (
	    target: 0..67108863;
	    opcode: 0..63;
	    );
	  3: (
	    simmediate: -32768..32767;
	    rt: 0..31;
	    rs: 0..31;
	    opcode3: 0..63;
	    );
	  4: (
	    uimmediate: 0..65535;
	    rt4: 0..63;
	    rs4: 0..63;
	    opcode4: 0..63;
	    );
	  5: (
	    func: 0..63;
	    re5: 0..63;
	    rd5: 0..63;
	    rt5: 0..63;
	    rs5: 0..63;
	    opcode5: 0..63;
	    );
      end {record};
#endif

const
  /* major opcodes */
  spec_op =	16#00;
  bcond_op =	16#01;
  j_op =	16#02;
  jal_op =	16#03;

  beq_op =	16#04;
  bne_op =	16#05;
  blez_op =	16#06;
  bgtz_op =	16#07;

  addi_op =	16#08;
  addiu_op =	16#09;
  slti_op =	16#0A;
  sltiu_op =	16#0B;

  andi_op =	16#0C;
  ori_op =	16#0D;
  xori_op =	16#0E;
  lui_op =	16#0F;

  cop0_op =	16#10;
  cop1_op =	16#11;
  cop2_op =	16#12;
  cop3_op =	16#13;

  beql_op =	16#14;
  bnel_op =	16#15;
  blezl_op =	16#16;
  bgtzl_op =	16#17;

  lb_op =	16#20;
  lh_op =	16#21;
  lwl_op =	16#22;
  lw_op =	16#23;

  lbu_op =	16#24;
  lhu_op =	16#25;
  lwr_op =	16#26;
  ld_op =	16#27;

  sb_op =	16#28;
  sh_op =	16#29;
  swl_op =	16#2A;
  sw_op =	16#2B;

  ll_op =	16#2C;
  sc_op =	16#2D;
  swr_op =	16#2E;
  sd_op =	16#2F;

  lwc0_op =	16#30;
  lwc1_op =	16#31;
  lwc2_op =	16#32;
  lwc3_op =	16#33;

  ldc0_op =	16#34;
  ldc1_op =	16#35;
  ldc2_op =	16#36;
  ldc3_op =	16#37;

  swc0_op =	16#38;
  swc1_op =	16#39;
  swc2_op =	16#3A;
  swc3_op =	16#3B;

  sdc0_op =	16#3C;
  sdc1_op =	16#3D;
  sdc2_op =	16#3E;
  sdc3_op =	16#3F;

  /* func field of spec opcode */
  sll_op =	16#00;
  srl_op =	16#02;
  sra_op =	16#03;

  sllv_op =	16#04;
  srlv_op =	16#06;
  srav_op =	16#07;

  jr_op =	16#08;
  jalr_op =	16#09;

  syscall_op =	16#0C;
  break_op =	16#0D;
  spim_op =	16#0E;

  mfhi_op =	16#10;
  mthi_op =	16#11;
  mflo_op =	16#12;
  mtlo_op =	16#13;

  mult_op =	16#18;
  multu_op =	16#19;
  div_op =	16#1A;
  divu_op =	16#1B;

  add_op =	16#20;
  addu_op =	16#21;
  sub_op =	16#22;
  subu_op =	16#23;

  and_op =	16#24;
  or_op =	16#25;
  xor_op =	16#26;
  nor_op =	16#27;

  slt_op =	16#2A;
  sltu_op =	16#2B;

  tge_op =	16#30;
  tgeu_op =	16#31;
  tlt_op =	16#32;
  tltu_op =	16#33;

  teq_op =	16#34;
  tne_op =	16#36;

  /* rt field of bcond opcodes */
  bltz_op =	16#00;
  bgez_op =	16#01;
  bltzl_op =	16#02;
  bgezl_op =	16#03;

  spimi_op =	16#04;

  tgei_op =	16#08;
  tgeiu_op =	16#09;
  tlti_op =	16#0A;
  tltiu_op =	16#0B;

  teqi_op =	16#0C;
  tnei_op =	16#0E;

  bltzal_op =	16#10;
  bgezal_op =	16#11;
  bltzall_op =	16#12;
  bgezall_op =	16#13;

  /* rs field of cop opcodes */
  bc_op =	16#08;
  mfc_op =	16#00;
  cfc_op =	16#02;
  mtc_op =	16#04;
  ctc_op =	16#06;

  /* func field of cop0/op opcodes */
  tlbr_op =	16#1;
  tlbwi_op =	16#2;
  tlbwr_op =	16#6;
  tlbp_op =	16#8;
  rfe_op =	16#10;

  /* rs field of cop1 opcode */
  s_fmt =	0;
  d_fmt =	1;
  e_fmt =	2;
  w_fmt =	4;

  /* func field of cop1/{s,d,w} opcodes */
  fadd_op =	16#00;
  fsub_op =	16#01;
  fmul_op =	16#02;
  fdiv_op =	16#03;

  fsqrt_op =	16#04;
  fabs_op =	16#05;
  fmov_op =	16#06;
  fneg_op =	16#07;

  fcvts_op =	16#20;
  fcvtd_op =	16#21;
  fcvte_op =	16#22;
  fcvtw_op =	16#24;

  fcmp_op =	16#30;

#endif /* LANGUAGE_PASCAL */

#endif	_SYS_INST_

