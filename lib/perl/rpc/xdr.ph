if (!defined &__XDR_HEADER__) {
    eval 'sub __XDR_HEADER__ {1;}';
    eval 'sub BYTES_PER_XDR_UNIT {(4);}';
    eval 'sub RNDUP {
        local($x) = @_;
        eval "(((($x) +  &BYTES_PER_XDR_UNIT - 1) /  &BYTES_PER_XDR_UNIT) *  &BYTES_PER_XDR_UNIT)";
    }';
    eval 'sub XDR_GETLONG {
        local($xdrs, $longp) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_getlong)($xdrs, $longp)";
    }';
    eval 'sub xdr_getlong {
        local($xdrs, $longp) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_getlong)($xdrs, $longp)";
    }';
    eval 'sub XDR_PUTLONG {
        local($xdrs, $longp) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_putlong)($xdrs, $longp)";
    }';
    eval 'sub xdr_putlong {
        local($xdrs, $longp) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_putlong)($xdrs, $longp)";
    }';
    eval 'sub XDR_GETBYTES {
        local($xdrs, $addr, $len) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_getbytes)($xdrs, $addr, $len)";
    }';
    eval 'sub xdr_getbytes {
        local($xdrs, $addr, $len) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_getbytes)($xdrs, $addr, $len)";
    }';
    eval 'sub XDR_PUTBYTES {
        local($xdrs, $addr, $len) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_putbytes)($xdrs, $addr, $len)";
    }';
    eval 'sub xdr_putbytes {
        local($xdrs, $addr, $len) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_putbytes)($xdrs, $addr, $len)";
    }';
    eval 'sub XDR_GETPOS {
        local($xdrs) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_getpostn)($xdrs)";
    }';
    eval 'sub xdr_getpos {
        local($xdrs) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_getpostn)($xdrs)";
    }';
    eval 'sub XDR_SETPOS {
        local($xdrs, $pos) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_setpostn)($xdrs, $pos)";
    }';
    eval 'sub xdr_setpos {
        local($xdrs, $pos) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_setpostn)($xdrs, $pos)";
    }';
    eval 'sub XDR_INLINE {
        local($xdrs, $len) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_inline)($xdrs, $len)";
    }';
    eval 'sub xdr_inline {
        local($xdrs, $len) = @_;
        eval "(*($xdrs)-> &x_ops-> &x_inline)($xdrs, $len)";
    }';
    eval 'sub XDR_DESTROY {
        local($xdrs) = @_;
        eval " &if (($xdrs)-> &x_ops-> &x_destroy) (*($xdrs)-> &x_ops-> &x_destroy)($xdrs)";
    }';
    eval 'sub xdr_destroy {
        local($xdrs) = @_;
        eval " &if (($xdrs)-> &x_ops-> &x_destroy) (*($xdrs)-> &x_ops-> &x_destroy)($xdrs)";
    }';
    eval 'sub NULL_xdrproc_t {(( &xdrproc_t)0);}';
    eval 'sub IXDR_GET_LONG {
        local($buf) = @_;
        eval "((\'long\') &ntohl(( &u_long)*($buf)++))";
    }';
    eval 'sub IXDR_PUT_LONG {
        local($buf, $v) = @_;
        eval "(*($buf)++ = (\'long\') &htonl(( &u_long)$v))";
    }';
    eval 'sub IXDR_GET_BOOL {
        local($buf) = @_;
        eval "(( &bool_t) &IXDR_GET_LONG($buf))";
    }';
    eval 'sub IXDR_GET_ENUM {
        local($buf, $t) = @_;
        eval "(($t) &IXDR_GET_LONG($buf))";
    }';
    eval 'sub IXDR_GET_U_LONG {
        local($buf) = @_;
        eval "(( &u_long) &IXDR_GET_LONG($buf))";
    }';
    eval 'sub IXDR_GET_SHORT {
        local($buf) = @_;
        eval "((\'short\') &IXDR_GET_LONG($buf))";
    }';
    eval 'sub IXDR_GET_U_SHORT {
        local($buf) = @_;
        eval "(( &u_short) &IXDR_GET_LONG($buf))";
    }';
    eval 'sub IXDR_PUT_BOOL {
        local($buf, $v) = @_;
        eval " &IXDR_PUT_LONG(($buf), ((\'long\')($v)))";
    }';
    eval 'sub IXDR_PUT_ENUM {
        local($buf, $v) = @_;
        eval " &IXDR_PUT_LONG(($buf), ((\'long\')($v)))";
    }';
    eval 'sub IXDR_PUT_U_LONG {
        local($buf, $v) = @_;
        eval " &IXDR_PUT_LONG(($buf), ((\'long\')($v)))";
    }';
    eval 'sub IXDR_PUT_SHORT {
        local($buf, $v) = @_;
        eval " &IXDR_PUT_LONG(($buf), ((\'long\')($v)))";
    }';
    eval 'sub IXDR_PUT_U_SHORT {
        local($buf, $v) = @_;
        eval " &IXDR_PUT_LONG(($buf), ((\'long\')($v)))";
    }';
    eval 'sub MAX_NETOBJ_SZ {1024;}';
}
1;
