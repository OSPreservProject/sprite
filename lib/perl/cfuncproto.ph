if (!defined &_CFUNCPROTO) {
    eval 'sub _CFUNCPROTO {1;}';
    if (!defined &_ASM) {
	if (defined( &KERNEL) && defined( &__STDC__)) {
	    eval 'sub _HAS_PROTOTYPES {1;}';
	    eval 'sub _HAS_VOIDPTR {1;}';
	}
	if (defined( &__cplusplus)) {
	    eval 'sub _EXTERN { &extern "C";}';
	    eval 'sub _NULLARGS {( &void);}';
	    eval 'sub _HAS_PROTOTYPES {1;}';
	    eval 'sub _HAS_VOIDPTR {1;}';
	    eval 'sub _HAS_CONST {1;}';
	}
	else {
	    eval 'sub _EXTERN { &extern;}';
	    eval 'sub _NULLARGS {();}';
	}
	if (defined( &_HAS_PROTOTYPES) && !defined( &lint)) {
	    eval 'sub _ARGS_ {
	        local($x) = @_;
	        eval "$x";
	    }';
	}
	else {
	    eval 'sub _ARGS_ {
	        local($x) = @_;
	        eval "()";
	    }';
	}
	if (defined &_HAS_CONST) {
	    eval 'sub _CONST { &const;}';
	}
	else {
	    eval 'sub _CONST {1;}';
	}
	if (defined &_HAS_VOIDPTR) {
	}
	else {
	}
    }
}
1;
