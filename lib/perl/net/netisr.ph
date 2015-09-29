if (!defined &_NETISR) {
    eval 'sub _NETISR {1;}';
    if (defined( &vax) || defined( &tahoe)) {
	eval 'sub setsoftnet {
	    eval " &mtpr( &SIRR, 12)";
	}';
    }
    eval 'sub NETISR_RAW {0;}';
    eval 'sub NETISR_IP {2;}';
    eval 'sub NETISR_IMP {3;}';
    eval 'sub NETISR_NS {6;}';
    eval 'sub schednetisr {
        local($anisr) = @_;
        eval "{  &netisr |= 1<<($anisr);  &setsoftnet(); }";
    }';
    if (!defined &LOCORE) {
	if (defined &KERNEL) {
	}
    }
}
1;
