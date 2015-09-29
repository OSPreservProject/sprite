if (!defined &_NDBM) {
    eval 'sub _NDBM {1;}';
    eval 'sub PBLKSIZ {1024;}';
    eval 'sub DBLKSIZ {4096;}';
    eval 'sub _DBM_RDONLY {0x1;}';
    eval 'sub _DBM_IOERR {0x2;}';
    eval 'sub dbm_rdonly {
        local($db) = @_;
        eval "(($db)-> &dbm_flags &  &_DBM_RDONLY)";
    }';
    eval 'sub dbm_error {
        local($db) = @_;
        eval "(($db)-> &dbm_flags &  &_DBM_IOERR)";
    }';
    eval 'sub dbm_clearerr {
        local($db) = @_;
        eval "(($db)-> &dbm_flags &= ~ &_DBM_IOERR)";
    }';
    eval 'sub dbm_dirfno {
        local($db) = @_;
        eval "(($db)-> &dbm_dirf)";
    }';
    eval 'sub dbm_pagfno {
        local($db) = @_;
        eval "(($db)-> &dbm_pagf)";
    }';
    eval 'sub DBM_INSERT {0;}';
    eval 'sub DBM_REPLACE {1;}';
}
1;
