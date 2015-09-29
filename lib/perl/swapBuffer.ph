if (!defined &_SWAP) {
    eval 'sub _SWAP {1;}';
    eval 'sub SWAP_SUN_TYPE {0;}';
    eval 'sub SWAP_VAX_TYPE {1;}';
    eval 'sub SWAP_SPUR_TYPE {2;}';
    eval 'sub SWAP_SPARC_TYPE { &SWAP_SUN_TYPE;}';
}
1;
