if (!defined &F2C_INCLUDE) {
    eval 'sub F2C_INCLUDE {1;}';
    eval 'sub TRUE_ {(1);}';
    eval 'sub FALSE_ {(0);}';
    if (!defined &Extern) {
	eval 'sub Extern { &extern;}';
    }
    if (defined &f2c_i2) {
    }
    else {
    }
    eval 'sub VOID { &void;}';
    eval 'sub abs {
        local($x) = @_;
        eval "(($x) >= 0 ? ($x) : -($x))";
    }';
    eval 'sub dabs {
        local($x) = @_;
        eval "( &doublereal) &abs($x)";
    }';
    eval 'sub min {
        local($a,$b) = @_;
        eval "(($a) <= ($b) ? ($a) : ($b))";
    }';
    eval 'sub max {
        local($a,$b) = @_;
        eval "(($a) >= ($b) ? ($a) : ($b))";
    }';
    eval 'sub dmin {
        local($a,$b) = @_;
        eval "( &doublereal) &min($a,$b)";
    }';
    eval 'sub dmax {
        local($a,$b) = @_;
        eval "( &doublereal) &max($a,$b)";
    }';
    eval 'sub F2C_proc_par_types {1;}';
    if (defined &__cplusplus) {
    }
    else {
    }
    if (!defined &Skip_f2c_Undefs) {
    }
}
1;
