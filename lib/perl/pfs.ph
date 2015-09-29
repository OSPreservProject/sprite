if (!defined &_PFSLIB) {
    eval 'sub _PFSLIB {1;}';
    require 'fs.ph';
    require 'pdev.ph';
    require 'dev/pfs.ph';
    eval 'sub PFS_MAGIC {0x4a3b2c1d;}';
}
1;
