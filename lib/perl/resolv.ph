if (!defined &_RESOLV) {
    eval 'sub _RESOLV {1;}';
    eval 'sub MAXNS {3;}';
    eval 'sub MAXDNSRCH {3;}';
    eval 'sub LOCALDOMAINPARTS {2;}';
    eval 'sub RES_TIMEOUT {4;}';
    eval 'sub nsaddr { &nsaddr_list[0];}';
    eval 'sub RES_INIT {0x0001;}';
    eval 'sub RES_DEBUG {0x0002;}';
    eval 'sub RES_AAONLY {0x0004;}';
    eval 'sub RES_USEVC {0x0008;}';
    eval 'sub RES_PRIMARY {0x0010;}';
    eval 'sub RES_IGNTC {0x0020;}';
    eval 'sub RES_RECURSE {0x0040;}';
    eval 'sub RES_DEFNAMES {0x0080;}';
    eval 'sub RES_STAYOPEN {0x0100;}';
    eval 'sub RES_DNSRCH {0x0200;}';
    eval 'sub RES_DEFAULT {( &RES_RECURSE |  &RES_DEFNAMES |  &RES_DNSRCH);}';
}
1;
