if (!defined &_RESOURCE) {
    eval 'sub _RESOURCE {1;}';
    eval 'sub PRIO_MIN {-20;}';
    eval 'sub PRIO_MAX {20;}';
    eval 'sub PRIO_PROCESS {0;}';
    eval 'sub PRIO_PGRP {1;}';
    eval 'sub PRIO_USER {2;}';
    eval 'sub RUSAGE_SELF {0;}';
    eval 'sub RUSAGE_CHILDREN {-1;}';
    eval 'sub ru_first { &ru_ixrss;}';
    eval 'sub ru_last { &ru_nivcsw;}';
    eval 'sub RLIMIT_CPU {0;}';
    eval 'sub RLIMIT_FSIZE {1;}';
    eval 'sub RLIMIT_DATA {2;}';
    eval 'sub RLIMIT_STACK {3;}';
    eval 'sub RLIMIT_CORE {4;}';
    eval 'sub RLIMIT_RSS {5;}';
    eval 'sub RLIM_NLIMITS {6;}';
    eval 'sub RLIM_INFINITY {0x7fffffff;}';
}
1;
