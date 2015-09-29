sub LOG_KERN {(0<<3);}
sub LOG_USER {(1<<3);}
sub LOG_MAIL {(2<<3);}
sub LOG_DAEMON {(3<<3);}
sub LOG_AUTH {(4<<3);}
sub LOG_SYSLOG {(5<<3);}
sub LOG_LPR {(6<<3);}
sub LOG_NEWS {(7<<3);}
sub LOG_UUCP {(8<<3);}
sub LOG_LOCAL0 {(16<<3);}
sub LOG_LOCAL1 {(17<<3);}
sub LOG_LOCAL2 {(18<<3);}
sub LOG_LOCAL3 {(19<<3);}
sub LOG_LOCAL4 {(20<<3);}
sub LOG_LOCAL5 {(21<<3);}
sub LOG_LOCAL6 {(22<<3);}
sub LOG_LOCAL7 {(23<<3);}
sub LOG_NFACILITIES {24;}
sub LOG_FACMASK {0x03f8;}
sub LOG_FAC {
    local($p) = @_;
    eval "((($p) &  &LOG_FACMASK) >> 3)";
}
sub LOG_EMERG {0;}
sub LOG_ALERT {1;}
sub LOG_CRIT {2;}
sub LOG_ERR {3;}
sub LOG_WARNING {4;}
sub LOG_NOTICE {5;}
sub LOG_INFO {6;}
sub LOG_DEBUG {7;}
sub LOG_PRIMASK {0x0007;}
sub LOG_PRI {
    local($p) = @_;
    eval "(($p) &  &LOG_PRIMASK)";
}
sub LOG_MAKEPRI {
    local($fac, $pri) = @_;
    eval "((($fac) << 3) | ($pri))";
}
if (defined &KERNEL) {
    eval 'sub LOG_PRINTF {-1;}';
}
sub LOG_MASK {
    local($pri) = @_;
    eval "(1 << ($pri))";
}
sub LOG_UPTO {
    local($pri) = @_;
    eval "((1 << (($pri)+1)) - 1)";
}
sub LOG_PID {0x01;}
sub LOG_CONS {0x02;}
sub LOG_ODELAY {0x04;}
sub LOG_NDELAY {0x08;}
sub LOG_NOWAIT {0x10;}
1;
