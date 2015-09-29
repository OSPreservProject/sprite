if (!defined &_TYPES) {
    eval 'sub _TYPES {1;}';
    if (!defined &KERNEL) {
	eval 'sub major {
	    local($x) = @_;
	    eval "((\'int\')((( &unsigned)($x)>>8)&0377))";
	}';
    }
    eval 'sub unix_major {
        local($x) = @_;
        eval "((\'int\')((( &unsigned)($x)>>8)&0377))";
    }';
    if (!defined &KERNEL) {
	eval 'sub minor {
	    local($x) = @_;
	    eval "((\'int\')(($x)&0377))";
	}';
    }
    eval 'sub unix_minor {
        local($x) = @_;
        eval "((\'int\')(($x)&0377))";
    }';
    eval 'sub makedev {
        local($x,$y) = @_;
        eval "(( &dev_t)((($x)<<8) | ($y)))";
    }';
    if (defined( &vax) || defined( &tahoe)) {
    }
    if (defined( &mc68000)) {
    }
    if (!defined &_SIZE_T) {
	eval 'sub _SIZE_T {1;}';
    }
    if (!defined &_TIME_T) {
	eval 'sub _TIME_T {1;}';
    }
    eval 'sub NBBY {8;}';
    if (!defined &FD_SETSIZE) {
	eval 'sub FD_SETSIZE {256;}';
    }
    eval 'sub NFDBITS {($sizeof{ &fd_mask} *  &NBBY);}';
    if (!defined &howmany) {
	eval 'sub howmany {
	    local($x, $y) = @_;
	    eval "((($x)+(($y)-1))/($y))";
	}';
    }
    eval 'sub FD_SET {
        local($n, $p) = @_;
        eval "(($p)-> &fds_bits[($n)/ &NFDBITS] |= (1 << (($n) %  &NFDBITS)))";
    }';
    eval 'sub FD_CLR {
        local($n, $p) = @_;
        eval "(($p)-> &fds_bits[($n)/ &NFDBITS] &= ~(1 << (($n) %  &NFDBITS)))";
    }';
    eval 'sub FD_ISSET {
        local($n, $p) = @_;
        eval "(($p)-> &fds_bits[($n)/ &NFDBITS] & (1 << (($n) %  &NFDBITS)))";
    }';
    eval 'sub FD_ZERO {
        local($p) = @_;
        eval " &bzero((\'char\' *)($p), $sizeof{*($p}))";
    }';
}
1;
