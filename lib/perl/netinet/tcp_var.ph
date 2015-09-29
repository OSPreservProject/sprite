if (!defined &_TCP_VAR) {
    eval 'sub _TCP_VAR {1;}';
    if (!defined &BSD) {
	eval 'sub BSD {42;}';
	eval 'sub OLDSTAT {1;}';
    }
    if ( &sun ||  &BSD < 43) {
	eval 'sub TCP_COMPAT_42 {1;}';
    }
    if (!defined &SB_MAX) {
	if (defined &SB_MAXCOUNT) {
	    eval 'sub SB_MAX { &SB_MAXCOUNT;}';
	}
	else {
	    eval 'sub SB_MAX {32767;}';
	}
    }
    if (!defined &IP_MAXPACKET) {
	eval 'sub IP_MAXPACKET {65535;}';
    }
    if (!defined &MCLBYTES) {
	eval 'sub MCLBYTES { &CLBYTES;}';
    }
    if (defined &sun) {
	eval 'sub in_localaddr { &tcp_in_localaddr;}';
    }
    eval 'sub TF_ACKNOW {0x01;}';
    eval 'sub TF_DELACK {0x02;}';
    eval 'sub TF_NODELAY {0x04;}';
    eval 'sub TF_NOOPT {0x08;}';
    eval 'sub TF_SENTFIN {0x10;}';
    eval 'sub TCPOOB_HAVEDATA {0x01;}';
    eval 'sub TCPOOB_HADDATA {0x02;}';
    eval 'sub intotcpcb {
        local($ip) = @_;
        eval "((\'struct tcpcb\' *)($ip)-> &inp_ppcb)";
    }';
    eval 'sub sototcpcb {
        local($so) = @_;
        eval "( &intotcpcb( &sotoinpcb($so)))";
    }';
    if (defined &OLDSTAT) {
	eval 'sub tcps_badsum { &tcps_rcvbadsum;}';
	eval 'sub tcps_badoff { &tcps_rcvbadoff;}';
	eval 'sub tcps_hdrops { &tcps_rcvshort;}';
    }
    if (!defined &OLDSTAT) {
    }
    if (defined &KERNEL) {
    }
}
1;
