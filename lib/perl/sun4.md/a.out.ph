if (!defined &_AOUT) {
    eval 'sub _AOUT {1;}';
    require 'sun4.md/sys/exec.ph';
    eval 'sub N_BADMAG {
        local($x) = @_;
        eval "((($x). &a_magic)!= &OMAGIC && (($x). &a_magic)!= &NMAGIC && (($x). &a_magic)!= &ZMAGIC && (($x). &a_magic)!= &SPRITE_ZMAGIC && (($x). &a_magic)!= &UNIX_ZMAGIC)";
    }';
    eval 'sub N_PAGSIZ {
        local($x) = @_;
        eval "( &Aout_PageSize[($x). &a_machtype])";
    }';
    eval 'sub N_TXTOFF {
        local($x) = @_;
        eval "(($x). &a_magic== &ZMAGIC ? 0 : $sizeof{\'struct exec\'})";
    }';
    eval 'sub N_SYMOFF {
        local($x) = @_;
        eval "( &N_TXTOFF($x) + ($x). &a_text+($x). &a_data + ($x). &a_trsize + ($x). &a_drsize)";
    }';
    eval 'sub N_STROFF {
        local($x) = @_;
        eval "( &N_SYMOFF($x) + ($x). &a_syms)";
    }';
    require 'sun4.md/kernel/procMach.ph';
    eval 'sub N_TXTADDR {
        local($x) = @_;
        eval " &PROC_CODE_LOAD_ADDR(*(( &ProcExecHeader *) &($x)))";
    }';
    eval 'sub N_DATADDR {
        local($x) = @_;
        eval " &PROC_DATA_LOAD_ADDR(*(( &ProcExecHeader *) &($x)))";
    }';
    eval 'sub N_BSSADDR {
        local($x) = @_;
        eval " &PROC_BSS_LOAD_ADDR(*(( &ProcExecHeader *) &($x)))";
    }';
    if (0) {
    }
    eval 'sub n_hash { &n_desc;}';
    eval 'sub N_UNDF {0x0;}';
    eval 'sub N_ABS {0x2;}';
    eval 'sub N_TEXT {0x4;}';
    eval 'sub N_DATA {0x6;}';
    eval 'sub N_BSS {0x8;}';
    eval 'sub N_COMM {0x12;}';
    eval 'sub N_FN {0x1e;}';
    eval 'sub N_EXT {01;}';
    eval 'sub N_TYPE {0x1e;}';
    eval 'sub N_STAB {0xe0;}';
    eval 'sub N_FORMAT {"%08x";}';
}
1;
