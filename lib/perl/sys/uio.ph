if (!defined &_UIO) {
    eval 'sub _UIO {1;}';
    eval 'sub UIO_USERSPACE {0;}';
    eval 'sub UIO_SYSSPACE {1;}';
    eval 'sub UIO_USERISPACE {2;}';
}
1;
