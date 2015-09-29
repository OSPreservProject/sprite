if (!defined &_UTMP) {
    eval 'sub _UTMP {1;}';
    eval 'sub _PATH_UTMP {"/etc/utmp";}';
    eval 'sub UT_NAMESIZE {8;}';
    eval 'sub UT_LINESIZE {8;}';
    eval 'sub UT_HOSTSIZE {16;}';
}
1;
