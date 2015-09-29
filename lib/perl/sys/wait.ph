if (!defined &_WAIT) {
    eval 'sub _WAIT {1;}';
#
# changed 11/4/91 by voelker from "require 'machine/machparam.ph'"
#
    require '/sprite/lib/perl/machparam.ph';    
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    else {
    }
#
# changed 11/10/91 by voelker
#
#    eval 'sub w_termsig { &w_T. &w_Termsig;}';
#    eval 'sub w_coredump { &w_T. &w_Coredump;}';
#    eval 'sub w_retcode { &w_T. &w_Retcode;}';
#    eval 'sub w_stopval { &w_S. &w_Stopval;}';
#    eval 'sub w_stopsig { &w_S. &w_Stopsig;}';
    eval 'sub w_termsig { 
	local ($x) = @_;
	$x & 0x7F;
    }';
    eval 'sub w_coredump {
	local ($x) = @_;
	($x & 0x80) >> 7;
    }';
    eval 'sub w_retcode {
	local ($x) = @_;
	($x & 0xFF00) >> 8;
    }';
    eval 'sub w_stopval {
	local ($x) = @_;
	($x & 0xFF0000) >> 16;
    }';
    eval 'sub w_stopsig {
	local ($x) = @_;
	($x & 0xFF000000) >> 24;
    }';
    eval 'sub WSTOPPED {0177;}';
    eval 'sub WNOHANG {1;}';
    eval 'sub WUNTRACED {2;}';
    eval 'sub WIFSTOPPED {
        local($x) = @_;
        eval "(&w_stopval($x) ==  &WSTOPPED)";
    }';
    eval 'sub WIFSIGNALED {
        local($x) = @_;
        eval "(&w_stopval($x) !=  &WSTOPPED && &w_termsig($x) != 0)";
    }';
    eval 'sub WIFEXITED {
        local($x) = @_;
        eval "(&w_stopval($x) !=  &WSTOPPED && &w_termsig($x) == 0)";
    }';
}
1;



