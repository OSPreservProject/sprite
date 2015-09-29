if (!defined &_CLNT_) {
    eval 'sub _CLNT_ {1;}';
    eval 'sub re_errno { &ru. &RE_errno;}';
    eval 'sub re_why { &ru. &RE_why;}';
    eval 'sub re_vers { &ru. &RE_vers;}';
    eval 'sub re_lb { &ru. &RE_lb;}';
    eval 'sub CLNT_CALL {
        local($rh, $proc, $xargs, $argsp, $xres, $resp, $secs) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_call)($rh, $proc, $xargs, $argsp, $xres, $resp, $secs))";
    }';
    eval 'sub clnt_call {
        local($rh, $proc, $xargs, $argsp, $xres, $resp, $secs) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_call)($rh, $proc, $xargs, $argsp, $xres, $resp, $secs))";
    }';
    eval 'sub CLNT_ABORT {
        local($rh) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_abort)($rh))";
    }';
    eval 'sub clnt_abort {
        local($rh) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_abort)($rh))";
    }';
    eval 'sub CLNT_GETERR {
        local($rh,$errp) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_geterr)($rh, $errp))";
    }';
    eval 'sub clnt_geterr {
        local($rh,$errp) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_geterr)($rh, $errp))";
    }';
    eval 'sub CLNT_FREERES {
        local($rh,$xres,$resp) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_freeres)($rh,$xres,$resp))";
    }';
    eval 'sub clnt_freeres {
        local($rh,$xres,$resp) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_freeres)($rh,$xres,$resp))";
    }';
    eval 'sub CLNT_CONTROL {
        local($cl,$rq,$in) = @_;
        eval "((*($cl)-> &cl_ops-> &cl_control)($cl,$rq,$in))";
    }';
    eval 'sub clnt_control {
        local($cl,$rq,$in) = @_;
        eval "((*($cl)-> &cl_ops-> &cl_control)($cl,$rq,$in))";
    }';
    eval 'sub CLSET_TIMEOUT {1;}';
    eval 'sub CLGET_TIMEOUT {2;}';
    eval 'sub CLGET_SERVER_ADDR {3;}';
    eval 'sub CLSET_RETRY_TIMEOUT {4;}';
    eval 'sub CLGET_RETRY_TIMEOUT {5;}';
    eval 'sub CLSET_RETRY_COUNT {6;}';
    eval 'sub CLGET_RETRY_COUNT {7;}';
    eval 'sub CLSET_RETRY_DELAY {8;}';
    eval 'sub CLGET_RETRY_DELAY {9;}';
    eval 'sub RETRYCOUNTDFL {0;}';
    eval 'sub RETRYDELAYDFL {60;}';
    eval 'sub CLNT_DESTROY {
        local($rh) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_destroy)($rh))";
    }';
    eval 'sub clnt_destroy {
        local($rh) = @_;
        eval "((*($rh)-> &cl_ops-> &cl_destroy)($rh))";
    }';
    eval 'sub RPCTEST_PROGRAM {(( &u_long)1);}';
    eval 'sub RPCTEST_VERSION {(( &u_long)1);}';
    eval 'sub RPCTEST_NULL_PROC {(( &u_long)2);}';
    eval 'sub RPCTEST_NULL_BATCH_PROC {(( &u_long)3);}';
    eval 'sub NULLPROC {(( &u_long)0);}';
    eval 'sub UDPMSGSIZE {8800;}';
    eval 'sub RPCSMALLMSGSIZE {400;}';
}
1;
