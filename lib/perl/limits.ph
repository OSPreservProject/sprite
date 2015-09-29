if (!defined &_LIMITS) {
    eval 'sub _LIMITS {1;}';
    eval 'sub CHAR_BIT {8;}';
    eval 'sub SCHAR_MIN {(-128);}';
    eval 'sub SCHAR_MAX {127;}';
    eval 'sub UCHAR_MAX {255;}';
    eval 'sub CHAR_MIN {(-128);}';
    eval 'sub CHAR_MAX {+127;}';
    eval 'sub SHRT_MIN {0x8000;}';
    eval 'sub SHRT_MAX {0x7fff;}';
    eval 'sub USHRT_MAX {0xffff;}';
    eval 'sub INT_MIN {0x80000000;}';
    eval 'sub INT_MAX {0x7fffffff;}';
    eval 'sub UINT_MAX {0xffffffff;}';
    eval 'sub LONG_MIN {0x80000000;}';
    eval 'sub LONG_MAX {0x7fffffff;}';
    eval 'sub ULONG_MAX {0xffffffff;}';
}
1;
