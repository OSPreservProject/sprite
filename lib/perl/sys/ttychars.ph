if (!defined &_TTYCHARS) {
    eval 'sub _TTYCHARS {1;}';
    eval 'sub CTRL {
        local($c) = @_;
        eval "($c&037)";
    }';
    eval 'sub CERASE {0177;}';
    eval 'sub CKILL { &CTRL(ord(\'u\'));}';
    eval 'sub CINTR { &CTRL(ord(\'c\'));}';
    eval 'sub CQUIT {034;}';
    eval 'sub CSTART { &CTRL(ord(\'q\'));}';
    eval 'sub CSTOP { &CTRL(ord(\'s\'));}';
    eval 'sub CEOF { &CTRL(ord(\'d\'));}';
    eval 'sub CEOT { &CEOF;}';
    eval 'sub CBRK {0377;}';
    eval 'sub CSUSP { &CTRL(ord(\'z\'));}';
    eval 'sub CDSUSP { &CTRL(ord(\'y\'));}';
    eval 'sub CRPRNT { &CTRL(ord(\'r\'));}';
    eval 'sub CFLUSH { &CTRL(ord(\'o\'));}';
    eval 'sub CWERASE { &CTRL(ord(\'w\'));}';
    eval 'sub CLNEXT { &CTRL(ord(\'v\'));}';
}
1;
