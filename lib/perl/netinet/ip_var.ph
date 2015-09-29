if (!defined &_IP_VAR) {
    eval 'sub _IP_VAR {1;}';
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    eval 'sub MAX_IPOPTLEN {40;}';
    if (defined &KERNEL) {
	eval 'sub IP_FORWARDING {0x1;}';
	eval 'sub IP_ROUTETOIF { &SO_DONTROUTE;}';
	eval 'sub IP_ALLOWBROADCAST { &SO_BROADCAST;}';
    }
}
1;
