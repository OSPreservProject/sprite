if (!defined &_NETINET) {
    eval 'sub _NETINET {1;}';
    eval 'sub NET_ARP_TYPE_ETHER {1;}';
    eval 'sub NET_ARP_REQUEST {1;}';
    eval 'sub NET_ARP_REPLY {2;}';
    eval 'sub NET_RARP_REQUEST {3;}';
    eval 'sub NET_RARP_REPLY {4;}';
    eval 'sub NET_INET_BROADCAST_ADDR {(( &Net_InetAddress) 0xFFFFFFFF);}';
    eval 'sub NET_INET_ANY_ADDR {(( &Net_InetAddress) 0);}';
    eval 'sub NET_INET_CLASS_A_ADDR {
        local($addr) = @_;
        eval "((( &unsigned \'int\')($addr) & 0x80000000) == 0)";
    }';
    eval 'sub NET_INET_CLASS_A_HOST_MASK {0x00FFFFFF;}';
    eval 'sub NET_INET_CLASS_A_NET_MASK {0xFF000000;}';
    eval 'sub NET_INET_CLASS_A_SHIFT {24;}';
    eval 'sub NET_INET_CLASS_B_ADDR {
        local($addr) = @_;
        eval "((( &unsigned \'int\')($addr) & 0xC0000000) == 0x80000000)";
    }';
    eval 'sub NET_INET_CLASS_B_HOST_MASK {0x0000FFFF;}';
    eval 'sub NET_INET_CLASS_B_NET_MASK {0xFFFF0000;}';
    eval 'sub NET_INET_CLASS_B_SHIFT {16;}';
    eval 'sub NET_INET_CLASS_C_ADDR {
        local($addr) = @_;
        eval "((( &unsigned \'int\')($addr) & 0xE0000000) == 0xC0000000)";
    }';
    eval 'sub NET_INET_CLASS_C_HOST_MASK {0x000000FF;}';
    eval 'sub NET_INET_CLASS_C_NET_MASK {0xFFFFFF00;}';
    eval 'sub NET_INET_CLASS_C_SHIFT {8;}';
    eval 'sub NET_INET_CLASS_D_ADDR {
        local($addr) = @_;
        eval "((( &unsigned \'int\')($addr) & 0xF0000000) == 0xE0000000)";
    }';
    eval 'sub NET_INET_CLASS_D_NET_MASK {0x0FFFFFFF;}';
    eval 'sub NET_INET_CLASS_E_ADDR {
        local($addr) = @_;
        eval "((( &unsigned \'int\')($addr) & 0xF0000000) == 0xF0000000)";
    }';
    eval 'sub NET_INET_LOCAL_HOST {0x7F000001;}';
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    else {
    }
    eval 'sub NET_IP_VERSION {4;}';
    eval 'sub NET_IP_MAX_HDR_SIZE {60;}';
    eval 'sub NET_IP_MAX_TTL {255;}';
    eval 'sub NET_IP_MAX_FRAG_TTL {15;}';
    eval 'sub NET_IP_TTL_DECR {1;}';
    eval 'sub NET_IP_MAX_SEG_SIZE {576;}';
    eval 'sub NET_IP_MAX_PACKET_SIZE {65535;}';
    eval 'sub NET_IP_SERV_PREC_NET_CTL {0xE0;}';
    eval 'sub NET_IP_SERV_PREC_INET_CTL {0xC0;}';
    eval 'sub NET_IP_SERV_PREC_CRITIC {0xA0;}';
    eval 'sub NET_IP_SERV_PREC_FLASH_OVR {0x80;}';
    eval 'sub NET_IP_SERV_PREC_FLASH {0x60;}';
    eval 'sub NET_IP_SERV_PREC_IMMED {0x40;}';
    eval 'sub NET_IP_SERV_PREC_PRIORITY {0x20;}';
    eval 'sub NET_IP_SERV_PREC_ROUTINE {0x00;}';
    eval 'sub NET_IP_SERV_NORM_DELAY {0x00;}';
    eval 'sub NET_IP_SERV_LOW_DELAY {0x10;}';
    eval 'sub NET_IP_SERV_NORM_THRUPUT {0x00;}';
    eval 'sub NET_IP_SERV_HIGH_THRUPUT {0x08;}';
    eval 'sub NET_IP_SERV_NORM_RELIABL {0x00;}';
    eval 'sub NET_IP_SERV_HIGH_RELIABL {0x04;}';
    eval 'sub NET_IP_LAST_FRAG {0x0;}';
    eval 'sub NET_IP_MORE_FRAGS {0x1;}';
    eval 'sub NET_IP_DONT_FRAG {0x2;}';
    eval 'sub NET_IP_PROTOCOL_IP {0;}';
    eval 'sub NET_IP_PROTOCOL_ICMP {1;}';
    eval 'sub NET_IP_PROTOCOL_TCP {6;}';
    eval 'sub NET_IP_PROTOCOL_EGP {8;}';
    eval 'sub NET_IP_PROTOCOL_UDP {17;}';
    eval 'sub NET_IP_PROTOCOL_SPRITE {90;}';
    eval 'sub NET_IP_OPT_TYPE_OFFSET {0;}';
    eval 'sub NET_IP_OPT_LEN_OFFSET {1;}';
    eval 'sub NET_IP_OPT_PTR_OFFSET {2;}';
    eval 'sub NET_IP_OPT_MIN_PTR {4;}';
    eval 'sub NET_IP_OPT_MAX_LEN {40;}';
    eval 'sub NET_IP_OPT_COPIED {
        local($opt) = @_;
        eval "(($opt) & 0x80)";
    }';
    eval 'sub NET_IP_OPT_CLASS {
        local($opt) = @_;
        eval "(($opt) & 0x60)";
    }';
    eval 'sub NET_IP_OPT_NUMBER {
        local($opt) = @_;
        eval "(($opt) & 0x1f)";
    }';
    eval 'sub NET_IP_OPT_CLASS_CONTROL {0x00;}';
    eval 'sub NET_IP_OPT_CLASS_RESERVED1 {0x20;}';
    eval 'sub NET_IP_OPT_CLASS_DEBUG {0x40;}';
    eval 'sub NET_IP_OPT_CLASS_RESERVED2 {0x60;}';
    eval 'sub NET_IP_OPT_END_OF_LIST {0x00;}';
    eval 'sub NET_IP_OPT_NOP {0x01;}';
    eval 'sub NET_IP_OPT_SECURITY {0x82;}';
    eval 'sub NET_IP_OPT_LOOSE_ROUTE {0x83;}';
    eval 'sub NET_IP_OPT_STRICT_ROUTE {0x89;}';
    eval 'sub NET_IP_OPT_REC_ROUTE {0x07;}';
    eval 'sub NET_IP_OPT_STREAM_ID {0x88;}';
    eval 'sub NET_IP_OPT_TIMESTAMP {0x44;}';
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    else {
    }
    eval 'sub NET_IP_OPT_TS_ONLY {0;}';
    eval 'sub NET_IP_OPT_TS_AND_ADDR {1;}';
    eval 'sub NET_IP_OPT_TS_ADDR_SPEC {3;}';
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    else {
    }
    eval 'sub NET_TCP_MAX_HDR_SIZE {60;}';
    eval 'sub NET_TCP_TTL {30;}';
    eval 'sub NET_TCP_FIN_FLAG {0x01;}';
    eval 'sub NET_TCP_SYN_FLAG {0x02;}';
    eval 'sub NET_TCP_RST_FLAG {0x04;}';
    eval 'sub NET_TCP_PSH_FLAG {0x08;}';
    eval 'sub NET_TCP_ACK_FLAG {0x10;}';
    eval 'sub NET_TCP_URG_FLAG {0x20;}';
    eval 'sub NET_TCP_OPTION_EOL {0x0;}';
    eval 'sub NET_TCP_OPTION_NOP {0x1;}';
    eval 'sub NET_TCP_OPTION_MAX_SEG_SIZE {0x2;}';
    eval 'sub NET_TCP_MAX_SEG_SIZE {512;}';
    eval 'sub NET_UDP_TTL {30;}';
    eval 'sub NET_ICMP_MIN_LEN {8;}';
    eval 'sub NET_ICMP_ECHO_REPLY {0;}';
    eval 'sub NET_ICMP_UNREACHABLE {3;}';
    eval 'sub NET_ICMP_UNREACH_NET {0;}';
    eval 'sub NET_ICMP_UNREACH_HOST {1;}';
    eval 'sub NET_ICMP_UNREACH_PROTOCOL {2;}';
    eval 'sub NET_ICMP_UNREACH_PORT {3;}';
    eval 'sub NET_ICMP_UNREACH_NEED_FRAG {4;}';
    eval 'sub NET_ICMP_UNREACH_SRC_ROUTE {5;}';
    eval 'sub NET_ICMP_SOURCE_QUENCH {4;}';
    eval 'sub NET_ICMP_REDIRECT {5;}';
    eval 'sub NET_ICMP_REDIRECT_NET {0;}';
    eval 'sub NET_ICMP_REDIRECT_HOST {1;}';
    eval 'sub NET_ICMP_REDIRECT_TOS_NET {2;}';
    eval 'sub NET_ICMP_REDIRECT_TOS_HOST {3;}';
    eval 'sub NET_ICMP_ECHO {8;}';
    eval 'sub NET_ICMP_TIME_EXCEED {11;}';
    eval 'sub NET_ICMP_TIME_EXCEED_TTL {0;}';
    eval 'sub NET_ICMP_TIME_EXCEED_REASS {1;}';
    eval 'sub NET_ICMP_PARAM_PROB {12;}';
    eval 'sub NET_ICMP_TIMESTAMP {13;}';
    eval 'sub NET_ICMP_TIMESTAMP_REPLY {14;}';
    eval 'sub NET_ICMP_INFO_REQ {15;}';
    eval 'sub NET_ICMP_INFO_REPLY {16;}';
    eval 'sub NET_ICMP_MASK_REQ {17;}';
    eval 'sub NET_ICMP_MASK_REPLY {18;}';
    eval 'sub NET_ICMP_MAX_TYPE {18;}';
}
1;
