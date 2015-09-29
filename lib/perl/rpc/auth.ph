sub MAX_AUTH_BYTES {400;}
sub MAXNETNAMELEN {255;}
if (( &mc68000 ||  &sparc ||  &vax ||  &i386)) {
}
sub AUTH_NEXTVERF {
    local($auth) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_nextverf))($auth))";
}
sub auth_nextverf {
    local($auth) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_nextverf))($auth))";
}
sub AUTH_MARSHALL {
    local($auth, $xdrs) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_marshal))($auth, $xdrs))";
}
sub auth_marshall {
    local($auth, $xdrs) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_marshal))($auth, $xdrs))";
}
sub AUTH_VALIDATE {
    local($auth, $verfp) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_validate))(($auth), $verfp))";
}
sub auth_validate {
    local($auth, $verfp) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_validate))(($auth), $verfp))";
}
sub AUTH_REFRESH {
    local($auth) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_refresh))($auth))";
}
sub auth_refresh {
    local($auth) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_refresh))($auth))";
}
sub AUTH_DESTROY {
    local($auth) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_destroy))($auth))";
}
sub auth_destroy {
    local($auth) = @_;
    eval "((*(($auth)-> &ah_ops-> &ah_destroy))($auth))";
}
sub AUTH_NONE {0;}
sub AUTH_NULL {0;}
sub AUTH_UNIX {1;}
sub AUTH_SHORT {2;}
sub AUTH_DES {3;}
1;
