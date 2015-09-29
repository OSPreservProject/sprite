if (!defined &_UDP_VAR) {
    eval 'sub _UDP_VAR {1;}';
    eval 'sub ui_next { &ui_i. &ih_next;}';
    eval 'sub ui_prev { &ui_i. &ih_prev;}';
    eval 'sub ui_x1 { &ui_i. &ih_x1;}';
    eval 'sub ui_pr { &ui_i. &ih_pr;}';
    eval 'sub ui_len { &ui_i. &ih_len;}';
    eval 'sub ui_src { &ui_i. &ih_src;}';
    eval 'sub ui_dst { &ui_i. &ih_dst;}';
    eval 'sub ui_sport { &ui_u. &uh_sport;}';
    eval 'sub ui_dport { &ui_u. &uh_dport;}';
    eval 'sub ui_ulen { &ui_u. &uh_ulen;}';
    eval 'sub ui_sum { &ui_u. &uh_sum;}';
    eval 'sub UDP_TTL {30;}';
    if (defined &KERNEL) {
    }
}
1;
