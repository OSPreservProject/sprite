if (!defined &_UNISTD) {
    eval 'sub _UNISTD {1;}';
    require 'cfuncproto.ph';
    require 'sys/types.ph';
    if (defined &__STDC__) {
	eval 'sub VOLATILE { &volatile;}';
    }
    else {
	eval 'sub VOLATILE {1;}';
    }
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    if (!defined &_SIZE_T) {
	eval 'sub _SIZE_T {1;}';
    }
    if (!defined &_POSIX_SOURCE) {
    }
}
1;
