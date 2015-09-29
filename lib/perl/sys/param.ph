if (!defined &_PARAM) {
    eval 'sub _PARAM {1;}';
    require 'sys/types.ph';
    require 'signal.ph';
    require 'machparam.ph';
    eval 'sub MAXPATHLEN {1024;}';
    eval 'sub MAXBSIZE {4096;}';
    eval 'sub dbtob {
        local($blocks) = @_;
        eval "(( &unsigned) ($blocks) << 9)";
    }';
    eval 'sub MAXHOSTNAMELEN {64;}';
    eval 'sub NCARGS {10240;}';
    eval 'sub NOFILE {64;}';
    eval 'sub NGROUPS {16;}';
    eval 'sub MIN {
        local($a,$b) = @_;
        eval "((($a)<($b))?($a):($b))";
    }';
    eval 'sub MAX {
        local($a,$b) = @_;
        eval "((($a)>($b))?($a):($b))";
    }';
    eval 'sub roundup {
        local($x, $y) = @_;
        eval "(((($x) + (($y) -1))/($y))*($y))";
    }';
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
}
1;
