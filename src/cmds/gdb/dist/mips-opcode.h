/* Mips opcde list for GDB, the GNU debugger.
   Copyright (C) 1989 Free Software Foundation, Inc.
   Contributed by Nobuyuki Hikichi(hikichi@sra.junet)
   Made to work for little-endian machines, and debugged
   by Per Bothner (bothner@cs.wisc.edu).
   Many fixes contributed by Frank Yellin (fy@lucid.com).

This file is part of GDB.

GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef BITS_BIG_ENDIAN
#define BIT_FIELDS_2(a,b) a;b;
#define BIT_FIELDS_4(a,b,c,d) a;b;c;d;
#define BIT_FIELDS_6(a,b,c,d,e,f) a;b;c;d;e;f;
#else
#define BIT_FIELDS_2(a,b) b;a;
#define BIT_FIELDS_4(a,b,c,d) d;c;b;a;
#define BIT_FIELDS_6(a,b,c,d,e,f) f;e;d;c;b;a;
#endif

struct op_i_fmt
{
BIT_FIELDS_4(
  unsigned op : 6,
  unsigned rs : 5,
  unsigned rt : 5,
  unsigned immediate : 16)
};

struct op_j_fmt
{
BIT_FIELDS_2(
  unsigned op : 6,
  unsigned target : 26)
};

struct op_r_fmt
{
BIT_FIELDS_6(
  unsigned op : 6,
  unsigned rs : 5,
  unsigned rt : 5,
  unsigned rd : 5,
  unsigned shamt : 5,
  unsigned funct : 6)
};


struct fop_i_fmt
{
BIT_FIELDS_4(
  unsigned op : 6,
  unsigned rs : 5,
  unsigned rt : 5,
  unsigned immediate : 16)
};

struct op_b_fmt
{
BIT_FIELDS_4(
  unsigned op : 6,
  unsigned rs : 5,
  unsigned rt : 5,
  short delta : 16)
};

struct fop_r_fmt
{
BIT_FIELDS_6(
  unsigned op : 6,
  unsigned fmt : 5,
  unsigned ft : 5,
  unsigned fs : 5,
  unsigned fd : 5,
  unsigned funct : 6)
};

struct mips_opcode
{
  char *name;
  unsigned long opcode;
  unsigned long match;
  char *args;
};

/* args format;

   "s" rs: source register specifier
   "t" rt: target register
   "i" immediate
   "a" target address
   "c" branch condition
   "d" rd: destination register specifier
   "h" shamt: shift amount
   "f" funct: function field

  for fpu
   "S" fs source 1 register
   "T" ft source 2 register
   "D" destination register
*/

#define one(x) (x << 26)
#define op_func(x, y) ((x << 26) | y)
#define op_cond(x, y) ((x << 26) | (y << 16))
#define op_rs_func(x, y, z) ((x << 26) | (y << 21) | z)
#define op_rs_b11(x, y, z) ((x << 26) | (y << 21) | z)
#define op_o16(x, y) ((x << 26) | (y << 16))
#define op_bc(x, y, z) ((x << 26) | (y << 21) | (z << 16))

struct mips_opcode mips_opcodes[] = 
{
/* These first opcodes are special cases of the ones in the comments */
  {"nop",	0,		0xffffffff,	     /*li*/	""},
  {"li",	op_bc(9,0,0),	op_bc(0x3f,31,0),    /*addiu*/	"t,j"},
  {"b",		one(4),		0xffff0000,	     /*beq*/	"b"},
  {"move",	op_func(0, 33),	op_cond(0x3f,31)|0x7ff,/*addu*/	"d,s"},

