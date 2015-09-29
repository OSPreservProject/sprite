if (!defined &_VARARGS) {
    eval 'sub _VARARGS {1;}';
    if (!defined &_VA_LIST) {
	eval 'sub _VA_LIST {1;}';
    }
    eval 'sub va_alist { &__builtin_va_alist;}';
    eval 'sub va_dcl {\'int\'  &__builtin_va_alist;;}';
    eval 'sub va_start {
        local($AP) = @_;
        eval "( &__builtin_saveregs(), ($AP) = (\'char\' *)& &__builtin_va_alist)";
    }';
    eval 'sub __va_rounded_size {
        local($TYPE) = @_;
        eval "((($sizeof{$TYPE} + $sizeof{\'int\'} - 1) / $sizeof{\'int\'}) * $sizeof{\'int\'})";
    }';
    eval 'sub va_arg {
        local($AP, $TYPE) = @_;
        eval "(($AP) +=  &__va_rounded_size ($TYPE), *(($TYPE *) (($AP) -  &__va_rounded_size ($TYPE))))";
    }';
    eval 'sub va_end {
        local($list) = @_;
        eval "";
    }';
}
1;
