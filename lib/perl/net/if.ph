if (!defined &_IF) {
    eval 'sub _IF {1;}';
    eval 'sub IFF_UP {0x1;}';
    eval 'sub IFF_BROADCAST {0x2;}';
    eval 'sub IFF_DEBUG {0x4;}';
    eval 'sub IFF_LOOPBACK {0x8;}';
    eval 'sub IFF_POINTOPOINT {0x10;}';
    eval 'sub IFF_NOTRAILERS {0x20;}';
    eval 'sub IFF_RUNNING {0x40;}';
    eval 'sub IFF_NOARP {0x80;}';
    eval 'sub IFF_PROMISC {0x100;}';
    eval 'sub IFF_ALLMULTI {0x200;}';
    eval 'sub IFF_CANTCHANGE {( &IFF_BROADCAST |  &IFF_POINTOPOINT |  &IFF_RUNNING);}';
    eval 'sub IF_QFULL {
        local($ifq) = @_;
        eval "(($ifq)-> &ifq_len >= ($ifq)-> &ifq_maxlen)";
    }';
    eval 'sub IF_DROP {
        local($ifq) = @_;
        eval "(($ifq)-> &ifq_drops++)";
    }';
    eval 'sub IF_ENQUEUE {
        local($ifq, $m) = @_;
        eval "{ ($m)-> &m_act = 0;  &if (($ifq)-> &ifq_tail == 0) ($ifq)-> &ifq_head = $m;  &else ($ifq)-> &ifq_tail-> &m_act = $m; ($ifq)-> &ifq_tail = $m; ($ifq)-> &ifq_len++; }";
    }';
    eval 'sub IF_PREPEND {
        local($ifq, $m) = @_;
        eval "{ ($m)-> &m_act = ($ifq)-> &ifq_head;  &if (($ifq)-> &ifq_tail == 0) ($ifq)-> &ifq_tail = ($m); ($ifq)-> &ifq_head = ($m); ($ifq)-> &ifq_len++; }";
    }';
    eval 'sub IF_ADJ {
        local($m) = @_;
        eval "{ ($m)-> &m_off += $sizeof{\'struct ifnet\' *}; ($m)-> &m_len -= $sizeof{\'struct ifnet\' *};  &if (($m)-> &m_len == 0) { \'struct mbuf\' * &n;  &MFREE(($m),  &n); ($m) =  &n; } }";
    }';
    eval 'sub IF_DEQUEUEIF {
        local($ifq, $m, $ifp) = @_;
        eval "{ ($m) = ($ifq)-> &ifq_head;  &if ($m) {  &if ((($ifq)-> &ifq_head = ($m)-> &m_act) == 0) ($ifq)-> &ifq_tail = 0; ($m)-> &m_act = 0; ($ifq)-> &ifq_len--; ($ifp) = *( &mtod(($m), \'struct ifnet\' **));  &IF_ADJ($m); } }";
    }';
    eval 'sub IF_DEQUEUE {
        local($ifq, $m) = @_;
        eval "{ ($m) = ($ifq)-> &ifq_head;  &if ($m) {  &if ((($ifq)-> &ifq_head = ($m)-> &m_act) == 0) ($ifq)-> &ifq_tail = 0; ($m)-> &m_act = 0; ($ifq)-> &ifq_len--; } }";
    }';
    eval 'sub IFQ_MAXLEN {50;}';
    eval 'sub IFNET_SLOWHZ {1;}';
    eval 'sub ifa_broadaddr { &ifa_ifu. &ifu_broadaddr;}';
    eval 'sub ifa_dstaddr { &ifa_ifu. &ifu_dstaddr;}';
    eval 'sub IFNAMSIZ {16;}';
    eval 'sub ifr_addr { &ifr_ifru. &ifru_addr;}';
    eval 'sub ifr_dstaddr { &ifr_ifru. &ifru_dstaddr;}';
    eval 'sub ifr_broadaddr { &ifr_ifru. &ifru_broadaddr;}';
    eval 'sub ifr_flags { &ifr_ifru. &ifru_flags;}';
    eval 'sub ifr_metric { &ifr_ifru. &ifru_metric;}';
    eval 'sub ifr_data { &ifr_ifru. &ifru_data;}';
    eval 'sub ifc_buf { &ifc_ifcu. &ifcu_buf;}';
    eval 'sub ifc_req { &ifc_ifcu. &ifcu_req;}';
    if (defined &KERNEL) {
    }
    else {
	require 'net/if_arp.ph';
    }
}
1;
