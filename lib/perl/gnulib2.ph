require 'stddef.ph';
if (!defined &SItype) {
    eval 'sub SItype {\'long\' \'int\';}';
}
if (defined &WORDS_BIG_ENDIAN) {
}
else {
}
sub B {0x10000;}
sub low16 {( &B - 1);}
if (defined &BYTES_BIG_ENDIAN) {
    eval 'sub HIGH {0;}';
    eval 'sub LOW {1;}';
    eval 'sub big_end {
        local($n) = @_;
        eval "0";
    }';
    eval 'sub little_end {
        local($n) = @_;
        eval "(($n) - 1)";
    }';
    eval 'sub next_msd {
        local($i) = @_;
        eval "(($i) - 1)";
    }';
    eval 'sub next_lsd {
        local($i) = @_;
        eval "(($i) + 1)";
    }';
    eval 'sub is_not_msd {
        local($i,$n) = @_;
        eval "(($i) >= 0)";
    }';
    eval 'sub is_not_lsd {
        local($i,$n) = @_;
        eval "(($i) < ($n))";
    }';
}
else {
    eval 'sub LOW {0;}';
    eval 'sub HIGH {1;}';
    eval 'sub big_end {
        local($n) = @_;
        eval "(($n) - 1)";
    }';
    eval 'sub little_end {
        local($n) = @_;
        eval "0";
    }';
    eval 'sub next_msd {
        local($i) = @_;
        eval "(($i) + 1)";
    }';
    eval 'sub next_lsd {
        local($i) = @_;
        eval "(($i) - 1)";
    }';
    eval 'sub is_not_msd {
        local($i,$n) = @_;
        eval "(($i) < ($n))";
    }';
    eval 'sub is_not_lsd {
        local($i,$n) = @_;
        eval "(($i) >= 0)";
    }';
}
if (defined &L_adddi3) {
}
if (defined &L_anddi3) {
}
if (defined &L_iordi3) {
}
if (defined &L_xordi3) {
}
if (defined &L_one_cmpldi2) {
}
if (defined &L_lshldi3) {
}
if (defined &L_lshrdi3) {
}
if (defined &L_ashldi3) {
}
if (defined &L_ashrdi3) {
}
if (defined &L_subdi3) {
}
if (defined &L_muldi3) {
}
if (defined &L_divdi3) {
}
if (defined &L_moddi3) {
}
if (defined &L_udivdi3) {
}
if (defined &L_umoddi3) {
}
if (defined &L_negdi2) {
}
if (defined &L_bdiv) {
}
if (defined &L_cmpdi2) {
}
if (defined &L_ucmpdi2) {
}
if (defined &L_fixunsdfdi) {
    eval 'sub HIGH_WORD_COEFF {(((\'long\' \'long\') 1) <<  &BITS_PER_WORD);}';
}
if (defined &L_fixdfdi) {
}
if (defined &L_floatdidf) {
    eval 'sub HIGH_HALFWORD_COEFF {(((\'long\' \'long\') 1) << ( &BITS_PER_WORD / 2));}';
    eval 'sub HIGH_WORD_COEFF {(((\'long\' \'long\') 1) <<  &BITS_PER_WORD);}';
}
1;
