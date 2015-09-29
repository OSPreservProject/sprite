if (!defined &_TIME) {
    eval 'sub _TIME {1;}';
    require 'cfuncproto.ph';
    if (!defined &_TIME_T) {
	eval 'sub _TIME_T {1;}';
    }
}
1;
