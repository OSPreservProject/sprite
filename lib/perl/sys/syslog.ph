if (!defined &_SYSLOG) {
    eval 'sub _SYSLOG {1;}';
    eval 'sub LOG_KERN {(0<<3);}';
    eval 'sub LOG_USER {(1<<3);}';
    eval 'sub LOG_MAIL {(2<<3);}';
    eval 'sub LOG_DAEMON {(3<<3);}';
    eval 'sub LOG_AUTH {(4<<3);}';
    eval 'sub LOG_SYSLOG {(5<<3);}';
    eval 'sub LOG_LPR {(6<<3);}';
    eval 'sub LOG_NEWS {(7<<3);}';
    eval 'sub LOG_UUCP {(8<<3);}';
    eval 'sub LOG_LOCAL0 {(16<<3);}';
    eval 'sub LOG_LOCAL1 {(17<<3);}';
    eval 'sub LOG_LOCAL2 {(18<<3);}';
    eval 'sub LOG_LOCAL3 {(19<<3);}';
    eval 'sub LOG_LOCAL4 {(20<<3);}';
    eval 'sub LOG_LOCAL5 {(21<<3);}';
    eval 'sub LOG_LOCAL6 {(22<<3);}';
    eval 'sub LOG_LOCAL7 {(23<<3);}';
    eval 'sub LOG_NFACILITIES {24;}';
    eval 'sub LOG_FACMASK {0x03f8;}';
    eval 'sub LOG_FAC {
        local($p) = @_;
        eval "((($p) &  &LOG_FACMASK) >> 3)";
    }';
    eval 'sub LOG_EMERG {0;}';
    eval 'sub LOG_ALERT {1;}';
    eval 'sub LOG_CRIT {2;}';
    eval 'sub LOG_ERR {3;}';
    eval 'sub LOG_WARNING {4;}';
    eval 'sub LOG_NOTICE {5;}';
    eval 'sub LOG_INFO {6;}';
    eval 'sub LOG_DEBUG {7;}';
    eval 'sub LOG_PRIMASK {0x0007;}';
    eval 'sub LOG_PRI {
        local($p) = @_;
        eval "(($p) &  &LOG_PRIMASK)";
    }';
    eval 'sub LOG_MAKEPRI {
        local($fac, $pri) = @_;
        eval "((($fac) << 3) | ($pri))";
    }';
    if (defined &KERNEL) {
	eval 'sub LOG_PRINTF {-1;}';
    }
    eval 'sub LOG_MASK {
        local($pri) = @_;
        eval "(1 << ($pri))";
    }';
    eval 'sub LOG_UPTO {
        local($pri) = @_;
        eval "((1 << (($pri)+1)) - 1)";
    }';
    eval 'sub LOG_PID {0x01;}';
    eval 'sub LOG_CONS {0x02;}';
    eval 'sub LOG_ODELAY {0x04;}';
    eval 'sub LOG_NDELAY {0x08;}';
    eval 'sub LOG_NOWAIT {0x10;}';
}
1;
