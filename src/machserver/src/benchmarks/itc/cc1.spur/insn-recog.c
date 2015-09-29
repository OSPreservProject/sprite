/* Generated automatically by the program `genrecog'
from the machine description file `md'.  */

#include "config.h"
#include "rtl.h"
#include "insn-config.h"
#include "recog.h"

/* `recog' contains a decision tree
   that recognizes whether the rtx X0 is a valid instruction.

   recog returns -1 if the rtx is not valid.
   If the rtx is valid, recog returns a nonnegative number
   which is the insn code number for the pattern that matched.
   This is the same as the order in the machine description of
   the entry that matched.  This number can be used as an index into
   insn_templates and insn_n_operands (found in insn-output.c)
   or as an argument to output_insn_hairy (also in insn-output.c).  */

rtx recog_operand[MAX_RECOG_OPERANDS];

rtx *recog_operand_loc[MAX_RECOG_OPERANDS];

rtx *recog_dup_loc[MAX_DUP_OPERANDS];

char recog_dup_num[MAX_DUP_OPERANDS];

extern rtx recog_addr_dummy;

#define operands recog_operand

int
recog_1 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L11:
  x1 = XEXP (x0, 1);
  x2 = XEXP (x1, 0);
 switch (GET_CODE (x2))
  {
  case EQ:
  if (1)
    goto L12;
  break;
  case NE:
  if (1)
    goto L21;
  break;
  case GT:
  if (1)
    goto L30;
  break;
  case GTU:
  if (1)
    goto L39;
  break;
  case LT:
  if (1)
    goto L48;
  break;
  case LTU:
  if (1)
    goto L57;
  break;
  case GE:
  if (1)
    goto L66;
  break;
  case GEU:
  if (1)
    goto L75;
  break;
  case LE:
  if (1)
    goto L84;
  break;
  case LEU:
  if (1)
    goto L93;
  break;
  }
  goto ret0;
 L12:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L13;
  goto ret0;
 L13:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L14;
  goto ret0;
 L14:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L15;
  if (x2 == pc_rtx && 1)
    goto L105;
  goto ret0;
 L15:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L16; }
  goto ret0;
 L16:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 2;
  goto ret0;
 L105:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L106;
  goto ret0;
 L106:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 12; }
  goto ret0;
 L21:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L22;
  goto ret0;
 L22:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L23;
  goto ret0;
 L23:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L24;
  if (x2 == pc_rtx && 1)
    goto L114;
  goto ret0;
 L24:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L25; }
  goto ret0;
 L25:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 3;
  goto ret0;
 L114:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L115;
  goto ret0;
 L115:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 13; }
  goto ret0;
 L30:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L31;
  goto ret0;
 L31:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L32;
  goto ret0;
 L32:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L33;
  if (x2 == pc_rtx && 1)
    goto L123;
  goto ret0;
 L33:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L34; }
  goto ret0;
 L34:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 4;
  goto ret0;
 L123:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L124;
  goto ret0;
 L124:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 14; }
  goto ret0;
 L39:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L40;
  goto ret0;
 L40:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L41;
  goto ret0;
 L41:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L42;
  if (x2 == pc_rtx && 1)
    goto L132;
  goto ret0;
 L42:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L43; }
  goto ret0;
 L43:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 5;
  goto ret0;
 L132:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L133;
  goto ret0;
 L133:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 15; }
  goto ret0;
 L48:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L49;
  goto ret0;
 L49:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L50;
  goto ret0;
 L50:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L51;
  if (x2 == pc_rtx && 1)
    goto L141;
  goto ret0;
 L51:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L52; }
  goto ret0;
 L52:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 6;
  goto ret0;
 L141:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L142;
  goto ret0;
 L142:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 16; }
  goto ret0;
 L57:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L58;
  goto ret0;
 L58:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L59;
  goto ret0;
 L59:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L60;
  if (x2 == pc_rtx && 1)
    goto L150;
  goto ret0;
 L60:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L61; }
  goto ret0;
 L61:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 7;
  goto ret0;
 L150:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L151;
  goto ret0;
 L151:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 17; }
  goto ret0;
 L66:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L67;
  goto ret0;
 L67:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L68;
  goto ret0;
 L68:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L69;
  if (x2 == pc_rtx && 1)
    goto L159;
  goto ret0;
 L69:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L70; }
  goto ret0;
 L70:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 8;
  goto ret0;
 L159:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L160;
  goto ret0;
 L160:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 18; }
  goto ret0;
 L75:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L76;
  goto ret0;
 L76:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L77;
  goto ret0;
 L77:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L78;
  if (x2 == pc_rtx && 1)
    goto L168;
  goto ret0;
 L78:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L79; }
  goto ret0;
 L79:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 9;
  goto ret0;
 L168:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L169;
  goto ret0;
 L169:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 19; }
  goto ret0;
 L84:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L85;
  goto ret0;
 L85:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L86;
  goto ret0;
 L86:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L87;
  if (x2 == pc_rtx && 1)
    goto L177;
  goto ret0;
 L87:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L88; }
  goto ret0;
 L88:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 10;
  goto ret0;
 L177:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L178;
  goto ret0;
 L178:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 20; }
  goto ret0;
 L93:
  x3 = XEXP (x2, 0);
  if (x3 == cc0_rtx && 1)
    goto L94;
  goto ret0;
 L94:
  x3 = XEXP (x2, 1);
  if (x3 == const0_rtx && 1)
    goto L95;
  goto ret0;
 L95:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L96;
  if (x2 == pc_rtx && 1)
    goto L186;
  goto ret0;
 L96:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; goto L97; }
  goto ret0;
 L97:
  x2 = XEXP (x1, 2);
  if (x2 == pc_rtx && 1)
    return 11;
  goto ret0;
 L186:
  x2 = XEXP (x1, 2);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L187;
  goto ret0;
 L187:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[0] = x3; return 21; }
  goto ret0;
 ret0: return -1;
}

