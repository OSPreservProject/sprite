if (!defined &_BIT) {
    eval 'sub _BIT {1;}';
    if (!defined &_SPRITE) {
    }
    require 'cfuncproto.ph';
    require 'bstring.ph';
    eval 'sub BIT_NUM_BITS_PER_INT {32;}';
    eval 'sub BIT_NUM_BITS_PER_BYTE {8;}';
    eval 'sub Bit_NumInts {
        local($numBits) = @_;
        eval "((($numBits)+ &BIT_NUM_BITS_PER_INT -1)/ &BIT_NUM_BITS_PER_INT)";
    }';
    eval 'sub Bit_NumBytes {
        local($numBits) = @_;
        eval "( &Bit_NumInts($numBits) * $sizeof{\'int\'})";
    }';
    eval 'sub Bit_Alloc {
        local($numBits, $bitArrayPtr) = @_;
        eval "$bitArrayPtr = (\'int\' *)  &malloc(( &unsigned)  &Bit_NumBytes($numBits));  &Bit_Zero(($numBits), ($bitArrayPtr))";
    }';
    eval 'sub Bit_Free {
        local($bitArrayPtr) = @_;
        eval " &free((\'char\' *)$bitArrayPtr)";
    }';
    eval 'sub Bit_Set {
        local($numBits, $bitArrayPtr) = @_;
        eval "(($bitArrayPtr)[($numBits)/ &BIT_NUM_BITS_PER_INT] |= (1 << (($numBits) %  &BIT_NUM_BITS_PER_INT)))";
    }';
    eval 'sub Bit_IsSet {
        local($numBits, $bitArrayPtr) = @_;
        eval "(($bitArrayPtr)[($numBits)/ &BIT_NUM_BITS_PER_INT] & (1 << (($numBits) %  &BIT_NUM_BITS_PER_INT)))";
    }';
    eval 'sub Bit_Clear {
        local($numBits, $bitArrayPtr) = @_;
        eval "(($bitArrayPtr)[($numBits)/ &BIT_NUM_BITS_PER_INT] &= ~(1 << (($numBits) %  &BIT_NUM_BITS_PER_INT)))";
    }';
    eval 'sub Bit_IsClear {
        local($numBits, $bitArrayPtr) = @_;
        eval "(!( &Bit_IsSet(($numBits), ($bitArrayPtr))))";
    }';
    eval 'sub Bit_Copy {
        local($numBits, $srcArrayPtr, $destArrayPtr) = @_;
        eval " &bcopy((\'char\' *) ($srcArrayPtr), (\'char\' *) ($destArrayPtr),  &Bit_NumBytes($numBits))";
    }';
    eval 'sub Bit_Zero {
        local($numBits, $bitArrayPtr) = @_;
        eval " &bzero((\'char\' *) ($bitArrayPtr),  &Bit_NumBytes($numBits))";
    }';
}
1;
