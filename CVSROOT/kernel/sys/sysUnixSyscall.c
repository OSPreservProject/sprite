/* 
 * _Syscall.c --
 *
 *	Table of Unix compatible system calls.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#include <sprite.h>
#include <vmUnixStubs.h>
#include <fsUnixStubs.h>
#include <proc.h>
#include <procUnixStubs.h>
#include <vm.h>
#include <sig.h>
#include <sigUnixStubs.h>
#include <sys.h>
#include <sysInt.h>
#include <timerUnixStubs.h>
#include <mach.h>
#include <stdio.h>


#define SYS_UNIX_ERROR  Sys_UnixError

static int Sys_UnixError _ARGS_((void));
static int Sys_TestStub _ARGS_((int arg0, int arg1, int arg2, int arg3,
	int arg4, int arg5));

/*
 * The system call table.
 * Note: from call #150 on, the calls are different between mips and sunOS.
 */

/*
 * Warning: if a system call with more than 6 arguments is added,
 * sun4.md/machTrap.s will have to be modified to fetch the arguments,
 * since only 6 arguments are passed in registers.
 */

unixSyscallEntry sysUnixSysCallTable[258] = {
    {  SYS_UNIX_ERROR,        0  },      /* indir  */
    { Proc_ExitStub,          1  },      /* exit  */
    { Proc_ForkStub,          0  },      /* fork  */
    { Fs_NewReadStub,         3  },      /* read  */
    { Fs_NewWriteStub,        3  },      /* write  */
    { Fs_NewOpenStub,         3  },      /* open  */
    { Fs_NewCloseStub,        1  },      /* close  */
#if defined(ds3100) || defined(ds5000)
    { SYS_UNIX_ERROR,         0  },      /* old wait  */
#else
    { Proc_Wait4Stub,         4  },      /* wait4  */
#endif
    { Fs_CreatStub,           2  },      /* creat  */
    { Fs_LinkStub,            2  },      /* link  */	
    { Fs_UnlinkStub,          1  },      /* unlink  */
    { Proc_ExecvStub,         2  },      /* execv  */
    { Fs_ChdirStub,           1  },      /* chdir  */
    { SYS_UNIX_ERROR,         0  },      /* old time  */
    { SYS_UNIX_ERROR,         3  },      /* mknod  */
    { Fs_ChmodStub,           2  },      /* chmod  */
    { Fs_ChownStub,           3  },      /* chown  */ 
    { Vm_SbrkStub,            1  },      /* sbrk  */
    { SYS_UNIX_ERROR,         2  },      /* old stat  */
    { Fs_LseekStub,           3  },      /* lseek  */
    { Proc_GetpidStub,        0  },      /* getpid  */
    { SYS_UNIX_ERROR,         5  },      /* mount  */
    { SYS_UNIX_ERROR,         1  },      /* umount  */
    { SYS_UNIX_ERROR,         1  },      /* old setuid  */
    { Proc_GetuidStub,        0  },      /* getuid  */
    { SYS_UNIX_ERROR,         1  },      /* old stime  */
    { Proc_PtraceStub,        4  },      /* ptrace  */
    { SYS_UNIX_ERROR,         1  },      /* old alarm  */
    { SYS_UNIX_ERROR,         2  },      /* old fstat  */
    { SYS_UNIX_ERROR,         0  },      /* old pause  */
    { SYS_UNIX_ERROR,         2  },      /* old utime  */
    { SYS_UNIX_ERROR,         0  },      /* old stty  */
    { SYS_UNIX_ERROR,         0  },      /* old gtty  */
    { Fs_AccessStub,          2  },      /* access  */
    { SYS_UNIX_ERROR,         1  },      /* old nice  */
    { SYS_UNIX_ERROR,         1  },      /* old ftime  */
    { Fs_SyncStub,            0  },      /* sync  */
    { Sig_KillStub,          2  },      /* kill  */
    { Fs_StatStub,            2  },      /* stat  */
    { SYS_UNIX_ERROR,         2  },      /* old setpgrp  */
    { Fs_LstatStub,           2  },      /* lstat  */
    { Fs_DupStub,             1  },      /* dup  */
    { Fs_PipeStub,            1  },      /* pipe  */
    { SYS_UNIX_ERROR,         1  },      /* old times  */
    { SYS_UNIX_ERROR,         4  },      /* profil  */
    { SYS_UNIX_ERROR,         0  },      /* nosys  */
    { SYS_UNIX_ERROR,         1  },      /* old setgid  */
    { Proc_GetgidStub,        0  },      /* getgid  */
    { SYS_UNIX_ERROR,         2  },      /* old sig  */
    { SYS_UNIX_ERROR,         0  },      /* USG 1  */
    { SYS_UNIX_ERROR,         0  },      /* USG 2  #50 */
    { SYS_UNIX_ERROR,         1  },      /* acct  */
    { SYS_UNIX_ERROR,         0  },      /* old set phys addr  */
    { SYS_UNIX_ERROR,         0  },      /* old syslock in core  */
    { Fs_IoctlStub,           3  },      /* ioctl  */
    { Sys_RebootStub,         1  },      /* reboot  */
    { SYS_UNIX_ERROR,         0  },      /* old mpxchan  */
    { Fs_SymlinkStub,         2  },      /* symlink  */
    { Fs_ReadlinkStub,        3  },      /* readlink  */
    { Proc_ExecveStub,        3  },      /* execve  */
    { Proc_UmaskStub,         1  },      /* umask  #60 */
    { SYS_UNIX_ERROR,         1  },      /* chroot  */
    { Fs_FstatStub,           2  },      /* fstat  */
    { SYS_UNIX_ERROR,         0  },      /* used internally  */
    { Vm_GetpagesizeStub,     0  },      /* getpagesize  */
    { SYS_UNIX_ERROR,         5  },      /* mremap  */
    { Proc_VforkStub,         0  },      /* vfork  */
    { SYS_UNIX_ERROR,         0  },      /* old vread  */
    { SYS_UNIX_ERROR,         0  },      /* old vwrite  */
    { SYS_UNIX_ERROR,         1  },      /* new sbrk  */
    { SYS_UNIX_ERROR,         1  },      /* sstk #70 */
    { Vm_MmapStub,            6  },      /* mmap  */
    { SYS_UNIX_ERROR,         1  },      /* old vadvise  */
    { Vm_MunmapStub,          2  },      /* munmap  */
    { Vm_MprotectStub,        3  },      /* mprotect  */
    { SYS_UNIX_ERROR,         3  },      /* madvise  */
    { SYS_UNIX_ERROR,         1  },      /* vhangup  */
    { SYS_UNIX_ERROR,         2  },      /* old vlimit  */
    { Vm_MincoreStub,         3  },      /* mincore  */
    { Proc_GetgroupsStub,     2  },      /* getgroups  */
    { Proc_SetgroupsStub,     2  },      /* setgroups  #80 */
    { Proc_GetpgrpStub,       1  },      /* getpgrp  */
    { Proc_SetpgrpStub,       2  },      /* setpgrp  */
    { Proc_SetitimerStub,     3  },      /* setitimer  */
    { Proc_Wait3Stub,         3  },      /* wait3  */
    { SYS_UNIX_ERROR,         1  },      /* swapon  */
    { Proc_GetitimerStub,     2  },      /* getitimer  */
    { Sys_GethostnameStub,    2  },      /* gethostname  */
    { Sys_SethostnameStub,    2  },      /* sethostname  */
    { Fs_GetdtablesizeStub,   0  },      /* getdtablesize  */
    { Fs_Dup2Stub,            2  },      /* dup2  #90 */
    { Fs_GetdoptStub,         2  },      /* getdopt  */
    { Fs_FcntlStub,           3  },      /* fcntl  */
    { Fs_NewSelectStub,       5  },      /* select  */
    { Fs_SetdoptStub,         2  },      /* setdopt  */
    { Fs_FsyncStub,           1  },      /* fsync  */
    { Proc_SetpriorityStub,   3  },      /* setpriority  */
    { Fs_SocketStub,          3  },      /* socket  */
    { Fs_ConnectStub,         3  },      /* connect  */
    { Fs_AcceptStub,          3  },      /* accept  */
    { Proc_GetpriorityStub,   2  },      /* getpriority  #100 */
    { Fs_SendStub,            4  },      /* send  */
    { Fs_RecvStub,            4  },      /* recv  */
#if defined(ds3100) || defined(ds5000)
    { Mach_SigreturnStub,      1  },      /* sigreturn */
#else
    { SYS_UNIX_ERROR,         0  },      /* old socketaddr (103) */
#endif
    { Fs_BindStub,            3  },      /* bind  */
    { Fs_SetsockoptStub,      5  },      /* setsockopt  */
    { Fs_ListenStub,          2  },      /* listen  */
    { SYS_UNIX_ERROR,         2  },      /* old vtimes  */
    { Sig_SigvecStub,         4  },      /* sigvec  */
    { Sig_SigblockStub,       1  },      /* sigblock  */
    { Sig_SigsetmaskStub,     1  },      /* sigsetmask  #110 */
    { Sig_SigpauseStub,       1  },      /* sigpause  */
    { Sig_SigstackStub,       2  },      /* sigstack  */
    { Fs_RecvmsgStub,         3  },      /* recvmsg  */
    { Fs_SendmsgStub,         3  },      /* sendmsg  */
    { SYS_UNIX_ERROR,         0  },      /* #115  */
    { Timer_GettimeofdayStub, 2  },      /* gettimeofday  */
    { Proc_GetrusageStub,     2  },      /* getrusage  */
    { Fs_GetsockoptStub,      5  },      /* getsockopt  */
    { SYS_UNIX_ERROR,         0  },      /* #119  */
    { Fs_ReadvStub,           3  },      /* readv  #120 */
    { Fs_WritevStub,          3  },      /* writev  */
    { Timer_SettimeofdayStub, 2  },      /* settimeofday  */
    { Fs_FchownStub,          3  },      /* fchown  */
    { Fs_FchmodStub,          2  },      /* fchmod  */
    { Fs_RecvfromStub,        6  },      /* recvfrom  */
    { Proc_SetreuidStub,      2  },      /* setreuid  */
    { Proc_SetregidStub,      2  },      /* setregid  */
    { Fs_NewRenameStub,       2  },      /* rename  */
    { Fs_TruncateStub,        2  },      /* truncate  */
    { Fs_FtruncateStub,       2  },      /* ftruncate  #130 */
    { Fs_FlockStub,           2  },      /* flock  */
    { SYS_UNIX_ERROR,         0  },      /* #132  */
    { Fs_SendtoStub,          6  },      /* sendto  */
    { Sys_ShutdownStub,       2  },      /* shutdown  */
    { Fs_SocketpairStub,      5  },      /* socketpair  */
    { Fs_MkdirStub,           2  },      /* mkdir  */
    { Fs_RmdirStub,           1  },      /* rmdir  */
    { Fs_UtimesStub,          2  },      /* utimes  */
#if defined(ds3100) || defined(ds5000)
    { SYS_UNIX_ERROR,         1  },      /*  #139  */
#else
    { Mach_SigreturnStub,      1  },      /* sigreturn (from longjmp) */
#endif
    { Timer_AdjtimeStub,      2  },      /* adjtime  #140 */
    { Sys_GetpeernameStub,    3  },      /* getpeername  */
    { Sys_GethostidStub,      2  },      /* gethostid  */
    { Sys_SethostidStub,      2  },      /* sethostid  */
    { Sys_GetrlimitStub,      2  },      /* getrlimit  */
    { Sys_SetrlimitStub,      2  },      /* setrlimit  */
    { Sig_KillpgStub,         2  },      /* killpg  */
    { SYS_UNIX_ERROR,         0  },      /* #147  */
    { SYS_UNIX_ERROR,         0  },      /* setquota  */
    { SYS_UNIX_ERROR,         0  },      /* quota  */
    { Fs_GetsocknameStub,     3  },      /* getsockname  #150 */	
#if defined(ds3100) || defined(ds5000)
    { SYS_UNIX_ERROR,         0  },      /* sysmips #151 */
    { SYS_UNIX_ERROR,         0  },      /* cacheflush  */
    { SYS_UNIX_ERROR,         0  },      /* cachectl  */
    { SYS_UNIX_ERROR,         0  },      /* debug  */
    { SYS_UNIX_ERROR,         0  },      /* #155  */
    { SYS_UNIX_ERROR,         0  },      /* #156  */
    { SYS_UNIX_ERROR,         0  },      /* #157  */
    { SYS_UNIX_ERROR,         0  },      /* nfs_svc  */
    { Fs_GetdirentriesStub,   4  },      /* getdirentries  */
    { SYS_UNIX_ERROR,         0  },      /* statfs  #160 */
    { SYS_UNIX_ERROR,         0  },      /* fstatfs  */
    { SYS_UNIX_ERROR,         0  },      /* #162  */
    { SYS_UNIX_ERROR,         0  },      /* #163  */
    { SYS_UNIX_ERROR,         0  },      /* #164  */
    { Sys_GetdomainnameStub,  2  },      /* getdomainname  */
    { Sys_SetdomainnameStub,  2  },      /* setdomainname  */
    { SYS_UNIX_ERROR,         0  },      /* #167  */
    { SYS_UNIX_ERROR,         0  },      /* #168  */
    { SYS_UNIX_ERROR,         0  },      /* exportfs  */
    { SYS_UNIX_ERROR,         0  },      /* #170  */
    { SYS_UNIX_ERROR,         0  },      /* #171  */
    { SYS_UNIX_ERROR,         0  },      /* msgctl  */
    { SYS_UNIX_ERROR,         0  },      /* msgget  */
    { SYS_UNIX_ERROR,         0  },      /* msgrcv  */
    { SYS_UNIX_ERROR,         0  },      /* msgsnd  */
    { SYS_UNIX_ERROR,         0  },      /* semctl  */
    { SYS_UNIX_ERROR,         0  },      /* semget  */
    { SYS_UNIX_ERROR,         0  },      /* semop  */
    { SYS_UNIX_ERROR,         0  },      /* uname  */
    { SYS_UNIX_ERROR,         0  },      /* shmsys  #180 */
    { SYS_UNIX_ERROR,         0  },      /* plock */
    { SYS_UNIX_ERROR,         0  },      /* lockf  */
    { SYS_UNIX_ERROR,         0  },      /* ustat  */
    { SYS_UNIX_ERROR,         0  },      /* getmnt  */
    { SYS_UNIX_ERROR,         0  },      /* mount  */
    { SYS_UNIX_ERROR,         0  },      /* umount  */
    { SYS_UNIX_ERROR,         0  },      /* sigpending  */
    { SYS_UNIX_ERROR,         0  },      /* setsid  */
    { Proc_WaitpidStub,     3  },      /* waitpid  */
#else
    { SYS_UNIX_ERROR,         0  },      /* getmsg  */
    { SYS_UNIX_ERROR,         0  },      /* putmsg  */
    { SYS_UNIX_ERROR,         0  },      /* poll  */
    { SYS_UNIX_ERROR,         0  },      /* #154  */
    { SYS_UNIX_ERROR,         0  },      /* nfs_svc  */
    { Fs_GetdirentriesStub,   4  },      /* getdirentries  */
    { SYS_UNIX_ERROR,         0  },      /* statfs  */
    { SYS_UNIX_ERROR,         0  },      /* fstatfs  */
    { SYS_UNIX_ERROR,         0  },      /* unmount  */
    { SYS_UNIX_ERROR,         0  },      /* async_daemon #160 */
    { SYS_UNIX_ERROR,         0  },      /* getfh  */
    { Sys_GetdomainnameStub,  2  },      /* getdomainname  */
    { Sys_SetdomainnameStub,  2  },      /* setdomainname  */
    { SYS_UNIX_ERROR,         0  },      /* #164  */
    { SYS_UNIX_ERROR,         0  },      /* quotactl  */
    { SYS_UNIX_ERROR,         0  },      /* exportfs  */
    { SYS_UNIX_ERROR,         0  },      /* mount  */
    { SYS_UNIX_ERROR,         0  },      /* ustat  */
    { SYS_UNIX_ERROR,         0  },      /* semsys  */
    { SYS_UNIX_ERROR,         0  },      /* msgsys  #170 */
    { SYS_UNIX_ERROR,         0  },      /* shmsys  */
    { SYS_UNIX_ERROR,         0  },      /* auditsys  */
    { SYS_UNIX_ERROR,         0  },      /* rfssys  */
    { Fs_GetdentsStub,        3  },      /* getdents  */
    { SYS_UNIX_ERROR,         0  },      /* setsid  */
    { SYS_UNIX_ERROR,         0  },      /* fchdir  */
    { SYS_UNIX_ERROR,         0  },      /* fchroot  */
    { SYS_UNIX_ERROR,         0  },      /* vpixsys  */
    { SYS_UNIX_ERROR,         0  },      /* aioread  */
    { SYS_UNIX_ERROR,         0  },      /* aiowrite  #180 */
    { SYS_UNIX_ERROR,         0  },      /* aiowait  */
    { SYS_UNIX_ERROR,         0  },      /* aiocancel  */
    { SYS_UNIX_ERROR,         0  },      /* sigpending  */
    { SYS_UNIX_ERROR,         0  },      /* #184  */
    { SYS_UNIX_ERROR,         0  },      /* setpgid  */
    { SYS_UNIX_ERROR,         0  },      /* pathconf  */
    { SYS_UNIX_ERROR,         0  },      /* fpathconf  */
    { SYS_UNIX_ERROR,         0  },      /* sysconf  */
    { SYS_UNIX_ERROR,         0  },      /* uname  */
#endif
    { SYS_UNIX_ERROR,         0  },      /* #190  */
    { SYS_UNIX_ERROR,         0  },      /* #191  */
    { SYS_UNIX_ERROR,         0  },      /* #192  */
    { SYS_UNIX_ERROR,         0  },      /* #193  */
    { SYS_UNIX_ERROR,         0  },      /* #194  */
    { SYS_UNIX_ERROR,         0  },      /* #195  */
    { SYS_UNIX_ERROR,         0  },      /* #196  */
    { SYS_UNIX_ERROR,         0  },      /* #197  */
    { SYS_UNIX_ERROR,         0  },      /* #198  */
    { SYS_UNIX_ERROR,         0  },      /* #199  */
    { SYS_UNIX_ERROR,         0  },      /* #200  */
    { SYS_UNIX_ERROR,         0  },      /* #201  */
    { SYS_UNIX_ERROR,         0  },      /* #202  */
    { SYS_UNIX_ERROR,         0  },      /* #203  */
    { SYS_UNIX_ERROR,         0  },      /* #204  */
    { SYS_UNIX_ERROR,         0  },      /* #205  */
    { SYS_UNIX_ERROR,         0  },      /* #206  */
    { SYS_UNIX_ERROR,         0  },      /* #207  */
    { SYS_UNIX_ERROR,         0  },      /* #208  */
    { SYS_UNIX_ERROR,         0  },      /* #209  */
    { SYS_UNIX_ERROR,         0  },      /* #210  */
    { SYS_UNIX_ERROR,         0  },      /* #211  */
    { SYS_UNIX_ERROR,         0  },      /* #212  */
    { SYS_UNIX_ERROR,         0  },      /* #213  */
    { SYS_UNIX_ERROR,         0  },      /* #214  */
    { SYS_UNIX_ERROR,         0  },      /* #215  */
    { SYS_UNIX_ERROR,         0  },      /* #216  */
    { SYS_UNIX_ERROR,         0  },      /* #217  */
    { SYS_UNIX_ERROR,         0  },      /* #218  */
    { SYS_UNIX_ERROR,         0  },      /* #219  */
    { SYS_UNIX_ERROR,         0  },      /* #220  */
    { SYS_UNIX_ERROR,         0  },      /* #221  */
    { SYS_UNIX_ERROR,         0  },      /* #222  */
    { SYS_UNIX_ERROR,         0  },      /* #223  */
    { SYS_UNIX_ERROR,         0  },      /* #224  */
    { SYS_UNIX_ERROR,         0  },      /* #225  */
    { SYS_UNIX_ERROR,         0  },      /* #226  */
    { SYS_UNIX_ERROR,         0  },      /* #227  */
    { SYS_UNIX_ERROR,         0  },      /* #228  */
    { SYS_UNIX_ERROR,         0  },      /* #229  */
    { SYS_UNIX_ERROR,         0  },      /* #230  */
    { SYS_UNIX_ERROR,         0  },      /* #231  */
    { SYS_UNIX_ERROR,         0  },      /* #232  */
    { SYS_UNIX_ERROR,         0  },      /* #233  */
    { SYS_UNIX_ERROR,         0  },      /* #234  */
    { SYS_UNIX_ERROR,         0  },      /* #235  */
    { SYS_UNIX_ERROR,         0  },      /* #236  */
    { SYS_UNIX_ERROR,         0  },      /* #237  */
    { SYS_UNIX_ERROR,         0  },      /* #238  */
    { SYS_UNIX_ERROR,         0  },      /* #239  */
    { SYS_UNIX_ERROR,         0  },      /* #240  */
    { SYS_UNIX_ERROR,         0  },      /* #241  */
    { SYS_UNIX_ERROR,         0  },      /* #242  */
    { SYS_UNIX_ERROR,         0  },      /* #243  */
    { SYS_UNIX_ERROR,         0  },      /* #244  */
    { SYS_UNIX_ERROR,         0  },      /* #245  */
    { SYS_UNIX_ERROR,         0  },      /* #246  */
    { SYS_UNIX_ERROR,         0  },      /* #247  */
    { SYS_UNIX_ERROR,         0  },      /* #248  */
    { SYS_UNIX_ERROR,         0  },      /* #249  */
    { SYS_UNIX_ERROR,         0  },      /* #250  */
    { SYS_UNIX_ERROR,         0  },      /* #251  */
    { SYS_UNIX_ERROR,         0  },      /* #252  */
    { SYS_UNIX_ERROR,         0  },      /* #253  */
    { SYS_UNIX_ERROR,         0  },      /* #254  */
    { Sys_TestStub,           4  },      /* #255  */
    { Sys_GetsysinfoStub,     0  },      /* getsysinfo  */
    { SYS_UNIX_ERROR,         0  },      /* setsysinfo  */
};

int sysUnixNumSyscalls =
                sizeof(sysUnixSysCallTable)/sizeof(*sysUnixSysCallTable);

#if defined(ds3100) || defined(ds5000)
int sysCallNum;
#endif

static int
Sys_UnixError()
{
    extern Mach_State *machCurStatePtr;

#ifdef sun3
    printf("Unix system call #%d is not implemented yet.\n",
	machCurStatePtr->userState.lastSysCall);
#endif

#ifdef sun4
    printf("Unix system call #%d is not implemented yet.\n",
	machCurStatePtr->lastSysCall);
#endif

#ifdef ds3100
    printf("Unix system call #%d not implemented yet.\n", sysCallNum);
#endif
    Proc_Exit(1);
    return -1;
}

static int
Sys_TestStub(arg0, arg1, arg2, arg3, arg4, arg5)
    int arg0;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
{

    printf("Sys_TestStub(%x, %x, %x, %x, %x, %x)\n",
	arg0, arg1, arg2, arg3, arg4, arg5);
    return 9999;
}

