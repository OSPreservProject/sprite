if (!defined &_MMAN) {
    eval 'sub _MMAN {1;}';
    eval 'sub PROT_READ {0x4;}';
    eval 'sub PROT_WRITE {0x2;}';
    eval 'sub PROT_EXEC {0x1;}';
    eval 'sub SUN_PROT_READ {0x1;}';
    eval 'sub SUN_PROT_WRITE {0x2;}';
    eval 'sub SUN_PROT_EXEC {0x4;}';
    eval 'sub PROT_RDWR {( &PROT_READ| &PROT_WRITE);}';
    eval 'sub PROT_BITS {( &PROT_READ| &PROT_WRITE| &PROT_EXEC);}';
    eval 'sub MAP_SHARED {1;}';
    eval 'sub MAP_PRIVATE {2;}';
    eval 'sub MAP_ZEROFILL {3;}';
    eval 'sub MAP_TYPE {0xf;}';
    eval 'sub MAP_FIXED {0x10;}';
    eval 'sub _MAP_NEW {0x80000000;}';
}
1;
