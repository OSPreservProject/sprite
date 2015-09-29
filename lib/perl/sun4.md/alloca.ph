if (!defined &_ALLOCA_H) {
    eval 'sub _ALLOCA_H {1;}';
    if (defined &sun4) {
	eval 'sub alloca {
	    local($p) = @_;
	    eval "((\'char\' *)  &__builtin_alloca($p))";
	}';
    }
}
1;
