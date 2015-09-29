if (!defined &_RAW_CB) {
    eval 'sub _RAW_CB {1;}';
    eval 'sub RAW_LADDR {01;}';
    eval 'sub RAW_FADDR {02;}';
    eval 'sub RAW_DONTROUTE {04;}';
    eval 'sub sotorawcb {
        local($so) = @_;
        eval "((\'struct rawcb\' *)($so)-> &so_pcb)";
    }';
    eval 'sub RAWSNDQ {2048;}';
    eval 'sub RAWRCVQ {2048;}';
    if (defined &KERNEL) {
    }
}
1;
