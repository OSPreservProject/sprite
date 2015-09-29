if (!defined &_MACHPARAM) {
    eval 'sub _MACHPARAM {1;}';
    if (!defined &_LIMITS) {
	require 'limits.ph';
    }
    eval 'sub LITTLE_ENDIAN {1234;}';
    eval 'sub BIG_ENDIAN {4321;}';
    eval 'sub PDP_ENDIAN {3412;}';
    eval 'sub BYTE_ORDER { &BIG_ENDIAN;}';
    eval 'sub WORD_ALIGN_MASK {0x3;}';
    eval 'sub PAGSIZ {0x2000;}';
    eval 'sub SEGSIZ {0x40000;}';
}
1;
