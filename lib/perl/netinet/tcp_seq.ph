if (!defined &_TCP_SEQ) {
    eval 'sub _TCP_SEQ {1;}';
    eval 'sub SEQ_LT {
        local($a,$b) = @_;
        eval "((\'int\')(($a)-($b)) < 0)";
    }';
    eval 'sub SEQ_LEQ {
        local($a,$b) = @_;
        eval "((\'int\')(($a)-($b)) <= 0)";
    }';
    eval 'sub SEQ_GT {
        local($a,$b) = @_;
        eval "((\'int\')(($a)-($b)) > 0)";
    }';
    eval 'sub SEQ_GEQ {
        local($a,$b) = @_;
        eval "((\'int\')(($a)-($b)) >= 0)";
    }';
    eval 'sub tcp_rcvseqinit {
        local($tp) = @_;
        eval "($tp)-> &rcv_adv = ($tp)-> &rcv_nxt = ($tp)-> &irs + 1";
    }';
    eval 'sub tcp_sendseqinit {
        local($tp) = @_;
        eval "($tp)-> &snd_una = ($tp)-> &snd_nxt = ($tp)-> &snd_max = ($tp)-> &snd_up = ($tp)-> &iss";
    }';
    eval 'sub TCP_ISSINCR {(125*1024);}';
    if (defined &KERNEL) {
    }
}
1;
