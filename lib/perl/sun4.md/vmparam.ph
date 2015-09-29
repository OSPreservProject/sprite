if (!defined &_sun4_vmparam_h) {
    eval 'sub _sun4_vmparam_h {1;}';
    if (!defined &sprite) {
	require 'sun4/param.ph';
	eval 'sub USRTEXT {0x2000;}';
	eval 'sub USRSTACK { &KERNELBASE;}';
	eval 'sub LOWPAGES { &btoc( &USRTEXT);}';
	eval 'sub HIGHPAGES {0;}';
	eval 'sub DFLSSIZ {(8*1024*1024);}';
	eval 'sub DFLDSIZ_260 {((512*1024*1024)- &USRTEXT);}';
	eval 'sub MAXDSIZ_260 {((512*1024*1024)- &USRTEXT);}';
	eval 'sub MAXSSIZ_260 {((512*1024*1024)- &KERNELSIZE);}';
	eval 'sub DFLDSIZ_460 {((2048*1024*1024)- &USRTEXT);}';
	eval 'sub MAXDSIZ_460 {((2048*1024*1024)- &USRTEXT);}';
	eval 'sub MAXSSIZ_460 {((2048*1024*1024)- &KERNELSIZE);}';
	eval 'sub DFLDSIZ { &dfldsiz;}';
	eval 'sub MAXDSIZ { &maxdsiz;}';
	eval 'sub MAXSSIZ { &maxssiz;}';
	if (!defined &LOCORE) {
	}
	eval 'sub SSIZE {1;}';
	eval 'sub SINCR {1;}';
	eval 'sub SYSPTSIZE {(0x640000 /  &MMU_PAGESIZE);}';
	eval 'sub MINMAPSIZE {0x200000;}';
	eval 'sub MAXSLP {20;}';
	eval 'sub SAFERSS {3;}';
	eval 'sub DISKRPM {60;}';
	eval 'sub LOTSFREE {(256 * 1024);}';
	eval 'sub LOTSFREEFRACT {8;}';
	eval 'sub DESFREE {(100 * 1024);}';
	eval 'sub DESFREEFRACT {16;}';
	eval 'sub MINFREE {(32 * 1024);}';
	eval 'sub MINFREEFRACT {2;}';
	eval 'sub HANDSPREAD {(2 * 1024 * 1024);}';
	eval 'sub PGTHRESH {(280 * 1024);}';
    }
}
1;
