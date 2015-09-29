if (!defined &_SYSTIME) {
    eval 'sub _SYSTIME {1;}';
    require 'cfuncproto.ph';
    eval 'sub DST_NONE {0;}';
    eval 'sub DST_USA {1;}';
    eval 'sub DST_AUST {2;}';
    eval 'sub DST_WET {3;}';
    eval 'sub DST_MET {4;}';
    eval 'sub DST_EET {5;}';
    eval 'sub DST_CAN {6;}';
    eval 'sub timerisset {
        local($tvp) = @_;
        eval "(($tvp)-> &tv_sec || ($tvp)-> &tv_usec)";
    }';
    eval 'sub timercmp {
        local($tvp, $uvp, $cmp) = @_;
        eval "(($tvp)-> &tv_sec $cmp ($uvp)-> &tv_sec || ($tvp)-> &tv_sec == ($uvp)-> &tv_sec && ($tvp)-> &tv_usec $cmp ($uvp)-> &tv_usec)";
    }';
    eval 'sub timerclear {
        local($tvp) = @_;
        eval "($tvp)-> &tv_sec = ($tvp)-> &tv_usec = 0";
    }';
    eval 'sub ITIMER_REAL {0;}';
    eval 'sub ITIMER_VIRTUAL {1;}';
    eval 'sub ITIMER_PROF {2;}';
    if (!defined &KERNEL) {
	require 'time.ph';
    }
}
1;
