if (!defined &_TERMIO_) {
    eval 'sub _TERMIO_ {1;}';
    eval 'sub TCGETA { &_IOR(ord(\'T\'), 1, \'struct termio\');}';
    eval 'sub TCSETA { &_IOW(ord(\'T\'), 2, \'struct termio\');}';
    eval 'sub TCSETAW { &_IOW(ord(\'T\'), 3, \'struct termio\');}';
    eval 'sub TCSETAF { &_IOW(ord(\'T\'), 4, \'struct termio\');}';
    eval 'sub TCSBRK { &_IO(ord(\'T\'), 5);}';
    eval 'sub NCC {8;}';
}
1;
