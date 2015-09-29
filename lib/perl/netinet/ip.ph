if (!defined &_IP) {
    eval 'sub _IP {1;}';
    eval 'sub IPVERSION {4;}';
    if (( &BYTE_ORDER ==  &LITTLE_ENDIAN)) {
    }
    if (( &BYTE_ORDER ==  &BIG_ENDIAN)) {
    }
    eval 'sub IP_DF {0x4000;}';
    eval 'sub IP_MF {0x2000;}';
    eval 'sub IP_MAXPACKET {65535;}';
    eval 'sub IPOPT_COPIED {
        local($o) = @_;
        eval "(($o)&0x80)";
    }';
    eval 'sub IPOPT_CLASS {
        local($o) = @_;
        eval "(($o)&0x60)";
    }';
    eval 'sub IPOPT_NUMBER {
        local($o) = @_;
        eval "(($o)&0x1f)";
    }';
    eval 'sub IPOPT_CONTROL {0x00;}';
    eval 'sub IPOPT_RESERVED1 {0x20;}';
    eval 'sub IPOPT_DEBMEAS {0x40;}';
    eval 'sub IPOPT_RESERVED2 {0x60;}';
    eval 'sub IPOPT_EOL {0;}';
    eval 'sub IPOPT_NOP {1;}';
    eval 'sub IPOPT_RR {7;}';
    eval 'sub IPOPT_TS {68;}';
    eval 'sub IPOPT_SECURITY {130;}';
    eval 'sub IPOPT_LSRR {131;}';
    eval 'sub IPOPT_SATID {136;}';
    eval 'sub IPOPT_SSRR {137;}';
    eval 'sub IPOPT_OPTVAL {0;}';
    eval 'sub IPOPT_OLEN {1;}';
    eval 'sub IPOPT_OFFSET {2;}';
    eval 'sub IPOPT_MINOFF {4;}';
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN ) {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN ) {
    }
    eval 'sub IPOPT_TS_TSONLY {0;}';
    eval 'sub IPOPT_TS_TSANDADDR {1;}';
    eval 'sub IPOPT_TS_PRESPEC {2;}';
    eval 'sub IPOPT_SECUR_UNCLASS {0x0000;}';
    eval 'sub IPOPT_SECUR_CONFID {0xf135;}';
    eval 'sub IPOPT_SECUR_EFTO {0x789a;}';
    eval 'sub IPOPT_SECUR_MMMM {0xbc4d;}';
    eval 'sub IPOPT_SECUR_RESTR {0xaf13;}';
    eval 'sub IPOPT_SECUR_SECRET {0xd788;}';
    eval 'sub IPOPT_SECUR_TOPSECRET {0x6bc5;}';
    eval 'sub MAXTTL {255;}';
    eval 'sub IPFRAGTTL {60;}';
    eval 'sub IPTTLDEC {1;}';
    eval 'sub IP_MSS {576;}';
}
1;
