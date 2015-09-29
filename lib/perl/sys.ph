if (!defined &_SYSUSER) {
    eval 'sub _SYSUSER {1;}';
    if (!defined &_SPRITE) {
	require 'sprite.ph';
    }
    eval 'sub SYS_REBOOT {0x01;}';
    eval 'sub SYS_HALT {0x02;}';
    eval 'sub SYS_KILL_PROCESSES {0x04;}';
    eval 'sub SYS_DEBUG {0x08;}';
    eval 'sub SYS_WRITE_BACK {0x10;}';
    eval 'sub SYS_SPUR {1;}';
    eval 'sub SYS_SUN2 {2;}';
    eval 'sub SYS_SUN3 {3;}';
    eval 'sub SYS_SUN4 {4;}';
    eval 'sub SYS_MICROVAX_2 {5;}';
    eval 'sub SYS_DS3100 {6;}';
    eval 'sub SYS_SYM {7;}';
    eval 'sub SYS_DS5000 {8;}';
    eval 'sub SYS_SUN_ARCH_MASK {0xf0;}';
    eval 'sub SYS_SUN_IMPL_MASK {0x0f;}';
    eval 'sub SYS_SUN_2 {0x00;}';
    eval 'sub SYS_SUN_3 {0x10;}';
    eval 'sub SYS_SUN_4 {0x20;}';
    eval 'sub SYS_SUN_4_C {0x50;}';
    eval 'sub SYS_SUN_2_50 {0x02;}';
    eval 'sub SYS_SUN_2_120 {0x01;}';
    eval 'sub SYS_SUN_2_160 {0x02;}';
    eval 'sub SYS_SUN_3_75 {0x11;}';
    eval 'sub SYS_SUN_3_160 {0x11;}';
    eval 'sub SYS_SUN_3_50 {0x12;}';
    eval 'sub SYS_SUN_3_60 {0x17;}';
    eval 'sub SYS_SUN_4_200 {0x21;}';
    eval 'sub SYS_SUN_4_C_60 {0x51;}';
    eval 'sub SYS_SUN_4_C_65 {0x53;}';
}
1;
