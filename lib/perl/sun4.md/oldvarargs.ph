if (!defined &_VARARGS) {
    eval 'sub _VARARGS {1;}';
    eval 'sub va_alist { &__builtin_va_alist;}';
    eval 'sub va_dcl {\'int\'  &__builtin_va_alist;;}';
    eval 'sub va_start {
        local($list) = @_;
        eval "($list). &vl_current = ($list). &vl_next = (\'char\' *) & &__builtin_va_alist;";
    }';
    eval 'sub va_arg {
        local($list, $type) = @_;
        eval "(($list). &vl_current = ($list). &vl_next, ($list). &vl_next += $sizeof{$type}, *(($type *) ($list). &vl_current))";
    }';
    eval 'sub va_end {
        local($list) = @_;
        eval "";
    }';
}
1;