  {"sll",	op_func(0, 0),	op_func(0x3f, 0x3f),		"d,t,h"},
  {"srl",	op_func(0, 2),	op_func(0x3f, 0x3f),		"d,t,h"},
  {"sra",	op_func(0, 3),	op_func(0x3f, 0x3f),		"d,t,h"},
  {"sllv",	op_func(0, 4),	op_func(0x3f, 0x7ff),		"d,t,s"},
  {"srlv",	op_func(0, 6),	op_func(0x3f, 0x7ff),		"d,t,s"},
  {"srav",	op_func(0, 7),	op_func(0x3f, 0x7ff),		"d,t,s"},
  {"jr",	op_func(0, 8),	op_func(0x3f, 0x1fffff),	"s"},
  {"jalr",	op_func(0, 9),	op_func(0x3f, 0x1f07ff),	"d,s"},
  {"syscall",	op_func(0, 12),	op_func(0x3f, 0x3f),		""},
  {"break",	op_func(0, 13),	op_func(0x3f, 0x3f),		""},
  {"mfhi",      op_func(0, 16), op_func(0x3f, 0x03ff07ff),      "d"},
  {"mthi",      op_func(0, 17), op_func(0x3f, 0x1fffff),        "s"},
  {"mflo",      op_func(0, 18), op_func(0x3f, 0x03ff07ff),      "d"},
  {"mtlo",      op_func(0, 19), op_func(0x3f, 0x1fffff),        "s"},
  {"mult",	op_func(0, 24),	op_func(0x3f, 0xffff),		"s,t"},
  {"multu",	op_func(0, 25),	op_func(0x3f, 0xffff),		"s,t"},
  {"div",	op_func(0, 26),	op_func(0x3f, 0xffff),		"s,t"},
  {"divu",	op_func(0, 27),	op_func(0x3f, 0xffff),		"s,t"},
  {"add",	op_func(0, 32),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"addu",	op_func(0, 33),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"sub",	op_func(0, 34),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"subu",	op_func(0, 35),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"and",	op_func(0, 36),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"or",	op_func(0, 37),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"xor",	op_func(0, 38),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"nor",	op_func(0, 39),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"slt",	op_func(0, 42),	op_func(0x3f, 0x7ff),		"d,s,t"},
  {"sltu",	op_func(0, 43),	op_func(0x3f, 0x7ff),		"d,s,t"},

  {"bltz",	op_cond (1, 0),	op_cond(0x3f, 0x1f),		"s,b"},
  {"bgez",	op_cond (1, 1),	op_cond(0x3f, 0x1f),		"s,b"},
  {"bltzal",	op_cond (1, 16),op_cond(0x3f, 0x1f),		"s,b"},
  {"bgezal",	op_cond (1, 17),op_cond(0x3f, 0x1f),		"s,b"},


