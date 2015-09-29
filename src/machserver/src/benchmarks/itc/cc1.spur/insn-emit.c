/* Generated automatically by the program `genemit'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "expr.h"
#include "insn-config.h"

extern char *insn_operand_constraint[][MAX_RECOG_OPERANDS];

extern rtx recog_operand[];
#define operands recog_operand

#define FAIL do { emit_to_sequence--; return 0;} while (0)

#define DONE do { emit_to_sequence--; return gen_sequence ();} while (0)

rtx
gen_cmpsi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, cc0_rtx, gen_rtx (MINUS, VOIDmode, operand0, operand1));
}

rtx
gen_tstsi (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, cc0_rtx, operand0);
}

rtx
gen_beq (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (EQ, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bne (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (NE, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bgt (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GT, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bgtu (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GTU, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_blt (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LT, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bltu (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LTU, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bge (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GE, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bgeu (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (GEU, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_ble (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LE, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_bleu (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (IF_THEN_ELSE, VOIDmode, gen_rtx (LEU, VOIDmode, cc0_rtx, const0_rtx), gen_rtx (LABEL_REF, VOIDmode, operand0), pc_rtx));
}

rtx
gen_movsi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, operand1);
}

rtx
gen_movqi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (GET_CODE (operands[0]) == MEM && GET_CODE (operands[1]) == MEM)
    operands[1] = copy_to_reg (operands[1]);

  if (GET_CODE (operands[1]) == MEM)
    {
      rtx tem = gen_reg_rtx (SImode);
      rtx addr = force_reg (SImode, XEXP (operands[1], 0));
      emit_move_insn (tem, gen_rtx (MEM, SImode, addr));
      emit_insn (gen_rtx (SET, VOIDmode,
			  gen_rtx (SUBREG, SImode, operands[0], 0),
			  gen_rtx (ZERO_EXTRACT, SImode, tem,
				   gen_rtx (CONST_INT, VOIDmode, 8),
				   addr)));
    }
  else if (GET_CODE (operands[0]) == MEM)
    {
      rtx tem = gen_reg_rtx (SImode);
      rtx addr = force_reg (SImode, XEXP (operands[0], 0));
      emit_move_insn (tem, gen_rtx (MEM, SImode, addr));
      if (! CONSTANT_ADDRESS_P (operands[1]))
	operands[1] = gen_rtx (SUBREG, SImode, operands[1], 0);
      emit_insn (gen_rtx (SET, VOIDmode,
			  gen_rtx (ZERO_EXTRACT, SImode, tem,
				   gen_rtx (CONST_INT, VOIDmode, 8),
				   addr),
			  operands[1]));
      emit_move_insn (gen_rtx (MEM, SImode, addr), tem);
    }
  else
    {
      emit_insn (gen_rtx (SET, VOIDmode, operands[0], operands[1]));
    }
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_loadhi (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  rtx operand5;
  rtx operands[6];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;
  operands[4] = operand4;
operands[5] = gen_reg_rtx (HImode);
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  operand5 = operands[5];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (MEM, SImode, operand1)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand5, 0), gen_rtx (ZERO_EXTRACT, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, 8), operand1)));
  emit_insn (gen_rtx (SET, VOIDmode, operand3, gen_rtx (PLUS, SImode, operand1, const1_rtx)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand4, 0), gen_rtx (ZERO_EXTRACT, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, 8), operand3)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (ZERO_EXTRACT, SImode, operand5, gen_rtx (CONST_INT, VOIDmode, 8), const1_rtx), gen_rtx (SUBREG, SImode, operand4, 0)));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand5));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_storehi (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  rtx operands[5];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;
  operands[4] = operand4;

  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (MEM, SImode, operand0)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (ZERO_EXTRACT, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, 8), operand0), gen_rtx (SUBREG, SImode, operand1, 0)));
  emit_insn (gen_rtx (SET, VOIDmode, operand3, gen_rtx (PLUS, SImode, operand0, const1_rtx)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (SUBREG, SImode, operand4, 0), gen_rtx (ZERO_EXTRACT, SImode, operand1, gen_rtx (CONST_INT, VOIDmode, 8), const1_rtx)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (ZERO_EXTRACT, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, 8), operand3), gen_rtx (SUBREG, SImode, operand4, 0)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (MEM, SImode, operand0), operand2));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_storeinthi (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  rtx operand4;
  rtx operand5;
  rtx operand6;
  rtx operands[7];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;
  operands[3] = operand3;
 operands[5] = gen_rtx (CONST_INT, VOIDmode, INTVAL (operands[1]) & 255);
    operands[6] = gen_rtx (CONST_INT, VOIDmode,
			   (INTVAL (operands[1]) >> 8) & 255);

  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  operand5 = operands[5];
  operand6 = operands[6];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (MEM, SImode, operand0)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (ZERO_EXTRACT, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, 8), operand0), operand5));
  emit_insn (gen_rtx (SET, VOIDmode, operand3, gen_rtx (PLUS, SImode, operand0, const1_rtx)));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (ZERO_EXTRACT, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, 8), operand3), operand6));
  emit_insn (gen_rtx (SET, VOIDmode, gen_rtx (MEM, SImode, operand0), operand2));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_movhi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operands[2];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;

{
  if (GET_CODE (operands[0]) == MEM && GET_CODE (operands[1]) == MEM)
    operands[1] = copy_to_reg (operands[1]);
  
  if (GET_CODE (operands[1]) == MEM)
    {
      rtx insn =
	emit_insn (gen_loadhi (operands[0],
			       force_reg (SImode, XEXP (operands[1], 0)),
			       gen_reg_rtx (SImode), gen_reg_rtx (SImode),
			       gen_reg_rtx (QImode)));
      /* Tell cse what value the loadhi produces, so it detect duplicates.  */
      REG_NOTES (insn) = gen_rtx (EXPR_LIST, REG_EQUAL, operands[1], 0);
    }
  else if (GET_CODE (operands[0]) == MEM)
    {
      if (GET_CODE (operands[1]) == CONST_INT)
	emit_insn (gen_storeinthi (force_reg (SImode, XEXP (operands[0], 0)),
			           operands[1],
				   gen_reg_rtx (SImode), gen_reg_rtx (SImode),
				   gen_reg_rtx (QImode)));
      else
	{
	  if (CONSTANT_P (operands[1]))
            operands[1] = force_reg (HImode, operands[1]);
	  emit_insn (gen_storehi (force_reg (SImode, XEXP (operands[0], 0)),
				  operands[1],
				  gen_reg_rtx (SImode), gen_reg_rtx (SImode),
				  gen_reg_rtx (QImode)));
	}
    }
  else
    emit_insn (gen_rtx (SET, VOIDmode, operands[0], operands[1]));
  DONE;
}
  operand0 = operands[0];
  operand1 = operands[1];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, operand1));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_movdf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, operand1);
}

