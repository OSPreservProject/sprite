if (!defined &_STDARG_H) {
    eval 'sub _STDARG_H {1;}';
    if (!defined &_VA_LIST) {
	eval 'sub _VA_LIST {1;}';
    }
    eval 'sub __va_rounded_size {
        local($TYPE) = @_;
        eval "((($sizeof{$TYPE} + $sizeof{\'int\'} - 1) / $sizeof{\'int\'}) * $sizeof{\'int\'})";
    }';
    eval 'sub va_start {
        local($AP, $lastarg) = @_;
        eval "( &__builtin_saveregs(), ($AP) = ((\'char\' *)&$lastarg +  &__va_rounded_size($lastarg)))";
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
