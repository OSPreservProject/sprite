if (!defined &_TCPIP) {
    eval 'sub _TCPIP {1;}';
    eval 'sub ti_next { &ti_i. &ih_next;}';
    eval 'sub ti_prev { &ti_i. &ih_prev;}';
    eval 'sub ti_x1 { &ti_i. &ih_x1;}';
    eval 'sub ti_pr { &ti_i. &ih_pr;}';
    eval 'sub ti_len { &ti_i. &ih_len;}';
    eval 'sub ti_src { &ti_i. &ih_src;}';
    eval 'sub ti_dst { &ti_i. &ih_dst;}';
    eval 'sub ti_sport { &ti_t. &th_sport;}';
    eval 'sub ti_dport { &ti_t. &th_dport;}';
    eval 'sub ti_seq { &ti_t. &th_seq;}';
    eval 'sub ti_ack { &ti_t. &th_ack;}';
    eval 'sub ti_x2 { &ti_t. &th_x2;}';
    eval 'sub ti_off { &ti_t. &th_off;}';
    eval 'sub ti_flags { &ti_t. &th_flags;}';
    eval 'sub ti_win { &ti_t. &th_win;}';
    eval 'sub ti_sum { &ti_t. &th_sum;}';
    eval 'sub ti_urp { &ti_t. &th_urp;}';
}
1;
