if (!defined &_PWD) {
    eval 'sub _PWD {1;}';
    eval 'sub _PATH_PASSWD {"/etc/passwd";}';
    eval 'sub _PATH_MASTERPASSWD {"/etc/master.passwd";}';
    eval 'sub _PATH_MKPASSWD {"/sprite/cmds.$MACHINE/mkpasswd";}';
    eval 'sub _PATH_PTMP {"/etc/ptmp";}';
    eval 'sub _PW_KEYBYNAME {ord(\'0\');}';
    eval 'sub _PW_KEYBYUID {ord(\'1\');}';
}
1;
