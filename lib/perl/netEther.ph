if (!defined &_NETETHER) {
    eval 'sub _NETETHER {1;}';
    if (!defined &NET_ETHER_BAD_ALIGNMENT) {
	eval 'sub NET_ETHER_COMPARE {
	    local($e1,$e2) = @_;
	    eval " &NET_ETHER_COMPARE_PTR(&$e1,&$e2)";
	}';
	eval 'sub NET_ETHER_COMPARE_PTR {
	    local($e1,$e2) = @_;
	    eval "((($e1)-> &byte6 == ($e2)-> &byte6) && (($e1)-> &byte5 == ($e2)-> &byte5) && (($e1)-> &byte4 == ($e2)-> &byte4) && (($e1)-> &byte3 == ($e2)-> &byte3) && (($e1)-> &byte2 == ($e2)-> &byte2) && (($e1)-> &byte1 == ($e2)-> &byte1))";
	}';
    }
    else {
	eval 'sub NET_ETHER_COMPARE {
	    local($e1,$e2) = @_;
	    eval "( &bcmp(($e1),($e2), $sizeof{ &Net_EtherAddress})==0)";
	}';
	eval 'sub NET_ETHER_COMPARE_PTR {
	    local($e1Ptr,$e2Ptr) = @_;
	    eval " &NET_ETHER_COMPARE(*($e1Ptr),*($e2Ptr))";
	}';
    }
    if (!defined &NET_ETHER_BAD_ALIGNMENT) {
	eval 'sub NET_ETHER_ADDR_BYTE1 {
	    local($e) = @_;
	    eval "(($e). &byte1)";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE2 {
	    local($e) = @_;
	    eval "(($e). &byte2)";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE3 {
	    local($e) = @_;
	    eval "(($e). &byte3)";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE4 {
	    local($e) = @_;
	    eval "(($e). &byte4)";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE5 {
	    local($e) = @_;
	    eval "(($e). &byte5)";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE6 {
	    local($e) = @_;
	    eval "(($e). &byte6)";
	}';
	if (defined &sun4) {
	    eval 'sub NET_ETHER_ADDR_COPY {
	        local($src,$dst) = @_;
	        eval "(($dst). &byte1 = ($src). &byte1); (($dst). &byte2 = ($src). &byte2); (($dst). &byte3 = ($src). &byte3); (($dst). &byte4 = ($src). &byte4); (($dst). &byte5 = ($src). &byte5); (($dst). &byte6 = ($src). &byte6)";
	    }';
	}
	else {
	    eval 'sub NET_ETHER_ADDR_COPY {
	        local($src,$dst) = @_;
	        eval "(($dst) = ($src))";
	    }';
	}
    }
    else {
	eval 'sub NET_ETHER_ADDR_BYTE1 {
	    local($e) = @_;
	    eval "(($e)[0])";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE2 {
	    local($e) = @_;
	    eval "(($e)[1])";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE3 {
	    local($e) = @_;
	    eval "(($e)[2])";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE4 {
	    local($e) = @_;
	    eval "(($e)[3])";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE5 {
	    local($e) = @_;
	    eval "(($e)[4])";
	}';
	eval 'sub NET_ETHER_ADDR_BYTE6 {
	    local($e) = @_;
	    eval "(($e)[5])";
	}';
	eval 'sub NET_ETHER_ADDR_COPY {
	    local($src,$dst) = @_;
	    eval "( &bcopy(($src),($dst),$sizeof{ &Net_EtherAddress}))";
	}';
    }
    if (!defined &NET_ETHER_BAD_ALIGNMENT) {
	eval 'sub NET_ETHER_HDR_DESTINATION {
	    local($e) = @_;
	    eval "(($e). &destination)";
	}';
	eval 'sub NET_ETHER_HDR_SOURCE {
	    local($e) = @_;
	    eval "(($e). &source)";
	}';
	eval 'sub NET_ETHER_HDR_TYPE {
	    local($e) = @_;
	    eval "(($e). &type)";
	}';
	eval 'sub NET_ETHER_HDR_DESTINATION_PTR {
	    local($e) = @_;
	    eval "&(($e). &destination)";
	}';
	eval 'sub NET_ETHER_HDR_SOURCE_PTR {
	    local($e) = @_;
	    eval "&(($e). &source)";
	}';
	eval 'sub NET_ETHER_HDR_TYPE_PTR {
	    local($e) = @_;
	    eval "&(($e). &type)";
	}';
	eval 'sub NET_ETHER_HDR_COPY {
	    local($src, $dst) = @_;
	    eval "(($dst) = ($src))";
	}';
    }
    else {
	eval 'sub NET_ETHER_HDR_DESTINATION {
	    local($e) = @_;
	    eval "(( &unsigned \'char\' *) ($e))";
	}';
	eval 'sub NET_ETHER_HDR_SOURCE {
	    local($e) = @_;
	    eval "(( &unsigned \'char\' *) ($e+6))";
	}';
	eval 'sub NET_ETHER_HDR_TYPE {
	    local($e) = @_;
	    eval "(*(( &unsigned \'short\' *) ($e+12)))";
	}';
	eval 'sub NET_ETHER_HDR_DESTINATION_PTR {
	    local($e) = @_;
	    eval "(( &unsigned \'char\' *) ($e))";
	}';
	eval 'sub NET_ETHER_HDR_SOURCE_PTR {
	    local($e) = @_;
	    eval "(( &unsigned \'char\' *) ($e+6))";
	}';
	eval 'sub NET_ETHER_HDR_TYPE_PTR {
	    local($e) = @_;
	    eval "(*(( &unsigned \'short\' *) ($e+12)))";
	}';
	eval 'sub NET_ETHER_HDR_COPY {
	    local($src, $dst) = @_;
	    eval "( &bcopy($src,$dst,$sizeof{ &Net_EtherHdr}))";
	}';
    }
    eval 'sub NET_ETHER_MIN_BYTES {64;}';
    eval 'sub NET_ETHER_MAX_BYTES {1514;}';
    eval 'sub NET_ETHER_PUP {0x0200;}';
    eval 'sub NET_ETHER_PUP_ADDR_TRANS {0x0201;}';
    eval 'sub NET_ETHER_XNS_IDP {0x0600;}';
    eval 'sub NET_ETHER_IP {0x0800;}';
    eval 'sub NET_ETHER_ARP {0x0806;}';
    eval 'sub NET_ETHER_XNS_COMPAT {0x0807;}';
    eval 'sub NET_ETHER_SPRITE {0x0500;}';
    eval 'sub NET_ETHER_SPRITE_ARP {0x0502;}';
    eval 'sub NET_ETHER_SPRITE_DEBUG {0x0504;}';
    eval 'sub NET_ETHER_TRAIL {0x1000;}';
    eval 'sub NET_ETHER_REVARP {0x8035;}';
    eval 'sub NET_ETHER_MOP {0x6001;}';
}
1;
