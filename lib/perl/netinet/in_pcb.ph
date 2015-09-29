if (!defined &_IN_PCB) {
    eval 'sub _IN_PCB {1;}';
    eval 'sub INPLOOKUP_WILDCARD {1;}';
    eval 'sub INPLOOKUP_SETLOCAL {2;}';
    eval 'sub sotoinpcb {
        local($so) = @_;
        eval "((\'struct inpcb\' *)($so)-> &so_pcb)";
    }';
    if (defined &KERNEL) {
    }
}
1;
