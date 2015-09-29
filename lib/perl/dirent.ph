if (!defined &_DIRENT) {
    eval 'sub _DIRENT {1;}';
    require 'sys/types.ph';
    if (!defined &MAXNAMLEN) {
	eval 'sub MAXNAMLEN {255;}';
    }
}
1;
