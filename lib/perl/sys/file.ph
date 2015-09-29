if (!defined &_FILE) {
    eval 'sub _FILE {1;}';
    eval 'sub FOPEN {(-1);}';
    eval 'sub FREAD {00001;}';
    eval 'sub FWRITE {00002;}';
    if (!defined &F_DUPFD) {
	eval 'sub FNDELAY {00004;}';
	eval 'sub FAPPEND {00010;}';
    }
    eval 'sub FMARK {00020;}';
    eval 'sub FDEFER {00040;}';
    if (!defined &F_DUPFD) {
	eval 'sub FASYNC {00100;}';
    }
    eval 'sub FSHLOCK {00200;}';
    eval 'sub FEXLOCK {00400;}';
    eval 'sub FMASK {00113;}';
    eval 'sub FCNTLCANT {( &FREAD| &FWRITE| &FMARK| &FDEFER| &FSHLOCK| &FEXLOCK);}';
    eval 'sub FCREAT {01000;}';
    eval 'sub FTRUNC {02000;}';
    eval 'sub FEXCL {04000;}';
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
    }
    eval 'sub O_RDONLY {000;}';
    eval 'sub O_WRONLY {001;}';
    eval 'sub O_RDWR {002;}';
    eval 'sub O_NDELAY { &FNDELAY;}';
    eval 'sub O_APPEND { &FAPPEND;}';
    eval 'sub O_CREAT { &FCREAT;}';
    eval 'sub O_TRUNC { &FTRUNC;}';
    eval 'sub O_EXCL { &FEXCL;}';
    eval 'sub O_MASTER {010000;}';
    eval 'sub O_PFS_MASTER {020000;}';
    eval 'sub LOCK_SH {1;}';
    eval 'sub LOCK_EX {2;}';
    eval 'sub LOCK_NB {4;}';
    eval 'sub LOCK_UN {8;}';
    eval 'sub F_OK {0;}';
    eval 'sub X_OK {1;}';
    eval 'sub W_OK {2;}';
    eval 'sub R_OK {4;}';
    eval 'sub L_SET {0;}';
    eval 'sub L_INCR {1;}';
    eval 'sub L_XTND {2;}';
}
1;
