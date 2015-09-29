if (!defined &__TYPES_RPC_HEADER__) {
    eval 'sub __TYPES_RPC_HEADER__ {1;}';
    eval 'sub bool_t {\'int\';}';
    eval 'sub enum_t {\'int\';}';
    eval 'sub FALSE {(0);}';
    eval 'sub TRUE {(1);}';
    eval 'sub __dontcare__ {-1;}';
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    eval 'sub mem_alloc {
        local($bsize) = @_;
        eval " &malloc($bsize)";
    }';
    eval 'sub mem_free {
        local($ptr, $bsize) = @_;
        eval " &free($ptr)";
    }';
    if (!defined &makedev) {
	require 'sys/types.ph';
    }
}
1;
