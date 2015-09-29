if (!defined &_DISK) {
    eval 'sub _DISK {1;}';
    require 'kernel/fs.ph';
    require 'kernel/dev.ph';
    require 'kernel/fsdm.ph';
    require 'kernel/ofs.ph';
    require 'kernel/devDiskLabel.ph';
    eval 'sub BITS_PER_BYTE {8;}';
    eval 'sub BITS_PER_INT {32;}';
    eval 'sub DISK_SECTORS_PER_BLOCK {( &FS_BLOCK_SIZE /  &DEV_BYTES_PER_SECTOR);}';
    eval 'sub DISK_KBYTES_PER_BLOCK {( &FS_BLOCK_SIZE / 1024);}';
    eval 'sub DISK_MAX_PARTS {8;}';
    eval 'sub DISK_MAX_ASCII_LABEL {256;}';
    eval 'sub DISK_NO_LABEL {(( &Disk_NativeLabelType) 0);}';
    eval 'sub DISK_SUN_LABEL {(( &Disk_NativeLabelType) 1);}';
    eval 'sub DISK_DEC_LABEL {(( &Disk_NativeLabelType) 2);}';
}
1;
