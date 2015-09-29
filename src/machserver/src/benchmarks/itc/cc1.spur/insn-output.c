/* Generated automatically by the program `genoutput'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "regs.h"
#include "conditions.h"
#include "insn-flags.h"
#include "insn-config.h"

#include "output.h"
#include "aux-output.c"


char *
output_0 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.value1 = operands[0], cc_status.value2 = operands[1];
  return "";
}
}

char *
output_1 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  cc_status.value1 = operands[0], cc_status.value2 = const0_rtx;
  return "";
}
}

char *
output_2 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "eq", "eq", "ne", "ne"); 
}

char *
output_3 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ne", "ne", "eq", "eq"); 
}

char *
output_4 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "gt", "lt", "le", "ge"); 
}

char *
output_5 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ugt", "ult", "ule", "uge"); 
}

char *
output_6 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "lt", "gt", "ge", "le"); 
}

char *
output_7 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ult", "ugt", "uge", "ule"); 
}

char *
output_8 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ge", "le", "lt", "gt"); 
}

char *
output_9 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "uge", "ule", "ult", "ugt"); 
}

char *
output_10 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "le", "ge", "gt", "lt"); 
}

char *
output_11 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ule", "uge", "ugt", "ult"); 
}

char *
output_12 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ne", "ne", "eq", "eq"); 
}

char *
output_13 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "eq", "eq", "ne", "ne"); 
}

char *
output_14 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "le", "ge", "gt", "lt"); 
}

char *
output_15 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ule", "uge", "ugt", "ult"); 
}

char *
output_16 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ge", "le", "lt", "gt"); 
}

char *
output_17 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "uge", "ule", "ult", "ugt"); 
}

char *
output_18 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "lt", "gt", "ge", "le"); 
}

char *
output_19 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ult", "ugt", "uge", "ule"); 
}

char *
output_20 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "gt", "lt", "le", "ge"); 
}

char *
output_21 (operands, insn)
     rtx *operands;
     rtx insn;
{
 return output_compare (operands, "ugt", "ult", "ule", "uge"); 
}

char *
output_22 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM)
    return "st_32 %r1,%0";
  if (GET_CODE (operands[1]) == MEM)
    return "ld_32 %0,%1\n\tnop";
  if (GET_CODE (operands[1]) == REG)
    return "add_nt %0,%1,$0";
  if (GET_CODE (operands[1]) == SYMBOL_REF && operands[1]->unchanging)
    return "add_nt %0,r24,$(%1-0b)";
  return "add_nt %0,r0,%1";
}
}

char *
output_25 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM)
    return "st_32 %1,%0";
  if (GET_CODE (operands[1]) == MEM)
    return "ld_32 %0,%1\n\tnop";
  if (GET_CODE (operands[1]) == REG)
    return "add_nt %0,%1,$0";
  return "add_nt %0,r0,%1";
}
}

char *
output_32 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (GET_CODE (operands[0]) == MEM)
    return "st_32 %1,%0";
  if (GET_CODE (operands[1]) == MEM)
    return "ld_32 %0,%1\n\tnop";
  if (GET_CODE (operands[1]) == REG)
    return "add_nt %0,%1,$0";
  return "add_nt %0,r0,%1";
}
}

char *
output_35 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]))
    return output_fp_move_double (operands);
  if (operands[1] == dconst0_rtx && GET_CODE (operands[0]) == REG)
    {
      operands[1] = gen_rtx (REG, SImode, REGNO (operands[0]) + 1);
      return "add_nt %0,r0,$0\n\tadd_nt %1,r0,$0";
    }
  if (operands[1] == dconst0_rtx && GET_CODE (operands[0]) == MEM)
    {
      operands[1] = adj_offsetable_operand (operands[0], 4);
      return "st_32 r0,%0\n\tst_32 r0,%1";
    }
  return output_move_double (operands);
}

}

char *
output_36 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]) || FP_REG_P (operands[1]))
    return output_fp_move_double (operands);
  return output_move_double (operands);
}

}

char *
output_37 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]) || FP_REG_P (operands[1]))
    return output_fp_move_double (operands);
  return output_move_double (operands);
}

}

char *
output_38 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  if (FP_REG_P (operands[0]))
    {
      if (FP_REG_P (operands[1]))
	return "fmov %0,%1";
      if (GET_CODE (operands[1]) == REG)
	{
	  rtx xoperands[2];
	  int offset = - get_frame_size () - 8;
	  xoperands[1] = operands[1];
	  xoperands[0] = gen_rtx (CONST_INT, VOIDmode, offset);
	  output_asm_insn ("st_32 %1,r25,%0", xoperands);
	  xoperands[1] = operands[0];
	  output_asm_insn ("ld_sgl %1,r25,%0\n\tnop", xoperands);
	  return "";
	}
      return "ld_sgl %0,%1\n\tnop";
    }
  if (FP_REG_P (operands[1]))
    {
      if (GET_CODE (operands[0]) == REG)
	{
	  rtx xoperands[2];
	  int offset = - get_frame_size () - 8;
	  xoperands[0] = gen_rtx (CONST_INT, VOIDmode, offset);
	  xoperands[1] = operands[1];
	  output_asm_insn ("st_sgl %1,r25,%0", xoperands);
	  xoperands[1] = operands[0];
	  output_asm_insn ("ld_32 %1,r25,%0\n\tnop", xoperands);
	  return "";
	}
      return "st_sgl %1,%0";
    }
  if (GET_CODE (operands[0]) == MEM)
    return "st_32 %r1,%0";
  if (GET_CODE (operands[1]) == MEM)
    return "ld_32 %0,%1\n\tnop";
  if (GET_CODE (operands[1]) == REG)
    return "add_nt %0,%1,$0";
  return "add_nt %0,r0,%1";
}
}

char *
output_49 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  return 
     output_add_large_offset (operands[0], operands[1], INTVAL (operands[2]));
}
}

char *
output_68 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  unsigned int amount = INTVAL (operands[2]);

  switch (amount) {
  case 0:
      return "add_nt %0,%1,$0";
  case 1:
      return "sll %0,%1,$1";
  case 2:
      return "sll %0,%1,$2";
  default:
      output_asm_insn ("sll %0,%1,$3", operands);

      for (amount -= 3; amount >= 3; amount -= 3) {
	  output_asm_insn ("sll %0,%0,$3", operands);
      }

      if (amount > 0)
	  output_asm_insn (amount == 1 ? "sll %0,%0,$1" : "sll %0,%0,$2",
			   operands);
      return "";
  }
}
}

char *
output_69 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  unsigned int amount = INTVAL (operands[2]);

  if (amount == 0) 
      return "add_nt %0,%1,$0";
  else
      output_asm_insn ("sra %0,%1,$1", operands);
  
  for (amount -= 1; amount > 0; amount -= 1) {
      output_asm_insn ("sra %0,%0,$1", operands);
  }

  return "";
}
}

char *
output_70 (operands, insn)
     rtx *operands;
     rtx insn;
{

{
  unsigned int amount = INTVAL (operands[2]);

  if (amount == 0) 
      return "add_nt %0,%1,$0";
  else
      output_asm_insn ("srl %0,%1,$1", operands);
  
  for (amount -= 1; amount > 0; amount -= 1) {
      output_asm_insn ("srl %0,%0,$1", operands);
  }

  return "";
}
}

char *insn_template[] =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "ld_32 %0,%1,%2\n\tnop",
    0,
    0,
    "extract %0,%1,%2",
    "wr_insert %1\n\tinsert %0,%0,%2",
    0,
    0,
    0,
    0,
    0,
    "extract %0,%1,%2",
    "wr_insert %1\n\tinsert %0,%0,%2",
    0,
    0,
    0,
    0,
    "add_nt %0,%1,$0",
    "add_nt %0,%1,$0",
    "add_nt %0,%1,$0",
    0,
    "extract %0,%1,$0",
    "extract %0,%1,$0",
    0,
    0,
    0,
    "add %0,%1,%2",
    0,
    "sub %0,%1,%2",
    "and %0,%1,%2",
    "or %0,%1,%2",
    "xor %0,%1,%2",
    "sub %0,r0,%1",
    "xor %0,%1,$-1",
    "fadd %0,%1,%2",
    "fadd %0,%1,%2",
    "fsub %0,%1,%2",
    "fsub %0,%1,%2",
    "fmul %0,%1,%2",
    "fmul %0,%1,%2",
    "fdiv %0,%1,%2",
    "fdiv %0,%1,%2",
    "fneg %0,%1",
    "fneg %0,%1",
    "fabs %0,%1",
    "fabs %0,%1",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "jump %l0\n\tnop",
    "jump_reg r0,%0\n\tnop",
    "add_nt r2,%0\n\tcall .+8\n\tjump_reg r0,r2\n\tnop",
    "add_nt r2,%1\n\tcall .+8\n\tjump_reg r0,r2\n\tnop",
    "call %0\n\tnop",
    "call %1\n\tnop",
  };

char *(*insn_outfun[])() =
  {
    output_0,
    output_1,
    output_2,
    output_3,
    output_4,
    output_5,
    output_6,
    output_7,
    output_8,
    output_9,
    output_10,
    output_11,
    output_12,
    output_13,
    output_14,
    output_15,
    output_16,
    output_17,
    output_18,
    output_19,
    output_20,
    output_21,
    output_22,
    0,
    0,
    output_25,
    0,
    0,
    0,
    0,
    0,
    0,
    output_32,
    0,
    0,
    output_35,
    output_36,
    output_37,
    output_38,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_49,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    output_68,
    output_69,
    output_70,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };

rtx (*insn_gen_function[]) () =
  {
    gen_cmpsi,
    gen_tstsi,
    gen_beq,
    gen_bne,
    gen_bgt,
    gen_bgtu,
    gen_blt,
    gen_bltu,
    gen_bge,
    gen_bgeu,
    gen_ble,
    gen_bleu,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    gen_movsi,
    0,
    gen_movqi,
    0,
    0,
    0,
    gen_loadhi,
    gen_storehi,
    gen_storeinthi,
    gen_movhi,
    0,
    0,
    0,
    0,
    gen_movdf,
    gen_movdi,
    gen_movsf,
    gen_truncsiqi2,
    gen_trunchiqi2,
    gen_truncsihi2,
    gen_zero_extendhisi2,
    gen_zero_extendqihi2,
    gen_zero_extendqisi2,
    gen_extendhisi2,
    gen_extendqihi2,
    gen_extendqisi2,
    gen_addsi3,
    0,
    gen_subsi3,
    gen_andsi3,
    gen_iorsi3,
    gen_xorsi3,
    gen_negsi2,
    gen_one_cmplsi2,
    gen_adddf3,
    gen_addsf3,
    gen_subdf3,
    gen_subsf3,
    gen_muldf3,
    gen_mulsf3,
    gen_divdf3,
    gen_divsf3,
    gen_negdf2,
    gen_negsf2,
    gen_absdf2,
    gen_abssf2,
    0,
    0,
    0,
    gen_ashlsi3,
    gen_lshlsi3,
    gen_ashrsi3,
    gen_lshrsi3,
    gen_jump,
    gen_tablejump,
    gen_call,
    gen_call_value,
    0,
    0,
  };

int insn_n_operands[] =
  {
    2,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,
    3,
    2,
    2,
    3,
    3,
    5,
    5,
    4,
    2,
    2,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    0,
    1,
    2,
    3,
    2,
    3,
  };

int insn_n_dups[] =
  {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  };

char *insn_operand_constraint[][MAX_RECOG_OPERANDS] =
  {
    { "rK", "rK", },
    { "r", },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { "=r,m", "rmi,rJ", },
    { "=r", "r", "r", },
    { "", "", },
    { "=r,m", "rmi,r", },
    { "=r", "r", "rI", },
    { "+r", "rI", "ri", },
    { "", "", "", "", "", },
    { "", "", "", "", "", },
    { "", "", "", "", },
    { "", "", },
    { "=r,m", "rmi,r", },
    { "=r", "r", "rI", },
    { "+r", "rI", "ri", },
    { "=&r,f,&o", "mG,m,G", },
    { "=r,&r,m,?f,?rm", "r,m,r,rfm,f", },
    { "=r,&r,m,?f,?rm", "r,m,r,rfm,f", },
    { "=rf,m", "rfm,rf", },
    { "=r", "r", },
    { "=r", "r", },
    { "=r", "r", },
    { "", "", },
    { "=r", "r", },
    { "=r", "r", },
    { "", "", },
    { "", "", },
    { "", "", },
    { "=r", "%r", "rI", },
    { "=r", "%r", "g", },
    { "=r", "r", "rI", },
    { "=r", "%r", "rI", },
    { "=r", "%r", "rI", },
    { "=r", "%r", "rI", },
    { "=r", "rI", },
    { "=r", "r", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", "f", },
    { "=f", "f", },
    { "=f", "f", },
    { "=f", "f", },
    { "=f", "f", },
    { "=r", "r", "I", },
    { "=r", "r", "I", },
    { "=r", "r", "I", },
    { "", "", "", },
    { "", "", "", },
    { "", "", "", },
    { "", "", "", },
    { 0 },
    { "r", },
    { "m", "g", },
    { "g", "m", "g", },
    { "i", "g", },
    { "g", "i", "g", },
  };

enum machine_mode insn_operand_mode[][MAX_RECOG_OPERANDS] =
  {
    { SImode, SImode, },
    { SImode, },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { VOIDmode },
    { SImode, SImode, },
    { SImode, SImode, SImode, },
    { QImode, QImode, },
    { QImode, QImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { HImode, SImode, SImode, SImode, QImode, },
    { SImode, HImode, SImode, SImode, QImode, },
    { SImode, VOIDmode, SImode, SImode, },
    { HImode, HImode, },
    { HImode, HImode, },
    { SImode, HImode, SImode, },
    { HImode, SImode, SImode, },
    { DFmode, DFmode, },
    { DFmode, DFmode, },
    { DImode, DImode, },
    { SFmode, SFmode, },
    { QImode, SImode, },
    { QImode, HImode, },
    { HImode, SImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { SImode, HImode, },
    { HImode, QImode, },
    { SImode, QImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, },
    { SImode, SImode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, DFmode, },
    { SFmode, SFmode, SFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { DFmode, DFmode, },
    { SFmode, SFmode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { SImode, SImode, SImode, },
    { VOIDmode },
    { SImode, },
    { SImode, SImode, },
    { VOIDmode, SImode, SImode, },
    { SImode, SImode, },
    { VOIDmode, SImode, SImode, },
  };

char insn_operand_strict_low[][MAX_RECOG_OPERANDS] =
  {
    { 0, 0, },
    { 0, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0, 0, 0, },
    { 0 },
    { 0, },
    { 0, 0, },
    { 0, 0, 0, },
    { 0, 0, },
    { 0, 0, 0, },
  };

int (*insn_operand_predicate[][MAX_RECOG_OPERANDS])() =
  {
    { nonmemory_operand, nonmemory_operand, },
    { register_operand, },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { general_operand, general_operand, },
    { register_operand, register_operand, register_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, nonmemory_operand, },
    { register_operand, register_operand, register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, register_operand, register_operand, },
    { register_operand, 0, register_operand, register_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, nonmemory_operand, },
    { general_operand, 0, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { general_operand, general_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, },
    { register_operand, nonmemory_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, big_immediate_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, },
    { register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, register_operand, register_operand, },
    { register_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, },
    { register_operand, nonmemory_operand, },
    { register_operand, register_operand, immediate_operand, },
    { register_operand, register_operand, immediate_operand, },
    { register_operand, register_operand, immediate_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { register_operand, register_operand, nonmemory_operand, },
    { 0 },
    { register_operand, },
    { memory_operand, general_operand, },
    { 0, memory_operand, general_operand, },
    { 0, general_operand, },
    { 0, 0, general_operand, },
  };