rtx
gen_movdi (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, operand1);
}

rtx
gen_movsf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, operand1);
}

rtx
gen_truncsiqi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (TRUNCATE, QImode, operand1));
}

rtx
gen_trunchiqi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (TRUNCATE, QImode, operand1));
}

rtx
gen_truncsihi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (TRUNCATE, HImode, operand1));
}

rtx
gen_zero_extendhisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operand2;
  rtx operands[3];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = force_reg (SImode, gen_rtx (CONST_INT, VOIDmode, 65535)); 
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (AND, SImode, gen_rtx (SUBREG, SImode, operand1, 0), operand2)));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_zero_extendqihi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ZERO_EXTEND, HImode, operand1));
}

rtx
gen_zero_extendqisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ZERO_EXTEND, SImode, operand1));
}

rtx
gen_extendhisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operand2;
  rtx operand3;
  rtx operand4;
  rtx operand5;
  rtx operands[6];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;

{
  operands[2] = gen_reg_rtx (SImode);
  operands[3] = gen_reg_rtx (SImode);
  operands[4] = force_reg (SImode, gen_rtx (CONST_INT, VOIDmode, 65535));
  operands[5] = force_reg (SImode, gen_rtx (CONST_INT, VOIDmode, -32768));
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  operand4 = operands[4];
  operand5 = operands[5];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (AND, SImode, gen_rtx (SUBREG, SImode, operand1, 0), operand4)));
  emit_insn (gen_rtx (SET, VOIDmode, operand3, gen_rtx (PLUS, SImode, operand2, operand5)));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (XOR, SImode, operand3, operand5)));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_extendqihi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operand2;
  rtx operand3;
  rtx operands[4];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;

