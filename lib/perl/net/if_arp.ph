if (!defined &_IF_ARP) {
    eval 'sub _IF_ARP {1;}';
    require 'sys/socket.ph';
    eval 'sub ARPHRD_ETHER {1;}';
    eval 'sub ARPOP_REQUEST {1;}';
    eval 'sub ARPOP_REPLY {2;}';
    eval 'sub REVARP_REQUEST {3;}';
    eval 'sub REVARP_REPLY {4;}';
    if (0) {
    }
    eval 'sub ATF_INUSE {0x01;}';
    eval 'sub ATF_COM {0x02;}';
    eval 'sub ATF_PERM {0x04;}';
    eval 'sub ATF_PUBL {0x08;}';
    eval 'sub ATF_USETRAILERS {0x10;}';
}
1;
