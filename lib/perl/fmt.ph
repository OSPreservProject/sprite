if (!defined &_FMT) {
    eval 'sub _FMT {1;}';
    require 'cfuncproto.ph';
    eval 'sub FMT_OK {0;}';
    eval 'sub FMT_CONTENT_ERROR {1;}';
    eval 'sub FMT_INPUT_TOO_SMALL {2;}';
    eval 'sub FMT_OUTPUT_TOO_SMALL {3;}';
    eval 'sub FMT_ILLEGAL_FORMAT {4;}';
    eval 'sub FMT_68K_FORMAT {(( &Fmt_Format) 0x1000 | 1);}';
    eval 'sub FMT_VAX_FORMAT {(( &Fmt_Format) 0x1000 | 2);}';
    eval 'sub FMT_SPUR_FORMAT {(( &Fmt_Format) 0x1000 | 3);}';
    eval 'sub FMT_MIPS_FORMAT {(( &Fmt_Format) 0x1000 | 4);}';
    eval 'sub FMT_SPARC_FORMAT {(( &Fmt_Format) 0x1000 | 5);}';
    eval 'sub FMT_SYM_FORMAT {(( &Fmt_Format) 0x1000 | 6);}';
    if (defined( &sun3) || defined( &sun2)) {
	eval 'sub FMT_MY_FORMAT { &FMT_68K_FORMAT;}';
    }
    if (defined( &sun4)) {
	eval 'sub FMT_MY_FORMAT { &FMT_SPARC_FORMAT;}';
    }
    if (defined( &ds3100) || defined( &mips)) {
	eval 'sub FMT_MY_FORMAT { &FMT_MIPS_FORMAT;}';
    }
    if (defined( &spur)) {
	eval 'sub FMT_MY_FORMAT { &FMT_SPUR_FORMAT;}';
    }
    if (defined( &vax)) {
	eval 'sub FMT_MY_FORMAT { &FMT_VAX_FORMAT;}';
    }
    if (defined( &sequent)) {
	eval 'sub FMT_MY_FORMAT { &FMT_SYM_FORMAT;}';
    }
}
1;
