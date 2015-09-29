if (!defined &_TCL) {
    eval 'sub _TCL {1;}';
    if (defined( &USE_ANSI) && defined( &__STDC__)) {
	eval 'sub _ANSI_ARGS_ {
	    local($x) = @_;
	    eval "$x";
	}';
    }
    else {
	eval 'sub _ANSI_ARGS_ {
	    local($x) = @_;
	    eval "()";
	}';
	eval 'sub const {1;}';
    }
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    if (!defined &_CLIENTDATA) {
	eval 'sub _CLIENTDATA {1;}';
    }
    eval 'sub TCL_OK {0;}';
    eval 'sub TCL_ERROR {1;}';
    eval 'sub TCL_RETURN {2;}';
    eval 'sub TCL_BREAK {3;}';
    eval 'sub TCL_CONTINUE {4;}';
    eval 'sub TCL_RESULT_SIZE {199;}';
    eval 'sub TCL_BRACKET_TERM {1;}';
    eval 'sub TCL_NO_EVAL {-1;}';
    eval 'sub TCL_STATIC {0;}';
    eval 'sub TCL_DYNAMIC {1;}';
    eval 'sub TCL_VOLATILE {2;}';
    eval 'sub TCL_TRACE_READS {1;}';
    eval 'sub TCL_TRACE_WRITES {2;}';
    eval 'sub TCL_TRACE_DELETES {4;}';
    eval 'sub TCL_VARIABLE_UNDEFINED {8;}';
}
1;
