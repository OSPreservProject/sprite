sub SEM_UNDO {010000;}
sub GETNCNT {3;}
sub GETPID {4;}
sub GETVAL {5;}
sub GETALL {6;}
sub GETZCNT {7;}
sub SETVAL {8;}
sub SETALL {9;}
sub GETKEYS {10;}
if (defined &KERNEL) {
}
sub PSEMN {( &PZERO + 3);}
sub PSEMZ {( &PZERO + 2);}
sub SEMVMX {32767;}
sub SEMAEM {16384;}
sub SEM_A {0200;}
sub SEM_R {0400;}
if (!defined &SEMMNI) {
    eval 'sub SEMMNI {10;}';
}
if (!defined &SEMMNS) {
    eval 'sub SEMMNS {60;}';
}
if (!defined &SEMUME) {
    eval 'sub SEMUME {10;}';
}
if (!defined &SEMMNU) {
    eval 'sub SEMMNU {30;}';
}
if (!defined &SEMMAP) {
    eval 'sub SEMMAP {30;}';
}
if (!defined &SEMMSL) {
    eval 'sub SEMMSL { &SEMMNS;}';
}
if (!defined &SEMOPM) {
    eval 'sub SEMOPM {100;}';
}
sub SEMUSZ {($sizeof{'struct sem_undo'}+$sizeof{'struct undo'}* &SEMUME);}
1;
