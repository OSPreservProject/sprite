if (!defined &_STAT) {
    eval 'sub _STAT {1;}';
    require 'cfuncproto.ph';
    eval 'sub S_IFMT {0170000;}';
    eval 'sub S_IFDIR {0040000;}';
    eval 'sub S_IFCHR {0020000;}';
    eval 'sub S_IFBLK {0060000;}';
    eval 'sub S_IFREG {0100000;}';
    eval 'sub S_IFLNK {0120000;}';
    eval 'sub S_IFSOCK {0140000;}';
    eval 'sub S_IFIFO {0010000;}';
    eval 'sub S_IFPDEV {0150000;}';
    eval 'sub S_IFRLNK {0160000;}';
    eval 'sub S_ISUID {0004000;}';
    eval 'sub S_ISGID {0002000;}';
    eval 'sub S_ISVTX {0001000;}';
    eval 'sub S_IREAD {0000400;}';
    eval 'sub S_IWRITE {0000200;}';
    eval 'sub S_IEXEC {0000100;}';
    eval 'sub S_TYPE_UNDEFINED {0;}';
    eval 'sub S_TYPE_TMP {1;}';
    eval 'sub S_TYPE_SWAP {2;}';
    eval 'sub S_TYPE_OBJECT {3;}';
    eval 'sub S_TYPE_BINARY {4;}';
    eval 'sub S_TYPE_OTHER {5;}';
}
1;
