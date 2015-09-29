if (!defined &_FCNTL) {
    eval 'sub _FCNTL {1;}';
    require 'cfuncproto.ph';
    eval 'sub O_RDONLY {000;}';
    eval 'sub O_WRONLY {001;}';
    eval 'sub O_RDWR {002;}';
    eval 'sub O_NDELAY { &FNDELAY;}';
    eval 'sub O_APPEND { &FAPPEND;}';
    eval 'sub O_CREAT { &FCREAT;}';
    eval 'sub O_TRUNC { &FTRUNC;}';
    eval 'sub O_EXCL { &FEXCL;}';
    if (!defined &F_DUPFD) {
	eval 'sub F_DUPFD {0;}';
	eval 'sub F_GETFD {1;}';
	eval 'sub F_SETFD {2;}';
	eval 'sub F_GETFL {3;}';
	eval 'sub F_SETFL {4;}';
	eval 'sub F_GETOWN {5;}';
	eval 'sub F_SETOWN {6;}';
	eval 'sub F_GETLK {7;}';
	eval 'sub F_SETLK {8;}';
	eval 'sub F_SETLKW {9;}';
	eval 'sub F_RGETLK {10;}';
	eval 'sub F_RSETLK {11;}';
	eval 'sub F_CNVT {12;}';
	eval 'sub F_RSETLKW {13;}';
	eval 'sub FNDELAY {00004;}';
	eval 'sub FAPPEND {00010;}';
	eval 'sub FASYNC {00100;}';
	eval 'sub FCREAT {01000;}';
	eval 'sub FTRUNC {02000;}';
	eval 'sub FEXCL {04000;}';
    }
}
1;
