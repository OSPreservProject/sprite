if (!defined &_STDLIB) {
    eval 'sub _STDLIB {1;}';
    require 'cfuncproto.ph';
    if (defined &KERNEL) {
	require 'sprite.ph';
	require 'sys/types.ph';
    }
    eval 'sub EXIT_SUCCESS {0;}';
    eval 'sub EXIT_FAILURE {1;}';
    if (defined( &_HAS_PROTOTYPES) && !defined( &_SIZE_T)) {
	eval 'sub _SIZE_T {1;}';
    }
    if (defined &KERNEL) {
	if (!defined &mips) {
	    if (defined &lint) {
		eval 'sub free {
		    local($ptr) = @_;
		    eval " &_free($ptr)";
		}';
	    }
	    else {
		eval 'sub free {
		    local($ptr) = @_;
		    eval "{ &_free($ptr); ($ptr) = ( &Address)  &NIL; }";
		}';
	    }
	}
	else {
	}
    }
    else {
    }
    eval 'sub MEM_PRINT_TRACE {0x1;}';
    eval 'sub MEM_STORE_TRACE {0x2;}';
    eval 'sub MEM_DONT_USE_ORIG_SIZE {0x4;}';
    eval 'sub MEM_TRACE_NOT_INIT {0x8;}';
}
1;
