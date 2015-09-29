if (!defined &_SPRITE) {
    eval 'sub _SPRITE {1;}';
    if (!defined &TRUE) {
	eval 'sub TRUE {1;}';
    }
    if (!defined &FALSE) {
	eval 'sub FALSE {0;}';
    }
    if (!defined &_ASM) {
    }
    eval 'sub SUCCESS {0x00000000;}';
    eval 'sub FAILURE {0x00000001;}';
    eval 'sub NIL {0xFFFFFFFF;}';
    eval 'sub USER_NIL {0;}';
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    if (!defined &_ASM) {
	if (!defined &_CLIENTDATA) {
	    eval 'sub _CLIENTDATA {1;}';
	}
	if (!defined &__STDC__) {
	    eval 'sub volatile {1;}';
	    eval 'sub const {1;}';
	}
    }
}
1;
