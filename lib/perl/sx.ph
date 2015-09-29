if (!defined &_SX) {
    eval 'sub _SX {1;}';
    if (!defined &_XLIB_H_) {
	require 'X11/Xlib.ph';
    }
    if (!defined &_CLIENTDATA) {
	eval 'sub _CLIENTDATA {1;}';
    }
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    eval 'sub SX_SCROLL_ABSOLUTE {0;}';
    eval 'sub SX_SCROLL_PAGES {1;}';
    eval 'sub SX_MAX_MENU_ENTRIES {16;}';
    eval 'sub SX_MAX_MENUS {16;}';
    eval 'sub SX_FORMAT_SIZE {20;}';
}
1;