int
recog_2 (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L1:
  x1 = XEXP (x0, 0);
  if (x1 == cc0_rtx && 1)
    goto L2;
  if (x1 == pc_rtx && 1)
    goto L10;
 L383:
  if (1)
    { recog_operand[0] = x1; goto L384; }
 L198:
 switch (GET_MODE (x1))
  {
  case QImode:
  if (general_operand (x1, QImode))
    { recog_operand[0] = x1; goto L199; }
 L240:
  if (register_operand (x1, QImode))
    { recog_operand[0] = x1; goto L241; }
  break;
  case HImode:
  if (general_operand (x1, HImode))
    { recog_operand[0] = x1; goto L214; }
 L248:
  if (register_operand (x1, HImode))
    { recog_operand[0] = x1; goto L249; }
  break;
  case SImode:
  if (GET_CODE (x1) == ZERO_EXTRACT && 1)
    goto L223;
 L189:
  if (general_operand (x1, SImode))
    { recog_operand[0] = x1; goto L190; }
 L192:
  if (register_operand (x1, SImode))
    { recog_operand[0] = x1; goto L193; }
  break;
  case DImode:
  if (general_operand (x1, DImode))
    { recog_operand[0] = x1; goto L235; }
  break;
  case SFmode:
  if (general_operand (x1, SFmode))
    { recog_operand[0] = x1; goto L238; }
 L303:
  if (register_operand (x1, SFmode))
    { recog_operand[0] = x1; goto L304; }
  break;
  case DFmode:
  if (general_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L229; }
 L298:
  if (register_operand (x1, DFmode))
    { recog_operand[0] = x1; goto L299; }
  break;
  }
  goto ret0;
 L2:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == MINUS && 1)
    goto L3;
 L7:
  if (register_operand (x1, SImode))
    { recog_operand[0] = x1; return 1; }
  x1 = XEXP (x0, 0);
  goto L383;
 L3:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[0] = x2; goto L4; }
  goto L7;
 L4:
  x2 = XEXP (x1, 1);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; return 0; }
  goto L7;
 L10:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == IF_THEN_ELSE && 1)
    goto L11;
  if (GET_CODE (x1) == LABEL_REF && 1)
    goto L371;
  x1 = XEXP (x0, 0);
  goto L383;
 L11:
  tem = recog_1 (x0, insn);
  if (tem >= 0) return tem;
  x1 = XEXP (x0, 0);
  goto L383;
 L371:
  x2 = XEXP (x1, 0);
  if (1)
    { recog_operand[0] = x2; return 75; }
  x1 = XEXP (x0, 0);
  goto L383;
 L384:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == CALL && 1)
    goto L394;
  x1 = XEXP (x0, 0);
  goto L198;
 L394:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == MEM && GET_MODE (x2) == SImode && 1)
    goto L395;
 L385:
  if (memory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L386; }
  x1 = XEXP (x0, 0);
  goto L198;
 L395:
  x3 = XEXP (x2, 0);
  if (GET_MODE (x3) == SImode && 1)
    { recog_operand[1] = x3; goto L396; }
  goto L385;
 L396:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[1]) == SYMBOL_REF) return 80; }
  x2 = XEXP (x1, 0);
  goto L385;
 L386:
  x2 = XEXP (x1, 1);
  if (general_operand (x2, SImode))
    { recog_operand[2] = x2; return 78; }
  x1 = XEXP (x0, 0);
  goto L198;
 L199:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, QImode))
    { recog_operand[1] = x1; return 25; }
  x1 = XEXP (x0, 0);
  goto L240;
 L241:
  x1 = XEXP (x0, 1);
  if (GET_CODE (x1) == TRUNCATE && GET_MODE (x1) == QImode && 1)
    goto L242;
  goto ret0;
 L242:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; return 39; }
 L246:
  if (register_operand (x2, HImode))
    { recog_operand[1] = x2; return 40; }
  goto ret0;
 L214:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, HImode))
    { recog_operand[1] = x1; return 32; }
  x1 = XEXP (x0, 0);
  goto L248;
 L249:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != HImode)
    goto ret0;
  if (GET_CODE (x1) == TRUNCATE && 1)
    goto L250;
  if (GET_CODE (x1) == ZERO_EXTEND && 1)
    goto L254;
  goto ret0;
 L250:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; return 41; }
  goto ret0;
 L254:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, QImode))
    { recog_operand[1] = x2; return 43; }
  goto ret0;
 L223:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, HImode))
    { recog_operand[0] = x2; goto L224; }
 L208:
  if (register_operand (x2, SImode))
    { recog_operand[0] = x2; goto L209; }
  goto L189;
 L224:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && XINT (x2, 0) == 8 && 1)
    goto L225;
  x2 = XEXP (x1, 0);
  goto L208;
 L225:
  x2 = XEXP (x1, 2);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L226; }
  x2 = XEXP (x1, 0);
  goto L208;
 L226:
  x1 = XEXP (x0, 1);
  if (nonmemory_operand (x1, SImode))
    { recog_operand[2] = x1; return 34; }
  x1 = XEXP (x0, 0);
  x2 = XEXP (x1, 0);
  goto L208;
 L209:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && XINT (x2, 0) == 8 && 1)
    goto L210;
  goto L189;
 L210:
  x2 = XEXP (x1, 2);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L211; }
  goto L189;
 L211:
  x1 = XEXP (x0, 1);
  if (nonmemory_operand (x1, SImode))
    { recog_operand[2] = x1; return 27; }
  x1 = XEXP (x0, 0);
  goto L189;
 L190:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[1] = x1; return 22; }
  x1 = XEXP (x0, 0);
  goto L192;
 L193:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != SImode)
    goto ret0;
 switch (GET_CODE (x1))
  {
  case MEM:
  if (1)
    goto L194;
  break;
  case ZERO_EXTRACT:
  if (1)
    goto L218;
  break;
  case ZERO_EXTEND:
  if (1)
    goto L258;
  break;
  case PLUS:
  if (1)
    goto L262;
  break;
  case MINUS:
  if (1)
    goto L272;
  break;
  case AND:
  if (1)
    goto L277;
  break;
  case IOR:
  if (1)
    goto L282;
  break;
  case XOR:
  if (1)
    goto L287;
  break;
  case NEG:
  if (1)
    goto L292;
  break;
  case NOT:
  if (1)
    goto L296;
  break;
  case ASHIFT:
  if (1)
    goto L356;
  break;
  case ASHIFTRT:
  if (1)
    goto L361;
  break;
  case LSHIFTRT:
  if (1)
    goto L366;
  break;
  }
  goto ret0;
 L194:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == PLUS && GET_MODE (x2) == SImode && 1)
    goto L195;
  goto ret0;
 L195:
  x3 = XEXP (x2, 0);
  if (register_operand (x3, SImode))
    { recog_operand[1] = x3; goto L196; }
  goto ret0;
 L196:
  x3 = XEXP (x2, 1);
  if (register_operand (x3, SImode))
    { recog_operand[2] = x3; return 23; }
  goto ret0;
 L218:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, HImode))
    { recog_operand[1] = x2; goto L219; }
 L203:
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; goto L204; }
  goto ret0;
 L219:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && XINT (x2, 0) == 8 && 1)
    goto L220;
  x2 = XEXP (x1, 0);
  goto L203;
 L220:
  x2 = XEXP (x1, 2);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 33; }
  x2 = XEXP (x1, 0);
  goto L203;
 L204:
  x2 = XEXP (x1, 1);
  if (GET_CODE (x2) == CONST_INT && XINT (x2, 0) == 8 && 1)
    goto L205;
  goto ret0;
 L205:
  x2 = XEXP (x1, 2);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 26; }
  goto ret0;
 L258:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, QImode))
    { recog_operand[1] = x2; return 44; }
  goto ret0;
 L262:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L263; }
  goto ret0;
 L263:
  x2 = XEXP (x1, 1);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 48; }
 L268:
  if (big_immediate_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[2]) == CONST_INT 
   && (unsigned) (INTVAL (operands[2]) + 0x8000000) < 0x10000000) return 49; }
  goto ret0;
 L272:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; goto L273; }
  goto ret0;
 L273:
  x2 = XEXP (x1, 1);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 50; }
  goto ret0;
 L277:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L278; }
  goto ret0;
 L278:
  x2 = XEXP (x1, 1);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 51; }
  goto ret0;
 L282:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L283; }
  goto ret0;
 L283:
  x2 = XEXP (x1, 1);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 52; }
  goto ret0;
 L287:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; goto L288; }
  goto ret0;
 L288:
  x2 = XEXP (x1, 1);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[2] = x2; return 53; }
  goto ret0;
 L292:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SImode))
    { recog_operand[1] = x2; return 54; }
  goto ret0;
 L296:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; return 55; }
  goto ret0;
 L356:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; goto L357; }
  goto ret0;
 L357:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[2]) == CONST_INT) return 68; }
  goto ret0;
 L361:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; goto L362; }
  goto ret0;
 L362:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[2]) == CONST_INT) return 69; }
  goto ret0;
 L366:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SImode))
    { recog_operand[1] = x2; goto L367; }
  goto ret0;
 L367:
  x2 = XEXP (x1, 1);
  if (immediate_operand (x2, SImode))
    { recog_operand[2] = x2; if (GET_CODE (operands[2]) == CONST_INT) return 70; }
  goto ret0;
 L235:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, DImode))
    { recog_operand[1] = x1; return 37; }
  goto ret0;
 L238:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SFmode))
    { recog_operand[1] = x1; return 38; }
  x1 = XEXP (x0, 0);
  goto L303;
 L304:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != SFmode)
    goto ret0;
 switch (GET_CODE (x1))
  {
  case PLUS:
  if (1)
    goto L305;
  break;
  case MINUS:
  if (1)
    goto L315;
  break;
  case MULT:
  if (1)
    goto L325;
  break;
  case DIV:
  if (1)
    goto L335;
  break;
  case NEG:
  if (1)
    goto L344;
  break;
  case ABS:
  if (1)
    goto L352;
  break;
  }
  goto ret0;
 L305:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L306; }
  goto ret0;
 L306:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 57; }
  goto ret0;
 L315:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L316; }
  goto ret0;
 L316:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 59; }
  goto ret0;
 L325:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L326; }
  goto ret0;
 L326:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 61; }
  goto ret0;
 L335:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, SFmode))
    { recog_operand[1] = x2; goto L336; }
  goto ret0;
 L336:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 63; }
  goto ret0;
 L344:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_FPU) return 65; }
  goto ret0;
 L352:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, SFmode))
    { recog_operand[1] = x2; if (TARGET_FPU) return 67; }
  goto ret0;
 L229:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == DFmode && 1)
    { recog_operand[1] = x1; if (GET_CODE (operands[1]) == CONST_DOUBLE) return 35; }
 L232:
  if (general_operand (x1, DFmode))
    { recog_operand[1] = x1; return 36; }
  x1 = XEXP (x0, 0);
  goto L298;
 L299:
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) != DFmode)
    goto ret0;
 switch (GET_CODE (x1))
  {
  case PLUS:
  if (1)
    goto L300;
  break;
  case MINUS:
  if (1)
    goto L310;
  break;
  case MULT:
  if (1)
    goto L320;
  break;
  case DIV:
  if (1)
    goto L330;
  break;
  case NEG:
  if (1)
    goto L340;
  break;
  case ABS:
  if (1)
    goto L348;
  break;
  }
  goto ret0;
 L300:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L301; }
  goto ret0;
 L301:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 56; }
  goto ret0;
 L310:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L311; }
  goto ret0;
 L311:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 58; }
  goto ret0;
 L320:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L321; }
  goto ret0;
 L321:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 60; }
  goto ret0;
 L330:
  x2 = XEXP (x1, 0);
  if (register_operand (x2, DFmode))
    { recog_operand[1] = x2; goto L331; }
  goto ret0;
 L331:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, DFmode))
    { recog_operand[2] = x2; if (TARGET_FPU) return 62; }
  goto ret0;
 L340:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_FPU) return 64; }
  goto ret0;
 L348:
  x2 = XEXP (x1, 0);
  if (nonmemory_operand (x2, DFmode))
    { recog_operand[1] = x2; if (TARGET_FPU) return 66; }
  goto ret0;
 ret0: return -1;
}

