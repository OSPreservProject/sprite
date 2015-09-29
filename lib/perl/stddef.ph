if (!defined &_STDDEF) {
    eval 'sub _STDDEF {1;}';
    if (!defined &_SIZE_T) {
	eval 'sub _SIZE_T {1;}';
    }
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    eval 'sub offsetof {
        local($structtype, $field) = @_;
        eval "(( &size_t)(\'char\' *)&((($structtype *)0)->$field))";
    }';
}
1;
