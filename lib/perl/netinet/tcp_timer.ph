if (!defined &_TCP_TIMER) {
    eval 'sub _TCP_TIMER {1;}';
    eval 'sub TCPT_NTIMERS {4;}';
    eval 'sub TCPT_REXMT {0;}';
    eval 'sub TCPT_PERSIST {1;}';
    eval 'sub TCPT_KEEP {2;}';
    eval 'sub TCPT_2MSL {3;}';
    eval 'sub TCP_TTL {30;}';
    eval 'sub TCPTV_MSL {( 30* &PR_SLOWHZ);}';
    eval 'sub TCPTV_SRTTBASE {0;}';
    eval 'sub TCPTV_SRTTDFLT {( 3* &PR_SLOWHZ);}';
    eval 'sub TCPTV_PERSMIN {( 5* &PR_SLOWHZ);}';
    eval 'sub TCPTV_PERSMAX {( 60* &PR_SLOWHZ);}';
    eval 'sub TCPTV_KEEP_INIT {( 75* &PR_SLOWHZ);}';
    eval 'sub TCPTV_KEEP_IDLE {(120*60* &PR_SLOWHZ);}';
    eval 'sub TCPTV_KEEPINTVL {( 75* &PR_SLOWHZ);}';
    eval 'sub TCPTV_KEEPCNT {8;}';
    eval 'sub TCPTV_MIN {( 1* &PR_SLOWHZ);}';
    eval 'sub TCPTV_REXMTMAX {( 64* &PR_SLOWHZ);}';
    eval 'sub TCP_LINGERTIME {120;}';
    eval 'sub TCP_MAXRXTSHIFT {12;}';
    if (defined &TCPTIMERS) {
    }
    eval 'sub TCPT_RANGESET {
        local($tv, $value, $tvmin, $tvmax) = @_;
        eval "{ ($tv) = ($value);  &if (($tv) < ($tvmin)) ($tv) = ($tvmin);  &else  &if (($tv) > ($tvmax)) ($tv) = ($tvmax); }";
    }';
    if (defined &KERNEL) {
    }
}
1;
