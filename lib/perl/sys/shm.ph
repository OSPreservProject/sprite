if (defined &vax) {
    eval 'sub SMLBA { &ctob( &CLSIZE);}';
}
if (defined &mips) {
    eval 'sub SMLBA { &ctob( &NPTEPG);}';
}
sub SHMLBA { &SMLBA;}
sub SM_R {0400;}
sub SM_W {0200;}
sub SHM_R { &SM_R;}
sub SHM_W { &SM_W;}
sub SM_CLEAR {01000;}
sub SM_DEST {02000;}
sub SHM_INIT { &SM_CLEAR;}
sub SHM_DEST { &SM_DEST;}
sub SM_RDONLY {010000;}
sub SM_RND {020000;}
sub SHM_RDONLY { &SM_RDONLY;}
sub SHM_RND { &SM_RND;}
sub SMMNI {100;}
sub shmid_ds { &smem;}
sub key_t {'long';}
sub shm_perm { &sm_perm;}
sub shm_segsz { &sm_size;}
sub shm_cpid { &sm_cpid;}
sub shm_lpid { &sm_lpid;}
sub shm_nattch { &sm_count;}
sub shm_atime { &sm_atime;}
sub shm_dtime { &sm_dtime;}
sub shm_ctime { &sm_ctime;}
sub SMLOCK {010;}
sub SMWANT {020;}
sub SMNOSW {0100;}
sub SHM_LOCK {3;}
sub SHM_UNLOCK {4;}
sub shminfo { &sminfo;}
sub shmmax { &smmax;}
sub shmmin { &smmin;}
sub shmmni { &smmni;}
sub shmseg { &smseg;}
sub shmbrk { &smbrk;}
1;
