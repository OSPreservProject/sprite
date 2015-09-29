if (!defined &_TCP_FSM) {
    eval 'sub _TCP_FSM {1;}';
    eval 'sub TCP_NSTATES {11;}';
    eval 'sub TCPS_CLOSED {0;}';
    eval 'sub TCPS_LISTEN {1;}';
    eval 'sub TCPS_SYN_SENT {2;}';
    eval 'sub TCPS_SYN_RECEIVED {3;}';
    eval 'sub TCPS_ESTABLISHED {4;}';
    eval 'sub TCPS_CLOSE_WAIT {5;}';
    eval 'sub TCPS_FIN_WAIT_1 {6;}';
    eval 'sub TCPS_CLOSING {7;}';
    eval 'sub TCPS_LAST_ACK {8;}';
    eval 'sub TCPS_FIN_WAIT_2 {9;}';
    eval 'sub TCPS_TIME_WAIT {10;}';
    eval 'sub TCPS_HAVERCVDSYN {
        local($s) = @_;
        eval "(($s) >=  &TCPS_SYN_RECEIVED)";
    }';
    eval 'sub TCPS_HAVERCVDFIN {
        local($s) = @_;
        eval "(($s) >=  &TCPS_TIME_WAIT)";
    }';
    if (defined &TCPOUTFLAGS) {
    }
    if (defined &KPROF) {
    }
    if (defined &TCPSTATES) {
    }
}
1;
