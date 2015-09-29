if (!defined &__SVC_HEADER__) {
    eval 'sub __SVC_HEADER__ {1;}';
    eval 'sub svc_getcaller {
        local($x) = @_;
        eval "(&($x)-> &xp_raddr)";
    }';
    eval 'sub SVC_RECV {
        local($xprt, $msg) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_recv)(($xprt), ($msg))";
    }';
    eval 'sub svc_recv {
        local($xprt, $msg) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_recv)(($xprt), ($msg))";
    }';
    eval 'sub SVC_STAT {
        local($xprt) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_stat)($xprt)";
    }';
    eval 'sub svc_stat {
        local($xprt) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_stat)($xprt)";
    }';
    eval 'sub SVC_GETARGS {
        local($xprt, $xargs, $argsp) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_getargs)(($xprt), ($xargs), ($argsp))";
    }';
    eval 'sub svc_getargs {
        local($xprt, $xargs, $argsp) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_getargs)(($xprt), ($xargs), ($argsp))";
    }';
    eval 'sub SVC_REPLY {
        local($xprt, $msg) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_reply) (($xprt), ($msg))";
    }';
    eval 'sub svc_reply {
        local($xprt, $msg) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_reply) (($xprt), ($msg))";
    }';
    eval 'sub SVC_FREEARGS {
        local($xprt, $xargs, $argsp) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_freeargs)(($xprt), ($xargs), ($argsp))";
    }';
    eval 'sub svc_freeargs {
        local($xprt, $xargs, $argsp) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_freeargs)(($xprt), ($xargs), ($argsp))";
    }';
    eval 'sub SVC_DESTROY {
        local($xprt) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_destroy)($xprt)";
    }';
    eval 'sub svc_destroy {
        local($xprt) = @_;
        eval "(*($xprt)-> &xp_ops-> &xp_destroy)($xprt)";
    }';
    if (defined &FD_SETSIZE) {
	eval 'sub svc_fds { &svc_fdset. &fds_bits[0];}';
    }
    else {
    }
    eval 'sub RPC_ANYSOCK {-1;}';
}
1;
