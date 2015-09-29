if (!defined &_FBDEV) {
    eval 'sub _FBDEV {1;}';
    if (!defined &ASM) {
	eval 'sub FB_ATTR_NDEVSPECIFIC {8;}';
	eval 'sub FB_ATTR_NEMUTYPES {4;}';
	eval 'sub FBVIDEO_OFF {0;}';
	eval 'sub FBVIDEO_ON {1;}';
	eval 'sub FBTYPE_SUN1BW {0;}';
	eval 'sub FBTYPE_SUN1COLOR {1;}';
	eval 'sub FBTYPE_SUN2BW {2;}';
	eval 'sub FBTYPE_SUN2COLOR {3;}';
	eval 'sub FBTYPE_SUN2GP {4;}';
	eval 'sub FBTYPE_SUN5COLOR {5;}';
	eval 'sub FBTYPE_SUN3COLOR {6;}';
	eval 'sub FBTYPE_MEMCOLOR {7;}';
	eval 'sub FBTYPE_SUN4COLOR {8;}';
	eval 'sub FBTYPE_NOTSUN1 {9;}';
	eval 'sub FBTYPE_NOTSUN2 {10;}';
	eval 'sub FBTYPE_NOTSUN3 {11;}';
	eval 'sub FBTYPE_SUNFAST_COLOR {12;}';
	eval 'sub FBTYPE_SUNROP_COLOR {13;}';
	eval 'sub FBTYPE_SUNFB_VIDEO {14;}';
	eval 'sub FBTYPE_RESERVED5 {15;}';
	eval 'sub FBTYPE_RESERVED4 {16;}';
	eval 'sub FBTYPE_RESERVED3 {17;}';
	eval 'sub FBTYPE_RESERVED2 {18;}';
	eval 'sub FBTYPE_RESERVED1 {19;}';
	eval 'sub FBTYPE_LASTPLUSONE {20;}';
	eval 'sub FB_ATTR_AUTOINIT {1;}';
	eval 'sub FB_ATTR_DEVSPECIFIC {2;}';
    }
}
1;
