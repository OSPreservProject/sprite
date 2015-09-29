if (!defined &_SETJMP) {
    eval 'sub _SETJMP {1;}';
    require 'cfuncproto.ph';
}
1;
