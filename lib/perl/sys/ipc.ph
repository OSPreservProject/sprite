require 'sys/types.ph';
sub IPC_ALLOC {0100000;}
sub IPC_CREAT {0001000;}
sub IPC_EXCL {0002000;}
sub IPC_NOWAIT {0004000;}
sub IPC_SYSTEM {0040000;}
sub IPC_PRIVATE {('long')0;}
sub IPC_RMID {0;}
sub IPC_SET {1;}
sub IPC_STAT {2;}
1;
