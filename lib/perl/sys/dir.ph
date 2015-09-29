if (!defined &_DIR) {
    eval 'sub _DIR {1;}';
    require 'sys/types.ph';
    if (!defined( &KERNEL) && !defined( &DEV_BSIZE)) {
	eval 'sub DEV_BSIZE {512;}';
    }
    eval 'sub DIRBLKSIZ { &DEV_BSIZE;}';
    eval 'sub MAXNAMLEN {255;}';
    eval 'sub DIRSIZ {
        local($dp) = @_;
        eval "(($sizeof{\'struct direct\'} - ( &MAXNAMLEN+1)) + ((($dp)-> &d_namlen+1 + 3) &~ 3))";
    }';
    if (!defined &KERNEL) {
	eval 'sub dirfd {
	    local($dirp) = @_;
	    eval "(($dirp)-> &dd_fd)";
	}';
	if (!defined &NULL) {
	    eval 'sub NULL {0;}';
	}
	eval 'sub rewinddir {
	    local($dirp) = @_;
	    eval " &seekdir(($dirp), (\'long\')0)";
	}';
    }
}
1;
