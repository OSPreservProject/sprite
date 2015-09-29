if (!defined &_NET_USER) {
    eval 'sub _NET_USER {1;}';
    eval 'sub NET_ADDRESS_COMPARE {
        local($a,$b) = @_;
        eval "( &bcmp((\'char\' *) &($a), (\'char\' *) &($b), $sizeof{ &Net_Address}))";
    }';
    eval 'sub NET_MAX_PROTOCOLS {2;}';
    eval 'sub NET_PROTO_RAW {0;}';
    eval 'sub NET_PROTO_INET {1;}';
    eval 'sub NET_ROUTE_VERSION {0x70500;}';
    eval 'sub NET_NUM_NETWORK_TYPES {2;}';
    eval 'sub NET_NETWORK_ETHER {(( &Net_NetworkType) 0);}';
    eval 'sub NET_NETWORK_ULTRA {(( &Net_NetworkType) 1);}';
    eval 'sub NET_FLAGS_VALID {0x1;}';
    eval 'sub NET_BROADCAST_HOSTID {0;}';
    if (!defined &KERNEL) {
    }
    if ( &BYTE_ORDER ==  &LITTLE_ENDIAN) {
    }
    else {
	eval 'sub Net_NetToHostInt {
	    local($arg) = @_;
	    eval "($arg)";
	}';
	eval 'sub Net_HostToNetInt {
	    local($arg) = @_;
	    eval "($arg)";
	}';
	eval 'sub Net_NetToHostShort {
	    local($arg) = @_;
	    eval "($arg)";
	}';
	eval 'sub Net_HostToNetShort {
	    local($arg) = @_;
	    eval "($arg)";
	}';
    }
}
1;
