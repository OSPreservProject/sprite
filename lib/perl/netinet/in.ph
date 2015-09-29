if (!defined &_IN) {
    eval 'sub _IN {1;}';
    if (!defined &_MACHPARAM) {
	require 'machparam.ph';
    }
    eval 'sub IPPROTO_IP {0;}';
    eval 'sub IPPROTO_ICMP {1;}';
    eval 'sub IPPROTO_GGP {3;}';
    eval 'sub IPPROTO_TCP {6;}';
    eval 'sub IPPROTO_EGP {8;}';
    eval 'sub IPPROTO_PUP {12;}';
    eval 'sub IPPROTO_UDP {17;}';
    eval 'sub IPPROTO_IDP {22;}';
    eval 'sub IPPROTO_SPRITE {90;}';
    eval 'sub IPPROTO_RAW {255;}';
    eval 'sub IPPROTO_MAX {256;}';
    eval 'sub IPPORT_RESERVED {1024;}';
    eval 'sub IPPORT_USERRESERVED {5000;}';
    eval 'sub IMPLINK_IP {155;}';
    eval 'sub IMPLINK_LOWEXPER {156;}';
    eval 'sub IMPLINK_HIGHEXPER {158;}';
    eval 'sub IN_CLASSA {
        local($i) = @_;
        eval "(((\'long\')($i) & 0x80000000) == 0)";
    }';
    eval 'sub IN_CLASSA_NET {0xff000000;}';
    eval 'sub IN_CLASSA_NSHIFT {24;}';
    eval 'sub IN_CLASSA_HOST {0x00ffffff;}';
    eval 'sub IN_CLASSA_MAX {128;}';
    eval 'sub IN_CLASSB {
        local($i) = @_;
        eval "(((\'long\')($i) & 0xc0000000) == 0x80000000)";
    }';
    eval 'sub IN_CLASSB_NET {0xffff0000;}';
    eval 'sub IN_CLASSB_NSHIFT {16;}';
    eval 'sub IN_CLASSB_HOST {0x0000ffff;}';
    eval 'sub IN_CLASSB_MAX {65536;}';
    eval 'sub IN_CLASSC {
        local($i) = @_;
        eval "(((\'long\')($i) & 0xe0000000) == 0xc0000000)";
    }';
    eval 'sub IN_CLASSC_NET {0xffffff00;}';
    eval 'sub IN_CLASSC_NSHIFT {8;}';
    eval 'sub IN_CLASSC_HOST {0x000000ff;}';
    eval 'sub IN_CLASSD {
        local($i) = @_;
        eval "(((\'long\')($i) & 0xf0000000) == 0xe0000000)";
    }';
    eval 'sub IN_MULTICAST {
        local($i) = @_;
        eval " &IN_CLASSD($i)";
    }';
    eval 'sub IN_EXPERIMENTAL {
        local($i) = @_;
        eval "(((\'long\')($i) & 0xe0000000) == 0xe0000000)";
    }';
    eval 'sub IN_BADCLASS {
        local($i) = @_;
        eval "(((\'long\')($i) & 0xf0000000) == 0xf0000000)";
    }';
    eval 'sub INADDR_ANY {( &u_long)0x00000000;}';
    eval 'sub INADDR_BROADCAST {( &u_long)0xffffffff;}';
    if (!defined &KERNEL) {
	eval 'sub INADDR_NONE {0xffffffff;}';
    }
    eval 'sub IN_LOOPBACKNET {127;}';
    eval 'sub IP_OPTIONS {1;}';
    if ( &BYTE_ORDER ==  &BIG_ENDIAN && !defined( &lint)) {
	eval 'sub ntohl {
	    local($x) = @_;
	    eval "($x)";
	}';
	eval 'sub ntohs {
	    local($x) = @_;
	    eval "($x)";
	}';
	eval 'sub htonl {
	    local($x) = @_;
	    eval "($x)";
	}';
	eval 'sub htons {
	    local($x) = @_;
	    eval "($x)";
	}';
    }
    else {
    }
}
1;