{
  operands[2] = gen_reg_rtx (HImode);
  operands[3] = gen_reg_rtx (HImode);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (AND, HImode, gen_rtx (SUBREG, HImode, operand1, 0), gen_rtx (CONST_INT, VOIDmode, 255))));
  emit_insn (gen_rtx (SET, VOIDmode, operand3, gen_rtx (PLUS, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, -128))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (XOR, SImode, operand3, gen_rtx (CONST_INT, VOIDmode, -128))));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_extendqisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  rtx operand2;
  rtx operand3;
  rtx operands[4];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;

{
  operands[2] = gen_reg_rtx (SImode);
  operands[3] = gen_reg_rtx (SImode);
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  operand3 = operands[3];
  emit_insn (gen_rtx (SET, VOIDmode, operand2, gen_rtx (AND, SImode, gen_rtx (SUBREG, SImode, operand1, 0), gen_rtx (CONST_INT, VOIDmode, 255))));
  emit_insn (gen_rtx (SET, VOIDmode, operand3, gen_rtx (PLUS, SImode, operand2, gen_rtx (CONST_INT, VOIDmode, -128))));
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (XOR, SImode, operand3, gen_rtx (CONST_INT, VOIDmode, -128))));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_addsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SImode, operand1, operand2));
}

rtx
gen_subsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, SImode, operand1, operand2));
}

rtx
gen_andsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (AND, SImode, operand1, operand2));
}

rtx
gen_iorsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (IOR, SImode, operand1, operand2));
}

rtx
gen_xorsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (XOR, SImode, operand1, operand2));
}

rtx
gen_negsi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, SImode, operand1));
}

rtx
gen_one_cmplsi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NOT, SImode, operand1));
}

rtx
gen_adddf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, DFmode, operand1, operand2));
}

rtx
gen_addsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (PLUS, SFmode, operand1, operand2));
}

rtx
gen_subdf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, DFmode, operand1, operand2));
}

rtx
gen_subsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MINUS, SFmode, operand1, operand2));
}

rtx
gen_muldf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MULT, DFmode, operand1, operand2));
}

rtx
gen_mulsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (MULT, SFmode, operand1, operand2));
}

rtx
gen_divdf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (DIV, DFmode, operand1, operand2));
}

rtx
gen_divsf3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (DIV, SFmode, operand1, operand2));
}

rtx
gen_negdf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, DFmode, operand1));
}

rtx
gen_negsf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (NEG, SFmode, operand1));
}

rtx
gen_absdf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ABS, DFmode, operand1));
}

rtx
gen_abssf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (ABS, SFmode, operand1));
}

rtx
gen_ashlsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (GET_CODE (operands[2]) != CONST_INT
      || ! TARGET_EXPAND_SHIFTS && (unsigned) INTVAL (operands[2]) > 3)
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFT, SImode, operand1, operand2)));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_lshlsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (GET_CODE (operands[2]) != CONST_INT
      || ! TARGET_EXPAND_SHIFTS && (unsigned) INTVAL (operands[2]) > 3)
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFT, SImode, operand1, operand2)));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_ashrsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (GET_CODE (operands[2]) != CONST_INT
      || ! TARGET_EXPAND_SHIFTS && (unsigned) INTVAL (operands[2]) > 1)
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (ASHIFTRT, SImode, operand1, operand2)));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_lshrsi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  rtx operands[3];

  extern rtx gen_sequence ();
  extern int emit_to_sequence;

  emit_to_sequence++;
  operands[0] = operand0;
  operands[1] = operand1;
  operands[2] = operand2;

{
  if (GET_CODE (operands[2]) != CONST_INT
      || ! TARGET_EXPAND_SHIFTS && (unsigned) INTVAL (operands[2]) > 1)
    FAIL;
}
  operand0 = operands[0];
  operand1 = operands[1];
  operand2 = operands[2];
  emit_insn (gen_rtx (SET, VOIDmode, operand0, gen_rtx (LSHIFTRT, SImode, operand1, operand2)));
  emit_to_sequence--;
  return gen_sequence ();
}

rtx
gen_jump (operand0)
     rtx operand0;
{
  return gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx (LABEL_REF, VOIDmode, operand0));
}

rtx
gen_tablejump (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (PARALLEL, VOIDmode, gen_rtvec (2,
		gen_rtx (SET, VOIDmode, pc_rtx, operand0),
		gen_rtx (USE, VOIDmode, gen_rtx (LABEL_REF, VOIDmode, operand1))));
}

rtx
gen_call (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx (CALL, VOIDmode, operand0, operand1);
}

rtx
gen_call_value (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx (SET, VOIDmode, operand0, gen_rtx (CALL, VOIDmode, operand1, operand2));
}

