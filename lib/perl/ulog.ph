if (!defined &_ULOG) {
    eval 'sub _ULOG {1;}';
    eval 'sub ULOG_LOC_CONSOLE {0;}';
    eval 'sub ULOG_MAX_PORTS {10;}';
    eval 'sub ULOG_LOC_LENGTH {33;}';
    eval 'sub LASTLOG_FILE_NAME {"/sprite/admin/lastLog";}';
    eval 'sub ULOG_FILE_NAME {"/sprite/admin/userLog";}';
}
1;
