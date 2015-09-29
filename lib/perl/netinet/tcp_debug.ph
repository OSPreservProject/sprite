if (!defined &_TCP_DEBUG) {
    eval 'sub _TCP_DEBUG {1;}';
    eval 'sub TA_INPUT {0;}';
    eval 'sub TA_OUTPUT {1;}';
    eval 'sub TA_USER {2;}';
    eval 'sub TA_RESPOND {3;}';
    eval 'sub TA_DROP {4;}';
    if (defined &TANAMES) {
    }
    eval 'sub TCP_NDEBUG {100;}';
}
1;