  {"j",		one(2),		one(0x3f),			"a"},
  {"jal",	one(3),		one(0x3f),			"a"},
  {"beq",	one(4),		one(0x3f),			"s,t,b"},
  {"bne",	one(5),		one(0x3f),			"s,t,b"},
  {"blez",	one(6),		one(0x3f) | 0x1f0000,		"s,b"},
  {"bgtz",	one(7),		one(0x3f) | 0x1f0000,		"s,b"},
  {"addi",	one(8),		one(0x3f),			"t,s,j"},
  {"addiu",	one(9),		one(0x3f),			"t,s,j"},
  {"slti",	one(10),	one(0x3f),			"t,s,j"},
  {"sltiu",	one(11),	one(0x3f),			"t,s,j"},
  {"andi",	one(12),	one(0x3f),			"t,s,i"},
  {"ori",	one(13),	one(0x3f),			"t,s,i"},
  {"xori",	one(14),	one(0x3f),			"t,s,i"},
	/* rs field is don't care field? */
  {"lui",	one(15),	one(0x3f),			"t,i"},

/* co processor 0 instruction */
  {"mfc0",	op_rs_b11 (16, 0, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"cfc0",	op_rs_b11 (16, 2, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"mtc0",	op_rs_b11 (16, 4, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"ctc0",	op_rs_b11 (16, 6, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},

  {"bc0f",	op_o16(16, 0x100),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc0f",	op_o16(16, 0x180),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc0t",	op_o16(16, 0x101),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc0t",	op_o16(16, 0x181),	op_o16(0x3f, 0x3ff),	"b"},

  {"tlbr",	op_rs_func(16, 0x10, 1),
				op_rs_func(0x3f, 0x1f, 0x1f), ""},
  {"tlbwi",	op_rs_func(16, 0x10, 2),
				op_rs_func(0x3f, 0x1f, 0x1f), ""},
  {"tlbwr",	op_rs_func(16, 0x10, 6),
				op_rs_func(0x3f, 0x1f, 0x1f), ""},
  {"tlbp",	op_rs_func(16, 0x10, 8),
				op_rs_func(0x3f, 0x1f, 0x1f), ""},
  {"rfe",	op_rs_func(16, 0x10, 16),
				op_rs_func(0x3f, 0x1f, 0x1f), ""},

  {"mfc1",	op_rs_b11 (17, 0, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,S"},
  {"cfc1",	op_rs_b11 (17, 2, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,S"},
  {"mtc1",	op_rs_b11 (17, 4, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,S"},
  {"ctc1",	op_rs_b11 (17, 6, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,S"},

  {"bc1f",	op_o16(17, 0x100),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc1f",	op_o16(17, 0x180),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc1t",	op_o16(17, 0x101),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc1t",	op_o16(17, 0x181),	op_o16(0x3f, 0x3ff),	"b"},

/* fpu instruction */
  {"add.s",	op_rs_func(17, 0x10, 0),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"add.d",	op_rs_func(17, 0x11, 0),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"sub.s",	op_rs_func(17, 0x10, 1),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"sub.d",	op_rs_func(17, 0x11, 1),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"mul.s",	op_rs_func(17, 0x10, 2),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"mul.d",	op_rs_func(17, 0x11, 2),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"div.s",	op_rs_func(17, 0x10, 3),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"div.d",	op_rs_func(17, 0x11, 3),
				op_rs_func(0x3f, 0x1f, 0x3f),	"D,S,T"},
  {"abs.s",	op_rs_func(17, 0x10, 5),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"abs.d",	op_rs_func(17, 0x11, 5),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"mov.s",	op_rs_func(17, 0x10, 6),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"mov.d",	op_rs_func(17, 0x11, 6),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"neg.s",	op_rs_func(17, 0x10, 7),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"neg.d",	op_rs_func(17, 0x11, 7),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.s.s",	op_rs_func(17, 0x10, 32),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.s.d",	op_rs_func(17, 0x11, 32),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.s.w",	op_rs_func(17, 0x14, 32),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.d.s",	op_rs_func(17, 0x10, 33),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.d.d",	op_rs_func(17, 0x11, 33),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.d.w",	op_rs_func(17, 0x14, 33),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.w.s",	op_rs_func(17, 0x10, 36),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"cvt.w.d",	op_rs_func(17, 0x11, 36),
				op_rs_func(0x3f, 0x1f, 0x1f003f),	"D,S"},
  {"c.f.s",	op_rs_func(17, 0x10, 48),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.f.d",	op_rs_func(17, 0x11, 48),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.un.s",	op_rs_func(17, 0x10, 49),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.un.d",	op_rs_func(17, 0x11, 49),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.eq.s",	op_rs_func(17, 0x10, 50),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.eq.d",	op_rs_func(17, 0x11, 50),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ueq.s",	op_rs_func(17, 0x10, 51),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ueq.d",	op_rs_func(17, 0x11, 51),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.olt.s",	op_rs_func(17, 0x10, 52),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.olt.d",	op_rs_func(17, 0x11, 52),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ult.s",	op_rs_func(17, 0x10, 53),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ult.d",	op_rs_func(17, 0x11, 53),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ole.s",	op_rs_func(17, 0x10, 54),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ole.d",	op_rs_func(17, 0x11, 54),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ule.s",	op_rs_func(17, 0x10, 55),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ule.d",	op_rs_func(17, 0x11, 55),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.sf.s",	op_rs_func(17, 0x10, 56),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.sf.d",	op_rs_func(17, 0x11, 56),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ngle.s",	op_rs_func(17, 0x10, 57),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ngle.d",	op_rs_func(17, 0x11, 57),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.seq.s",	op_rs_func(17, 0x10, 58),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.seq.d",	op_rs_func(17, 0x11, 58),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ngl.s",	op_rs_func(17, 0x10, 59),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ngl.d",	op_rs_func(17, 0x11, 59),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.lt.s",	op_rs_func(17, 0x10, 60),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.lt.d",	op_rs_func(17, 0x11, 60),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.nge.s",	op_rs_func(17, 0x10, 61),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.nge.d",	op_rs_func(17, 0x11, 61),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.le.s",	op_rs_func(17, 0x10, 62),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.le.d",	op_rs_func(17, 0x11, 62),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ngt.s",	op_rs_func(17, 0x10, 63),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},
  {"c.ngt.d",	op_rs_func(17, 0x11, 63),
				op_rs_func(0x3f, 0x1f, 0x7ff),	"S,T"},

/* co processor 2 instruction */
  {"mfc2",	op_rs_b11 (18, 0, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"cfc2",	op_rs_b11 (18, 2, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"mtc2",	op_rs_b11 (18, 4, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"ctc2",	op_rs_b11 (18, 6, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"bc2f",	op_o16(18, 0x100),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc2f",	op_o16(18, 0x180),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc2t",	op_o16(18, 0x101),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc2t",	op_o16(18, 0x181),	op_o16(0x3f, 0x3ff),	"b"},

/* co processor 3 instruction */
  {"mtc3",	op_rs_b11 (19, 0, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"cfc3",	op_rs_b11 (19, 2, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"mtc3",	op_rs_b11 (19, 4, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"ctc3",	op_rs_b11 (19, 6, 0),	op_rs_b11(0x3f, 0x1f, 0x7ff),	"t,d"},
  {"bc3f",	op_o16(19, 0x100),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc3f",	op_o16(19, 0x180),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc3t",	op_o16(19, 0x101),	op_o16(0x3f, 0x3ff),	"b"},
  {"bc3t",	op_o16(19, 0x181),	op_o16(0x3f, 0x3ff),	"b"},

  {"lb",	one(32),	one(0x3f),		"t,j(s)"},
  {"lh",	one(33),	one(0x3f),		"t,j(s)"},
  {"lwl",	one(34),	one(0x3f),		"t,j(s)"},
  {"lw",	one(35),	one(0x3f),		"t,j(s)"},
  {"lbu",	one(36),	one(0x3f),		"t,j(s)"},
  {"lhu",	one(37),	one(0x3f),		"t,j(s)"},
  {"lwr",	one(38),	one(0x3f),		"t,j(s)"},
  {"sb",	one(40),	one(0x3f),		"t,j(s)"},
  {"sh",	one(41),	one(0x3f),		"t,j(s)"},
  {"swl",	one(42),	one(0x3f),		"t,j(s)"},
  {"swr",       one(46),        one(0x3f),              "t,j(s)"},
  {"sw",	one(43),	one(0x3f),		"t,j(s)"},
  {"lwc0",	one(48),	one(0x3f),		"t,j(s)"},
/* for fpu */
  {"lwc1",	one(49),	one(0x3f),		"T,j(s)"},
  {"lwc2",	one(50),	one(0x3f),		"t,j(s)"},
  {"lwc3",	one(51),	one(0x3f),		"t,j(s)"},
  {"swc0",	one(56),	one(0x3f),		"t,j(s)"},
/* for fpu */
  {"swc1",	one(57),	one(0x3f),		"T,j(s)"},
  {"swc2",	one(58),	one(0x3f),		"t,j(s)"},
  {"swc3",	one(59),	one(0x3f),		"t,j(s)"},
};
