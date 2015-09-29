if (!defined &_NETULTRA) {
    eval 'sub _NETULTRA {1;}';
    eval 'sub Net_UltraAddressSet {
        local($addrPtr, $group, $unit) = @_;
        eval "{ (( &unsigned \'char\' *) ($addrPtr))[4] = ($group) >> 2; (( &unsigned \'char\' *) ($addrPtr))[5] = (($group) << 6) | ($unit); (( &unsigned \'char\' *) ($addrPtr))[1] = 0x49; (( &unsigned \'char\' *) ($addrPtr))[6] = 0xfe; }";
    }';
    eval 'sub Net_UltraAddressGet {
        local($addrPtr, $groupPtr, $unitPtr) = @_;
        eval "{ *($groupPtr) = ((( &unsigned \'char\' *) ($addrPtr))[4] << 2) | ((( &unsigned \'char\' *) ($addrPtr))[5] >> 6); *($unitPtr) = ((( &unsigned \'char\' *) ($addrPtr))[5] & 0x3f); }";
    }';
    eval 'sub NET_ULTRA_ADDR_SIZE {7;}';
    eval 'sub NET_ULTRA_TSAP_SIZE {4;}';
    eval 'sub Net_UltraTLWildcard {
        local($addrPtr) = @_;
        eval "{  &bzero((\'char\' *) ($addrPtr), $sizeof{ &Net_UltraTLAddress}); ($addrPtr)-> &addressSize = 7; ($addrPtr)-> &tsapSize = 4; }";
    }';
    eval 'sub NET_ULTRA_MIN_BYTES {0;}';
    eval 'sub NET_ULTRA_MAX_BYTES {(32768 + $sizeof{ &Net_UltraHeader});}';
}
1;
