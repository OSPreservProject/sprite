if (!defined &_TCP) {
    eval 'sub _TCP {1;}';
    if (!defined &BYTE_ORDER) {
	eval 'sub LITTLE_ENDIAN {1234;}';
	eval 'sub BIG_ENDIAN {4321;}';
	eval 'sub PDP_ENDIAN {3412;}';
	if (defined &vax) {
	    eval 'sub BYTE_ORDER { &LITTLE_ENDIAN;}';
	}
	else {
	    eval 'sub BYTE_ORDER { &BIG_ENDIAN;}';
	}
    }
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    eval 'sub TH_FIN {0x01;}';
    eval 'sub TH_SYN {0x02;}';
    eval 'sub TH_RST {0x04;}';
    eval 'sub TH_PUSH {0x08;}';
    eval 'sub TH_ACK {0x10;}';
    eval 'sub TH_URG {0x20;}';
    eval 'sub TCPOPT_EOL {0;}';
    eval 'sub TCPOPT_NOP {1;}';
    eval 'sub TCPOPT_MAXSEG {2;}';
    if (defined &lint) {
	eval 'sub TCP_MSS {536;}';
    }
    else {
	if (!defined &IP_MSS) {
	    eval 'sub IP_MSS {576;}';
	}
	eval 'sub TCP_MSS { &MIN(512,  &IP_MSS - $sizeof{\'struct tcpiphdr\'});}';
    }
    eval 'sub TCP_NODELAY {0x01;}';
    eval 'sub TCP_MAXSEG {0x02;}';
}
1;