int
recog (x0, insn)
     register rtx x0;
     rtx insn;
{
  register rtx x1, x2, x3, x4, x5;
  rtx x6, x7, x8, x9, x10, x11;
  int tem;
 L0:
  if (GET_CODE (x0) == SET && 1)
    goto L1;
  if (GET_CODE (x0) == PARALLEL && XVECLEN (x0, 0) == 2 && 1)
    goto L373;
  if (GET_CODE (x0) == CALL && 1)
    goto L388;
  goto ret0;
 L1:
  return recog_2 (x0, insn);
 L373:
  x1 = XVECEXP (x0, 0, 0);
  if (GET_CODE (x1) == SET && 1)
    goto L374;
  goto ret0;
 L374:
  x2 = XEXP (x1, 0);
  if (x2 == pc_rtx && 1)
    goto L375;
  goto ret0;
 L375:
  x2 = XEXP (x1, 1);
  if (register_operand (x2, SImode))
    { recog_operand[0] = x2; goto L376; }
  goto ret0;
 L376:
  x1 = XVECEXP (x0, 0, 1);
  if (GET_CODE (x1) == USE && 1)
    goto L377;
  goto ret0;
 L377:
  x2 = XEXP (x1, 0);
  if (GET_CODE (x2) == LABEL_REF && 1)
    goto L378;
  goto ret0;
 L378:
  x3 = XEXP (x2, 0);
  if (1)
    { recog_operand[1] = x3; return 76; }
  goto ret0;
 L388:
  x1 = XEXP (x0, 0);
  if (GET_CODE (x1) == MEM && GET_MODE (x1) == SImode && 1)
    goto L389;
 L380:
  if (memory_operand (x1, SImode))
    { recog_operand[0] = x1; goto L381; }
  goto ret0;
 L389:
  x2 = XEXP (x1, 0);
  if (GET_MODE (x2) == SImode && 1)
    { recog_operand[0] = x2; goto L390; }
  goto L380;
 L390:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[1] = x1; if (GET_CODE (operands[0]) == SYMBOL_REF) return 79; }
  x1 = XEXP (x0, 0);
  goto L380;
 L381:
  x1 = XEXP (x0, 1);
  if (general_operand (x1, SImode))
    { recog_operand[1] = x1; return 77; }
  goto ret0;
 ret0: return -1;
}
