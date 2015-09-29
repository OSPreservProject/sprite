if (!defined &_ROUTE) {
    eval 'sub _ROUTE {1;}';
    eval 'sub RTF_UP {0x1;}';
    eval 'sub RTF_GATEWAY {0x2;}';
    eval 'sub RTF_HOST {0x4;}';
    eval 'sub RTF_DYNAMIC {0x10;}';
    eval 'sub RTF_MODIFIED {0x20;}';
    if (defined &KERNEL) {
	eval 'sub RTFREE {
	    local($rt) = @_;
	    eval " &if (($rt)-> &rt_refcnt == 1)  &rtfree($rt);  &else ($rt)-> &rt_refcnt--;";
	}';
	if (defined &GATEWAY) {
	    eval 'sub RTHASHSIZ {64;}';
	}
	else {
	    eval 'sub RTHASHSIZ {8;}';
	}
	if (( &RTHASHSIZ & ( &RTHASHSIZ - 1)) == 0) {
	    eval 'sub RTHASHMOD {
	        local($h) = @_;
	        eval "(($h) & ( &RTHASHSIZ - 1))";
	    }';
	}
	else {
	    eval 'sub RTHASHMOD {
	        local($h) = @_;
	        eval "(($h) %  &RTHASHSIZ)";
	    }';
	}
    }
}
1;
