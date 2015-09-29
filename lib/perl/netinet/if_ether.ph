if (!defined &_IF_ETHER) {
    eval 'sub _IF_ETHER {1;}';
    require 'net/if_arp.ph';
    eval 'sub ETHERTYPE_PUP {0x0200;}';
    eval 'sub ETHERTYPE_IP {0x0800;}';
    eval 'sub ETHERTYPE_ARP {0x0806;}';
    eval 'sub ETHERTYPE_RARP {0x8035;}';
    eval 'sub ETHERTYPE_TRAIL {0x1000;}';
    eval 'sub ETHERTYPE_NTRAILER {16;}';
    eval 'sub ETHERMTU {1500;}';
    eval 'sub ETHERMIN {(60-14);}';
    eval 'sub arp_hrd { &ea_hdr. &ar_hrd;}';
    eval 'sub arp_pro { &ea_hdr. &ar_pro;}';
    eval 'sub arp_hln { &ea_hdr. &ar_hln;}';
    eval 'sub arp_pln { &ea_hdr. &ar_pln;}';
    eval 'sub arp_op { &ea_hdr. &ar_op;}';
}
1;
