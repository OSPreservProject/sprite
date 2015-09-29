if (!defined &_SIGNAL) {
    eval 'sub _SIGNAL {1;}';
    require 'cfuncproto.ph';
    if (!defined &NSIG) {
	eval 'sub NSIG {32;}';
    }
    eval 'sub SIGHUP {1;}';
    eval 'sub SIGINT {2;}';
    eval 'sub SIGQUIT {3;}';
    eval 'sub SIGILL {4;}';
    eval 'sub SIGTRAP {5;}';
    eval 'sub SIGIOT {6;}';
    eval 'sub SIGABRT { &SIGIOT;}';
    eval 'sub SIGEMT {7;}';
    eval 'sub SIGFPE {8;}';
    eval 'sub SIGKILL {9;}';
    eval 'sub SIGBUS {10;}';
    eval 'sub SIGSEGV {11;}';
    eval 'sub SIGSYS {12;}';
    eval 'sub SIGPIPE {13;}';
    eval 'sub SIGALRM {14;}';
    eval 'sub SIGTERM {15;}';
    eval 'sub SIGURG {16;}';
    eval 'sub SIGSTOP {17;}';
    eval 'sub SIGTSTP {18;}';
    eval 'sub SIGCONT {19;}';
    eval 'sub SIGCHLD {20;}';
    eval 'sub SIGCLD { &SIGCHLD;}';
    eval 'sub SIGTTIN {21;}';
    eval 'sub SIGTTOU {22;}';
    eval 'sub SIGIO {23;}';
    eval 'sub SIGXCPU {24;}';
    eval 'sub SIGXFSZ {25;}';
    eval 'sub SIGVTALRM {26;}';
    eval 'sub SIGPROF {27;}';
    eval 'sub SIGWINCH {28;}';
    eval 'sub SIGUSR1 {30;}';
    eval 'sub SIGUSR2 {31;}';
    eval 'sub SIGDEBUG {3;}';
    eval 'sub SIGMIG {10;}';
    eval 'sub SIGMIGHOME {29;}';
    if (!defined &KERNEL) {
    }
    eval 'sub SV_ONSTACK {0x0001;}';
    eval 'sub SV_INTERRUPT {0x0002;}';
    eval 'sub sv_onstack { &sv_flags;}';
    if (0) {
    }
    eval 'sub BADSIG {( &void (*)())-1;}';
    eval 'sub SIG_DFL {( &void (*)())0;}';
    eval 'sub SIG_IGN {( &void (*)())1;}';
    if (defined &KERNEL) {
	eval 'sub SIG_CATCH {( &void (*)())2;}';
	eval 'sub SIG_HOLD {( &void (*)())3;}';
    }
    eval 'sub sigmask {
        local($m) = @_;
        eval "(1 << (($m)-1))";
    }';
}
1;
