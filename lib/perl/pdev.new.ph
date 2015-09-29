if (!defined &_PDEVLIB) {
    eval 'sub _PDEVLIB {1;}';
    require 'fs.ph';
    require 'dev/pdev.new.ph';
    eval 'sub PDEV_MAGIC {0xabcd1234;}';
    eval 'sub PDEV_STREAM_MAGIC {0xa1b2c3d4;}';
    if (!defined &max) {
	eval 'sub max {
	    local($a, $b) = @_;
	    eval "( (($a) > ($b)) ? ($a) : ($b) )";
	}';
    }
}
1;
