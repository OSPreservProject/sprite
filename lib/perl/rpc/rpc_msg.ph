sub RPC_MSG_VERSION {(( &u_long) 2);}
sub RPC_SERVICE_PORT {(( &u_short) 2048);}
sub ar_results { &ru. &AR_results;}
sub ar_vers { &ru. &AR_versions;}
sub rj_vers { &ru. &RJ_versions;}
sub rj_why { &ru. &RJ_why;}
sub rp_acpt { &ru. &RP_ar;}
sub rp_rjct { &ru. &RP_dr;}
sub rm_call { &ru. &RM_cmb;}
sub rm_reply { &ru. &RM_rmb;}
sub acpted_rply { &ru. &RM_rmb. &ru. &RP_ar;}
sub rjcted_rply { &ru. &RM_rmb. &ru. &RP_dr;}
1;
