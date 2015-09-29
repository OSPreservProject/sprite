# 1 "Mig_GetAllInfo.c"
 















static char rcsid[] = "$Header: /sprite/src/lib/c/mig/RCS/Mig_GetAllInfo.c,v 2.1 90/06/22 14:58:21 douglis Exp $ SPRITE (Berkeley)";



# 1 "/sprite/lib/include/sprite.h" 1
 



















 












typedef int Boolean;

 




typedef int  ReturnStatus;


 











 












 




typedef char *Address;

 






typedef int *ClientData;











# 21 "Mig_GetAllInfo.c" 2

# 1 "/sprite/lib/include/stdio.h" 1
 


































 

















































typedef struct _file {
    unsigned char *lastAccess;	 


    int readCount;		 


    int writeCount;		 





    unsigned char *buffer;	 

    int bufSize;		 


    void (*readProc)();		 
    void (*writeProc)();	 
    int (*closeProc)();		 

    ClientData clientData;	 


    int status;			 


    int flags;			 

    struct _file *nextPtr;	 




} FILE;

 


















 





















































 

























 







 



extern FILE stdioInFile, stdioOutFile, stdioErrFile;




 





 







 







 







extern void	clearerr();
extern int	fclose();
extern FILE *	fdopen();
extern int	fflush();
extern int	fgetc();
extern char *	fgets();
extern int	fileno();
extern FILE *	fopen();
extern int	fprintf();
extern int	fputc();
extern int	fputs();
extern int	fread();
extern FILE *	freopen();
extern int	fscanf();
extern long	fseek();
extern long	ftell();
extern int	fwrite();
extern char *	gets();
extern int	getw();
extern void	perror();
extern FILE *   popen();
extern int      pclose();








extern int	printf();

extern int	puts();
extern int	putw();
extern void	rewind();
extern int	scanf();
extern void	setbuf();
extern void	setbuffer();
extern void	setlinebuf();
extern int	setvbuf();
extern char *	sprintf();
extern int	sscanf();
extern FILE *	tmpfile();
extern char *	tmpnam();
extern char *	tempnam();
extern int	ungetc();
extern int	vfprintf();
extern int	vfscanf();
extern int	vprintf();
extern char *	vsprintf();
extern void	_cleanup();

extern void	Stdio_Setup();


# 22 "Mig_GetAllInfo.c" 2

# 1 "/sprite/lib/include/mig.h" 1
 




















# 1 "/sprite/lib/include/fs.h" 1
 




















# 1 "/sprite/lib/include/spriteTime.h" 1
 























 

 



typedef struct {
    int	seconds;
    int	microseconds;
} Time;

typedef struct {
    int year;
    int month;
    int dayOfYear;
    int dayOfMonth;
    int dayOfWeek;
    int	hours;
    int	minutes;
    int	seconds;
    int localOffset;
    Boolean dst;
} Time_Parts;

 

 








 





 



extern Time time_ZeroSeconds;
extern Time time_OneMicrosecond;
extern Time time_OneMillisecond;
extern Time time_TenMilliseconds;
extern Time time_HundredMilliseconds;
extern Time time_HalfSecond;
extern Time time_OneSecond;
extern Time time_TwoSeconds;
extern Time time_TenSeconds;
extern Time time_OneMinute;
extern Time time_OneHour;
extern Time time_OneDay;
extern Time time_OneYear;
extern Time time_OneLeapYear;


 

extern void 	Time_Add();
extern void 	Time_Subtract();
extern void 	Time_Multiply();
extern void 	Time_Divide();
extern void 	Time_Normalize();
extern void	Time_ToAscii();
extern void	Time_ToParts();


 













































# 22 "/sprite/lib/include/fs.h" 2

# 1 "/sprite/lib/include/proc.h" 1
 





















# 1 "/sprite/lib/include/spriteTime.h" 1
 
















# 142 "/sprite/lib/include/spriteTime.h"

# 23 "/sprite/lib/include/proc.h" 2

# 1 "/sprite/lib/include/sig.h" 1
 




















 




















 


typedef	struct {
    int		action;
    int		(*handler)();
    int		sigHoldMask;
} Sig_Action;

 















































 











 




 













 








 







# 24 "/sprite/lib/include/proc.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/mach.h" 1
 

























# 1 "/sprite/lib/include/sun4.md/kernel/devAddrs.h" 1
 




























 






# 27 "/sprite/lib/include/sun4.md/kernel/mach.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/machConst.h" 1
 





















# 1 "/sprite/lib/include/sun4.md/kernel/vmSunConst.h" 1
 














 





























 





 



 





 












 









 




# 104 "/sprite/lib/include/sun4.md/kernel/vmSunConst.h"


 














































 



































 





















						 




 

































 






























# 291 "/sprite/lib/include/sun4.md/kernel/vmSunConst.h"










 






 


 























 




 









 










 




 







































 










 





 
































 






 













# 23 "/sprite/lib/include/sun4.md/kernel/machConst.h" 2

# 1 "/sprite/lib/include/kernel/sysSysCall.h" 1
 






























































































































# 24 "/sprite/lib/include/sun4.md/kernel/machConst.h" 2

# 1 "/sprite/lib/include/kernel/sys.h" 1
 















# 1 "/sprite/lib/include/kernel/user/sys.h" 1
 
























typedef enum {
    SYS_WARNING,
    SYS_FATAL
} Sys_PanicLevel;

 
















 




 



typedef struct {
    int architecture;		 
    int type;			 
    int	processors;		 
} Sys_MachineInfo;


















extern ReturnStatus		Sys_GetMachineInfo();


# 17 "/sprite/lib/include/kernel/sys.h" 2

# 1 "/sprite/lib/include/sprite.h" 1
 
















# 94 "/sprite/lib/include/sprite.h"

# 18 "/sprite/lib/include/kernel/sys.h" 2

# 1 "/sprite/lib/include/status.h" 1
 







































extern char *Stat_GetMsg();
extern char *Stat_GetPrivateMsg();
extern void Stat_PrintMsg();

 








 



extern ReturnStatus	stat_LastError;
extern void 	  	Stat_SetErrorHandler();
extern void 	  	Stat_SetErrorData();
extern void		Stat_GetErrorHandler();
extern ReturnStatus	Stat_Error();

 





extern int		Compat_MapCode();
extern ReturnStatus	Compat_MapToSprite();


 

 





















 


















 




 













 


































 







 



 
















 














 


# 19 "/sprite/lib/include/kernel/sys.h" 2



 













# 51 "/sprite/lib/include/kernel/sys.h"


 




extern	ReturnStatus	Sys_GetTimeOfDay();
extern	ReturnStatus	Sys_SetTimeOfDay();
extern	ReturnStatus	Sys_DoNothing();
extern	ReturnStatus	Sys_Shutdown();
extern	ReturnStatus	Sys_GetMachineInfo();
extern	ReturnStatus	Sys_GetMachineInfoNew();



# 25 "/sprite/lib/include/sun4.md/kernel/machConst.h" 2

# 1 "/sprite/lib/include/status.h" 1
 






























# 217 "/sprite/lib/include/status.h"

# 26 "/sprite/lib/include/sun4.md/kernel/machConst.h" 2




 













 


















































 









 






 



 




 




















 











 














 





 











 









 









 
















 
























 

































 




 




 





 












						 
 













 





 





 



 




 









 











 





 




 






 




 





 




 





 





 





 




















 





 









































































# 28 "/sprite/lib/include/sun4.md/kernel/mach.h" 2

# 1 "/sprite/lib/include/fmt.h" 1
 



















 








 


typedef int Fmt_Format;

 










 


















 

extern int	Fmt_Convert();
extern int	Fmt_Size();



# 29 "/sprite/lib/include/sun4.md/kernel/mach.h" 2



 


typedef enum {
    MACH_USER,
    MACH_KERNEL
} Mach_ProcessorStates;






 































# 86 "/sprite/lib/include/sun4.md/kernel/mach.h"

 





 





 





 




 




 


extern ReturnStatus (*(mach_NormalHandlers[]))();
extern ReturnStatus (*(mach_MigratedHandlers[]))();

 














# 1 "/sprite/lib/include/sun4.md/kernel/machSig.h" 1
 
























# 1 "/sprite/lib/include/sun4.md/kernel/machConst.h" 1
 











# 522 "/sprite/lib/include/sun4.md/kernel/machConst.h"

# 26 "/sprite/lib/include/sun4.md/kernel/machSig.h" 2



 






























 













typedef struct Mach_RegState {
    unsigned int	curPsr;				 
    unsigned int	pc;
    unsigned int	nextPc;
    unsigned int	tbr;
    unsigned int	y;
    unsigned int	safeTemp;
    unsigned int	volTemp1;
    unsigned int	volTemp2;
    unsigned int	ins[		8 ];		 
						 
    unsigned int	calleeInputs[		8 ];
							 
    unsigned int	extraParams[(10  - 6) ];
    unsigned int	globals[	8 ];	 
    unsigned int 	fsr;   

    int   numQueueEntries;     

    unsigned int	fregs[		32 ];	 



    struct {
	char          *address;	   
	unsigned int instruction;  
    }  fqueue[16 ];   

} Mach_RegState;

 





typedef	struct	Mach_DebugState {
    unsigned int	curPsr;				 
    unsigned int	pc;
    unsigned int	nextPc;
    unsigned int	tbr;
    unsigned int	y;
    unsigned int	safeTemp;
    unsigned int	volTemp1;
    unsigned int	volTemp2;
    unsigned int	ins[		8 ];		 
						 
    unsigned int	calleeInputs[		8 ];
							 
    unsigned int	extraParams[(10  - 6) ];
    unsigned int	globals[	8 ];	 
} Mach_DebugState;


 


typedef struct Mach_SigContext {
    unsigned	int	trapInst;	 
    Address	pcValue;		 
    Mach_RegState	userState;	 
} Mach_SigContext;


# 136 "/sprite/lib/include/sun4.md/kernel/mach.h" 2



typedef struct Mach_RegWindow {
     int locals[		8 ];
     int ins[		8 ];
} Mach_RegWindow;

 







# 1 "/sprite/lib/include/kernel/sig.h" 1
 























# 1 "/sprite/lib/include/sig.h" 1
 

















# 147 "/sprite/lib/include/sig.h"

# 25 "/sprite/lib/include/kernel/sig.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/mach.h" 1
 
















# 325 "/sprite/lib/include/sun4.md/kernel/mach.h"

# 26 "/sprite/lib/include/kernel/sig.h" 2



 


typedef struct {
    int			oldHoldMask;	 


    Mach_SigContext	machContext;	 

} Sig_Context;

 


typedef struct {
    int		sigNum;		 
    int		sigCode;    	 
    Sig_Context	*contextPtr;	 

} Sig_Stack;

 


















 



extern	ReturnStatus	Sig_Send();
extern	ReturnStatus 	Sig_SendProc();
extern	ReturnStatus	Sig_UserSend();
extern	ReturnStatus	Sig_SetHoldMask();
extern	ReturnStatus	Sig_SetAction();
extern	ReturnStatus	Sig_Pause();

extern	void		Sig_Init();
extern	void		Sig_ProcInit();
extern	void		Sig_Fork();
extern	void		Sig_Exec();
extern	void		Sig_ChangeState();
extern	Boolean		Sig_Handle();
extern	void		Sig_CheckForKill();
extern	void		Sig_Return();

 



extern ReturnStatus	Sig_GetEncapSize();
extern ReturnStatus	Sig_EncapState();
extern ReturnStatus	Sig_DeencapState();
extern	void		Sig_AllowMigration();



# 152 "/sprite/lib/include/sun4.md/kernel/mach.h" 2



 


typedef struct Mach_State {
    Mach_RegState	*trapRegs;		 
    Mach_RegState	*switchRegs;		 
    int			savedRegs[	7	][	(		8  + 		8 ) ];
						 








    int			savedMask;		 




    int			savedSps[	7	];
						 




    Address		kernStackStart;		 

    int			fpuStatus;		 
    Sig_Stack		sigStack;		 

    Sig_Context		sigContext;		 

    int			lastSysCall;		 
} Mach_State;

 








 



typedef	struct {
    Mach_RegState	extraSpace;	 
    Sig_Stack		sigStack;	 
    Sig_Context		sigContext;	 
} MachSignalStack;

 




extern	Address	Mach_GetPC();

extern	Boolean	mach_KernelMode;
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel;
extern	int	*mach_NumDisableIntrsPtr;
 


extern	char	*mach_MachineType;
 




extern	Fmt_Format	mach_Format;

 



extern void	Mach_Init();

 




extern	int	debugCounter;
extern	int	debugSpace[];







# 267 "/sprite/lib/include/sun4.md/kernel/mach.h"


 


extern ReturnStatus		Mach_EncapState();
extern ReturnStatus		Mach_DeencapState();
extern ReturnStatus		Mach_GetEncapSize();
extern Boolean			Mach_CanMigrate();

extern void			Mach_SetHandler();
extern void			Mach_InitSyscall();
extern ReturnStatus		Mach_Probe();

 


extern Mach_ProcessorStates	Mach_ProcessorState();
extern int			Mach_GetNumProcessors();

 


extern	void	Mach_GetEtherAddress();
extern	void	Mach_ContextSwitch();
extern	int	Mach_TestAndSet();
extern	int	Mach_GetMachineType();
extern	int	Mach_GetMachineArch();
extern	void	MachFlushWindowsToStack();

 


extern	int		spriteStart;
extern	int		endBss;
extern	int		endText;









 


extern	Address	mach_KernStart;
extern	Address	mach_CodeStart;
extern	Address	mach_StackBottom;
extern	int	mach_KernStackSize;
extern	Address	mach_KernEnd;
extern	Address	mach_FirstUserAddr;
extern	Address	mach_LastUserAddr;
extern	Address	mach_MaxUserStackAddr;
extern	int	mach_LastUserStackPage;


# 25 "/sprite/lib/include/proc.h" 2




# 1 "/sprite/lib/include/vm.h" 1
 














# 1 "/sprite/lib/include/sprite.h" 1
 
















# 94 "/sprite/lib/include/sprite.h"

# 16 "/sprite/lib/include/vm.h" 2

# 1 "/sprite/lib/include/sun4.md/vmMach.h" 1
 















# 1 "/sprite/lib/include/sprite.h" 1
 
















# 94 "/sprite/lib/include/sprite.h"

# 17 "/sprite/lib/include/sun4.md/vmMach.h" 2


 





# 17 "/sprite/lib/include/vm.h" 2

# 1 "/sprite/lib/include/vmStat.h" 1
 

















# 1 "/sprite/lib/include/sun4.md/kernel/vmMachStat.h" 1
 














 



typedef struct {
    int	stealContext;		 

    int	stealPmeg;		 

} VmMachDepStat;


# 19 "/sprite/lib/include/vmStat.h" 2




 

 



typedef struct {
    int	numPhysPages;		 
     


    int	numFreePages;		 
    int	numDirtyPages;		 
    int	numReservePages;	 

    int	numUserPages;		 


    int	kernStackPages;		 
    int kernMemPages;		 

     


    int	totalFaults;		 

    int	totalUserFaults;	 

    int	zeroFilled;		 

    int	fsFilled;		 

    int	psFilled;		 

    int	collFaults;		 

    int	quickFaults;		 

    int	codeFaults;		 
    int	heapFaults;		 
    int	stackFaults;		 
     


    int	numAllocs;		 
    int	gotFreePage;		 
    int	pageAllocs;		 
    int	gotPageFromFS;		 

    int	numListSearches;	 

    int	usedFreePage;		 
    int	lockSearched;		 

    int	refSearched;		 

    int	dirtySearched;		 

    int	reservePagesUsed;	 

     


    int	pagesWritten;		 

    int	cleanWait;		 


    int	pageoutWakeup;		 


    int	pageoutNoWork;		 

    int pageoutWait;		 


     


    int	mapPageWait;		 


    int	accessWait;		 


     


    VmMachDepStat	machDepStat;
     



    int	minVMPages;
     


    int	fsAsked;		 

    int	haveFreePage;		 

    int	fsMap;			 

    int	fsUnmap;		 

    int	maxFSPages;		 

    int	minFSPages;		 

     


    int	numCOWHeapPages;	 
    int	numCOWStkPages;		 
    int numCORHeapPages;	 
    int numCORStkPages;		 
    int	numCOWHeapFaults;	 
    int	numCOWStkFaults;	 
    int	quickCOWFaults;		 
    int numCORHeapFaults;	 
    int numCORStkFaults;	 
    int	quickCORFaults;		 
    int swapPagesCopied;	 
    int	numCORCOWHeapFaults;	 

    int	numCORCOWStkFaults;	 

     


    int	potModPages;		 


    int	notModPages;		 

    int	notHardModPages;	 

     


    int	codePrefetches;		 
    int	heapSwapPrefetches;	 
    int	heapFSPrefetches;	 

    int	stackPrefetches;	 

    int	codePrefetchHits;	 
    int	heapSwapPrefetchHits;	 

    int	heapFSPrefetchHits;	 

    int	stackPrefetchHits;	 

    int	prefetchAborts;		 


} Vm_Stat;

extern	Vm_Stat	vmStat;

# 18 "/sprite/lib/include/vm.h" 2


 


















































































 




 





 



typedef int Vm_SegmentID;

 









 





 



typedef struct Vm_SegmentInfo {
    int			segNum;		 
    int 		refCount;	 

				         

    char		objFileName[50 ];
    int           	type;		 
    int			numPages;	 
    int			ptSize;		 
    int			resPages;	 

    int			flags;		 

    int			ptUserCount;	 

    int			numCOWPages;	 

    int			numCORPages;	 

    Address		minAddr;	 

    Address		maxAddr;	 

    int			traceTime;	 

} Vm_SegmentInfo;

 


extern	ReturnStatus	Vm_PageSize();
extern	ReturnStatus	Vm_CreateVA();
extern	ReturnStatus	Vm_DestroyVA();
extern	ReturnStatus	Vm_Cmd();
extern	ReturnStatus	Vm_GetSegInfo();


# 29 "/sprite/lib/include/proc.h" 2





 




















 



















 



typedef unsigned int 	Proc_PID;


 







 




 




 






 








 







 













 


typedef enum {
    PROC_UNUSED,	 
    PROC_RUNNING,	 
    PROC_READY,		 
    PROC_WAITING,	 

    PROC_EXITING,	 

    PROC_DEAD,		 
    PROC_MIGRATED,	 
    PROC_NEW,		 
    PROC_SUSPENDED	 
} Proc_State;




 
































































 











typedef struct {
    Time kernelCpuUsage;	 
    Time userCpuUsage;		 

    Time childKernelCpuUsage;	 

    Time childUserCpuUsage;	 

    int	numQuantumEnds;		 

    int numWaitEvents;		 


} Proc_ResUsage;

 



typedef enum {
    PROC_GET_THIS_DEBUG,
    PROC_GET_NEXT_DEBUG,
    PROC_CONTINUE,
    PROC_SINGLE_STEP,
    PROC_GET_DBG_STATE,
    PROC_SET_DBG_STATE,
    PROC_READ,
    PROC_WRITE,
    PROC_DETACH_DEBUGGER
} Proc_DebugReq;

 














typedef struct {
    Proc_PID	processID;		 
    int	termReason;			 

    int	termStatus;			 

    int	termCode;			 
    Mach_RegState regState;		 
    int	sigHoldMask;			 
    int	sigPendingMask;			 
    int	sigActions[		32 ]; 	 

    int	sigMasks[		32 ]; 	 

    int	sigCodes[		32 ]; 	 


} Proc_DebugState;

 



typedef struct {
    char *name;		 
    char *value;	 
} Proc_EnvironVar;

 


typedef struct  {
    int		processor;	 



    Proc_State	state;		 
 

    int		genFlags;	 
 

     











    Proc_PID	processID;		 



    Proc_PID	parentID;		 

    int		familyID;		 

    int		userID;			 


    int		effectiveUserID;	 


     









    int		 event;		  

     








    int 	 billingRate;	 

    unsigned int recentUsage;	 
    unsigned int weightedUsage;	 

    unsigned int unweightedUsage;  


     







    Time kernelCpuUsage;	 
    Time userCpuUsage;		 

    Time childKernelCpuUsage;	 

    Time childUserCpuUsage;	 

    int 	numQuantumEnds;		 


    int		numWaitEvents;		 


    unsigned int schedQuantumTicks;	 


     






    Vm_SegmentID		vmSegments[	4 ];


     







    int		sigHoldMask;		 
    int		sigPendingMask;		 
    					 

    int		sigActions[		32 ];
    					 

     






    int		peerHostID;		  


    Proc_PID	peerProcessID;		  

} Proc_PCBInfo;

 








 





 










 







typedef struct {
    Time	interval;	 
    Time	curValue;	 
} Proc_TimerInterval;

 





 







typedef struct {
    char argString[256 ];
} Proc_PCBArgString;


 










































 



extern ReturnStatus Proc_SetExitHandler();
extern void	    Proc_Exit();

 



extern ReturnStatus Proc_Fork();
extern void	    Proc_RawExit();
extern ReturnStatus Proc_Detach();
extern ReturnStatus Proc_Wait();
extern ReturnStatus Proc_RawWait();
extern ReturnStatus Proc_Exec();
extern ReturnStatus Proc_ExecEnv();

extern ReturnStatus Proc_GetIDs();
extern ReturnStatus Proc_SetIDs();
extern ReturnStatus Proc_GetGroupIDs();
extern ReturnStatus Proc_SetGroupIDs();
extern ReturnStatus Proc_GetFamilyID();
extern ReturnStatus Proc_SetFamilyID();

extern ReturnStatus Proc_GetPCBInfo();
extern ReturnStatus Proc_GetResUsage();
extern ReturnStatus Proc_GetPriority();
extern ReturnStatus Proc_SetPriority();

extern ReturnStatus Proc_Debug();
extern ReturnStatus Proc_Profile();

extern ReturnStatus Proc_SetIntervalTimer();
extern ReturnStatus Proc_GetIntervalTimer();

extern ReturnStatus Proc_SetEnviron();
extern ReturnStatus Proc_UnsetEnviron();
extern ReturnStatus Proc_GetEnvironVar();
extern ReturnStatus Proc_GetEnvironRange();
extern ReturnStatus Proc_InstallEnviron();
extern ReturnStatus Proc_CopyEnviron();

extern ReturnStatus Proc_Migrate();




# 23 "/sprite/lib/include/fs.h" 2


 












 











 






































 

 















 





typedef struct Fs_Attributes {
    int	serverID;		 
    int domain;			 
    int fileNumber;		 
    int type;			 
    int size;			 
    int numLinks;		 
    unsigned int permissions;	 
    int uid;			 
    int gid;			 
    int devServerID;		 
    int devType;		 
    int devUnit;		 
    Time createTime;		 
    Time accessTime;		 
    Time descModifyTime;	 
    Time dataModifyTime;	 
    int  blocks;		 
    int  blockSize;		 
    int	version;		 
    int userType;		 
    int pad[4];			 
} Fs_Attributes;

 










 















 






 




























 






















 











typedef struct Fs_FileID {
    int		type;		 


    int		serverID;	 


    int		major;		 
    int		minor;		 
} Fs_FileID;			 

 





typedef struct Fs_UserIDs {
    int user;			 
    int numGroupIDs;		 
    int group[8 ];	 
} Fs_UserIDs;			 




 














































 






 



typedef struct Ioc_RepositionArgs {
    int base;	 
    int offset;	 
} Ioc_RepositionArgs;

 












 























 



typedef struct Ioc_LockArgs {
    int		flags;		 





     




    int		hostID;		 
    Proc_PID	pid;		 
    int		token;		 
} Ioc_LockArgs;





 



typedef struct Ioc_Owner {
    Proc_PID	id;		 
    int		procOrFamily;	 
} Ioc_Owner;




 


typedef struct Ioc_MapArgs {
    int		numBytes;
    Address	address;
} Ioc_MapArgs;

 


typedef struct Ioc_PrefixArgs {
    char	prefix[1024 ];   
} Ioc_PrefixArgs;

 






typedef struct Ioc_WriteBackArgs {
    int		firstByte;	 
    int		lastByte;	 
    Boolean	shouldBlock;	 
} Ioc_WriteBackArgs;

 



typedef struct Ioc_MmapInfoArgs {
    int		isMapped;	 
    int		clientID;	 
} Ioc_MmapInfoArgs;

 






















 











 








 




 















typedef struct Fs_TwoPaths {
    int		pathLen1;	 
    int		pathLen2;	 
    char 	*path1;		 
    char 	*path2;		 
} Fs_TwoPaths;

 


typedef struct {
    int	maxKbytes;		 

    int	freeKbytes;		 



    int	maxFileDesc;		 

    int	freeFileDesc;		 
    int blockSize;		 
    int optSize;		 
} Fs_DomainInfo;


 





typedef struct Fs_Prefix {
    int serverID;		 

    int domain;			 
    int fileNumber;		 
    int version;		 
    int flags;			 
    char prefix[64 ];
    Fs_DomainInfo domainInfo;	 
} Fs_Prefix;








 




typedef struct {
    int		bufSize;	 
    Address	buffer;		 


} Fs_IOVector;


 




typedef struct Fs_Device {
    int		serverID;	 

    int		type;		 

    int		unit;		 

    ClientData	data;		 


} Fs_Device;

 


typedef ClientData Fs_TimeoutHandler;

extern void		    Fs_Dispatch();
extern void		    Fs_EventHandlerCreate();
extern void 		    Fs_EventHandlerDestroy();
extern ClientData 	    Fs_EventHandlerData();
extern ClientData 	    Fs_EventHandlerChangeData();
extern char		    *Fs_GetTempName();
extern int		    Fs_GetTempFile();
extern int                  Fs_IOControl();
extern Boolean		    Fs_IsATerm();
extern Fs_TimeoutHandler    Fs_TimeoutHandlerCreate();
extern void 		    Fs_TimeoutHandlerDestroy();

extern int                  Ioc_ClearBits();
extern int                  Ioc_GetFlags();
extern int                  Ioc_GetOwner();
extern int                  Ioc_Lock();
extern int                  Ioc_Map();
extern int                  Ioc_NumReadable();
extern int                  Ioc_SetBits();
extern int                  Ioc_Reposition();
extern int                  Ioc_SetFlags();
extern int                  Ioc_SetOwner();
extern int                  Ioc_Truncate();
extern int                  Ioc_Unlock();
extern int                  Ioc_WriteBack();


# 22 "/sprite/lib/include/mig.h" 2


 











 





 


 































 











 









 
typedef struct {
    int 	timestamp;			 
    int 	noInput;			 
    int		allowMigration;			 
    int		foreignProcs;			 


    int		utils[3 ];	 
    int		pad;				 
    double	lengths[3 ];	 
} Mig_LoadVector;

typedef struct {
    int		hostID;			 
    int 	bootTime;		 
    int		migVersion;		 

    int		maxProcs;		 

    int		foreign[ ( 2  + 1) ];
    					 


    int		state;			 

    int		pad[4];			 
    Mig_LoadVector loadVec;		 

} Mig_Info;


 




 






























 












typedef struct {
    int firstHost;		 
    int numRecs;		 
} Mig_InfoRequest;


 






typedef struct {
    int numHosts;		 
    int flags;			 
    int priority;		 
    int	virtHost;		 
} Mig_IdleRequest;

 







 




 












 













 


































 








typedef struct {
    unsigned int requested;		 
    unsigned int obtained;		 
    unsigned int evicted;		 

    unsigned int reclaimed;		 


    unsigned int timeUsed;		 

    unsigned int timeToEviction;	 
    unsigned int hostIdleObtained[2];	 

    unsigned int hostIdleEvicted[2];	 


    unsigned int idleTimeWhenActive[2];	 

    unsigned int hostCounts[ 	(	5  + 1) ];  

    int pad[2];				 
} Mig_StatTotals;

typedef struct {
    char arch[12 ];	 

    unsigned int numClients;		 

    unsigned int gotAll;		 

    unsigned int requestDist[20  + 1];
    					 

    unsigned int obtainedDist[20  + 1];
    					 

    unsigned int nonIdleTransitions;	 

    Mig_StatTotals counters;		 

    Mig_StatTotals squared;		 

} Mig_ArchStats;

typedef struct {
    unsigned int version;		 
    unsigned int checkpointInterval;	 

    unsigned int firstRun;		 

    unsigned int restarts;		 

    unsigned int intervals;		 

    unsigned int maxArchs;		 

    unsigned int getLoadRequests;	 

    unsigned int totalRequests;		 

    unsigned int totalObtained;		 

    unsigned int numRepeatRequests;	 


    unsigned int numRepeatAssignments;	 


    unsigned int numFirstAssignments;	 


    Mig_ArchStats archStats[8 ];  
} Mig_Stats;    


 






 




















typedef struct {
    int		criteria;			 

    int		version;			 
    int		noInput;			 
    int		pad;				 
    double	minThresh[3 ];	 

    double	maxThresh[3 ];	 


} Mig_SystemParms;


 


extern int mig_GlobalPdev;     
extern int mig_LocalPdev;


extern Mig_Info *    Mig_GetInfo();
extern int	     Mig_GetAllInfo();
extern int	     Mig_GetIdleNode();
extern int	     Mig_OpenInfo();
extern int	     Mig_UpdateInfo();
extern int	     Mig_Done();
extern int	     Mig_ConfirmIdle();
extern int	     Mig_DeleteHost();
extern int	     Mig_Evict();
extern char *	     Mig_GetPdevName();
extern int	     Mig_OpenPdev();


# 23 "Mig_GetAllInfo.c" 2

# 1 "/sprite/lib/include/kernel/net.h" 1
 




















# 1 "/sprite/lib/include/sprite.h" 1
 
















# 94 "/sprite/lib/include/sprite.h"

# 22 "/sprite/lib/include/kernel/net.h" 2

# 1 "/sprite/lib/include/netEther.h" 1
 
































# 1 "/sprite/lib/include/sun4.md/machparam.h" 1
 

























# 1 "/sprite/lib/include/sun4.md/limits.h" 1
 




















 




 





 




 





 





 




 





 




 





 





# 27 "/sprite/lib/include/sun4.md/machparam.h" 2



 





 









 





 







 








# 34 "/sprite/lib/include/netEther.h" 2

 







 




















 




typedef struct {
    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;
    unsigned char byte4;
    unsigned char byte5;
    unsigned char byte6;
} Net_EtherAddress;




















# 111 "/sprite/lib/include/netEther.h"


 





typedef struct {
	Net_EtherAddress destination;
	Net_EtherAddress source;
	unsigned short   type;		 
} Net_EtherHdr;











# 149 "/sprite/lib/include/netEther.h"


 






 
















# 23 "/sprite/lib/include/kernel/net.h" 2

# 1 "/sprite/lib/include/kernel/netRoute.h" 1
 























# 1 "/sprite/lib/include/netInet.h" 1
 




















# 1 "/sprite/lib/include/sun4.md/machparam.h" 1
 





















# 68 "/sprite/lib/include/sun4.md/machparam.h"

# 22 "/sprite/lib/include/netInet.h" 2

# 1 "/sprite/lib/include/netEther.h" 1
 




























# 174 "/sprite/lib/include/netEther.h"

# 23 "/sprite/lib/include/netInet.h" 2



 



typedef unsigned int Net_InetAddress;


 



typedef struct { 
    unsigned short	hardwareType;		 


    unsigned short	protocolType;		 




    unsigned char	hardwareAddrLen;	 

    unsigned char	protocolAddrLen;	 

    unsigned short	opcode;			 
}  Net_ArpHeader;

typedef struct {
    Net_EtherHdr	header;			 
    unsigned short	hardwareType;		 


    unsigned short	protocolType;		 




    unsigned char	hardwareAddrLen;	 

    unsigned char	protocolAddrLen;	 

    unsigned short	opcode;			 
    Net_EtherAddress	senderEtherAddr;	 

    Net_InetAddress	senderProtAddr;		 

    Net_EtherAddress	targetEtherAddr;	 

    Net_InetAddress	targetProtAddr;		 
} Net_ArpPacket;

 






 



































 






 





 






 



typedef struct {




    unsigned int	version:4,
			headerLen:4;

    unsigned char	typeOfService;	 
    unsigned short	totalLen;	 

    unsigned short	ident;		 





    unsigned int	flags:3,
			fragOffset:13;

    unsigned char	timeToLive;	 


    unsigned char	protocol;	 

    unsigned short	checksum;	 
    Net_InetAddress	source;		 
    Net_InetAddress	dest;		 
     


} Net_IPHeader;

 



typedef struct {
    Net_InetAddress	source;		 
    Net_InetAddress	dest;		 
    unsigned char	zero;		 
    unsigned char	protocol;	 
    unsigned short	len;		 
} Net_IPPseudoHdr;




 















 






























 















 













 









 












 










 







 














 


typedef struct {
    unsigned char	type;
    unsigned char	len;			 
    unsigned char	pointer;		 






    unsigned int	overflow:4,
			flags:4;

} Net_IPTimestampHdr;

 






 



typedef int Net_InetTime;


 



typedef struct {
    unsigned short	srcPort;	 
    unsigned short	destPort;	 
    unsigned int	seqNum;		 
    unsigned int	ackNum;		 






    unsigned int	dataOffset:4,
			reserved:6,
			flags:6;

    unsigned short	window;		 
    unsigned short	checksum;	 

    unsigned short	urgentOffset;	 

} Net_TCPHeader;




 


















 














 



typedef struct {
    unsigned short	srcPort;	 
    unsigned short	destPort;	 
    unsigned short	len;		 
    unsigned short	checksum;	 

} Net_UDPHeader;




 














typedef struct {
    unsigned char	type;		 
    unsigned char	code;		 
    unsigned short	checksum;	 
} Net_ICMPHeader;

 



typedef struct {
    int			unused;	
    Net_IPHeader	ipHeader;
    char		hdrOptions[60  - sizeof(Net_IPHeader)];
    char		data[8];
} Net_ICMPDataMisc;

 


typedef struct {
    Net_InetAddress	gatewayAddr;
    Net_IPHeader	ipHeader;
    char		hdrOptions[60  - sizeof(Net_IPHeader)];
    char		data[8];
} Net_ICMPDataRedir;

 


typedef struct {
    unsigned char	paramOffset;
    char		unused1;
    short		unused2;
    Net_IPHeader	ipHeader;
    char		hdrOptions[60  - sizeof(Net_IPHeader)];
    char		data[8];
} Net_ICMPDataParam;

 



typedef struct {
    unsigned short	id;
    unsigned short	seqNum;
} Net_ICMPDataInfoEcho;


 


typedef struct {
    unsigned short	id;
    unsigned short	seqNum;
    unsigned int	addrMask;
} Net_ICMPDataMask;

 


typedef struct {
    unsigned short	id;
    unsigned short	seqNum;
    unsigned int	origTime;
    unsigned int	recvTime;
    unsigned int	transmitTime;
} Net_ICMPDataTime;


 



typedef struct {
    Net_ICMPHeader	header;
    union {
	Net_ICMPDataMisc	overlay;	 

	Net_ICMPDataMisc	unreach;	 
	Net_ICMPDataMisc	timeExceed;	 
	Net_ICMPDataMisc	quench;		 
	Net_ICMPDataParam	param;		 
	Net_ICMPDataRedir	redirect;	 
	Net_ICMPDataTime	timeStamp;	 
	Net_ICMPDataMask	mask;		 
	Net_ICMPDataInfoEcho	info;		 
	Net_ICMPDataInfoEcho	echo;		 
    } data;
} Net_ICMPPacket;



 




































 




typedef struct {
    short		addrFamily;	 
    unsigned short	port;
    Net_InetAddress	address;
    char		padding[8];	 
} Net_InetSocketAddr;


# 25 "/sprite/lib/include/kernel/netRoute.h" 2

 











typedef struct Net_Route {
    int		flags;		 
    int		spriteID;	 
    int		type;		 
    Address	data;		 
    char 	*name;		 
    char	*machType;	 

} Net_Route;

 















 






 




 


extern Net_Route *netRouteArray[];

 



typedef struct NetInetRoute {
    Net_InetAddress inetAddr;
    Net_EtherAddress gatewayAddress;
} NetInetRoute;

 















typedef struct NetSpriteArp {
    Net_ArpHeader arpHeader;	 

    unsigned char arpData[(2 * (sizeof(Net_EtherAddress) + sizeof(int))) ];
} NetSpriteArp;

 


















 



extern void	 NetArpInput();


# 24 "/sprite/lib/include/kernel/net.h" 2

# 1 "/sprite/lib/include/kernel/user/net.h" 1
 



















# 1 "/sprite/lib/include/sun4.md/machparam.h" 1
 





















# 68 "/sprite/lib/include/sun4.md/machparam.h"

# 21 "/sprite/lib/include/kernel/user/net.h" 2

# 1 "/sprite/lib/include/kernel/user/netEther.h" 1
 




























# 174 "/sprite/lib/include/kernel/user/netEther.h"

# 22 "/sprite/lib/include/kernel/user/net.h" 2

# 1 "/sprite/lib/include/kernel/user/netInet.h" 1
 

















# 593 "/sprite/lib/include/kernel/user/netInet.h"

# 23 "/sprite/lib/include/kernel/user/net.h" 2

# 1 "/sprite/lib/include/kernel/user/netUltra.h" 1
 



















# 1 "/sprite/lib/include/sun4.md/machparam.h" 1
 





















# 68 "/sprite/lib/include/sun4.md/machparam.h"

# 21 "/sprite/lib/include/kernel/user/netUltra.h" 2




typedef struct Net_UltraAddress {
    char		pad1[4];	 

    unsigned int	group:10;	 
    unsigned int	unit:6;		 
    char		pad2[2];	 

} Net_UltraAddress;

# 45 "/sprite/lib/include/kernel/user/netUltra.h"


 






typedef struct Net_UltraTLAddress {
    unsigned char	addressSize;	 
    unsigned char	tsapSize;	 
    char		tsap[4];	 
    Net_UltraAddress	address;	 
} Net_UltraTLAddress;

 






typedef struct Net_UltraHeader {
    unsigned char	cmd;		 
    unsigned char	status;		 
    unsigned short	reference;	 
    int			foo;		 
    Address		buffer;		 
    int			size;		 
    Net_UltraTLAddress	remoteAddress;	 
    unsigned short	pad1;
    Net_UltraTLAddress	localAddress;	 
    unsigned short	pad2;
    unsigned short	options;	 
    unsigned short	quality;	 
    unsigned int	pad3;
} Net_UltraHeader;

 








# 24 "/sprite/lib/include/kernel/user/net.h" 2

# 1 "/sprite/lib/include/kernel/user/sprite.h" 1
 
















# 94 "/sprite/lib/include/kernel/user/sprite.h"

# 25 "/sprite/lib/include/kernel/user/net.h" 2


 



typedef union Net_Address {
    Net_EtherAddress		ether;
    Net_InetAddress		inet;
    Net_UltraAddress		ultra;
    struct { char data[8]; } 	generic;
} Net_Address;

 





 






 







 





 





 



typedef int Net_NetworkType;

 






 


typedef union Net_Header {
    Net_EtherHdr	etherHdr;	 
    struct {				 
	Net_EtherHdr	etherHdr;
	Net_IPHeader	ipHdr;
    } inetHdr;		
} Net_Header;

 






typedef struct Net_RouteInfo {
    int			version;	 
    int			spriteID;	 
    int			interface;	 

    int			protocol;	 
    int			flags;		 
    int			refCount;	 

    int			routeID;	 
    int			maxBytes;	 


    int			minBytes;	 


    Net_NetworkType	netType;	 
    Net_Address		netAddress[2 ];	 


    char		name[100];	 
    char		hostname[100];	 
    char		machType[100];	 
    Net_Header		header;		 
} Net_RouteInfo;


 





 





extern ReturnStatus Net_InstallRoute();

 




 


typedef struct {
    char    data[14];
} Net_GenericAddress;

 



typedef struct Net_SpriteRoute {
    int		flags;		 
    int		spriteID;	 
    int		type;		 
    union {
	Net_EtherHdr	etherHdr;	 
	char		data[14];	 
	struct {
	    Net_EtherHdr	etherHdr;
	    Net_IPHeader	ipHdr;
	} inetHdr;			 
    } route;
} Net_SpriteRoute;


 



extern Net_InetAddress	Net_StringToInetAddr();
extern ReturnStatus	Net_StringToAddr();
extern char 		*Net_InetAddrToString();
extern unsigned int	Net_StringToNetNum();
extern unsigned int	Net_InetAddrHostNum();
extern unsigned int	Net_InetAddrNetNum();
extern unsigned int	Net_InetAddrNetMask();
extern Net_InetAddress	Net_MakeInetAddr();
extern char 		*Net_EtherAddrToString();
extern void		Net_StringToEtherAddr();
extern unsigned short	Net_InetChecksum();
extern unsigned short	Net_InetChecksum2();

















# 25 "/sprite/lib/include/kernel/net.h" 2

# 1 "/sprite/lib/include/kernel/sync.h" 1
 

















































# 1 "/sprite/lib/include/sprite.h" 1
 
















# 94 "/sprite/lib/include/sprite.h"

# 51 "/sprite/lib/include/kernel/sync.h" 2

# 1 "/sprite/lib/include/list.h" 1
 























 














































 



typedef struct List_Links {
    struct List_Links *prevPtr;
    struct List_Links *nextPtr;
} List_Links;

 



void	List_Init();     
void    List_Insert();   
void    List_ListInsert();   
void 	List_Remove();   
void 	List_Move();     

 












    
 



 




























 















 

















 














 














 















 
















 



































# 52 "/sprite/lib/include/kernel/sync.h" 2









# 1 "/sprite/lib/include/sync.h" 1
 

































 














typedef struct Sync_UserLock {
    Boolean inUse;		 
    Boolean waiting;		 
} Sync_UserLock;


typedef Sync_UserLock Sync_Lock;	  


 








typedef struct Sync_Condition {
    Boolean waiting;		 
} Sync_Condition;

 















 



typedef struct {
    int		inUse;				 
    int		class;				 
    int		hit;				 
    int		miss;				 
    char	name[30];			 
    int		priorCount;			 
    int		priorTypes[1 ];	 
    int		activeCount;			 
    int		deadCount;			 
    int		spinCount;			 
} Sync_LockStat;


 




































 



























 




























 



extern ReturnStatus	Sync_GetLock();
extern ReturnStatus	Sync_Unlock();
extern ReturnStatus	Sync_SlowLock();
extern ReturnStatus	Sync_SlowWait();
extern ReturnStatus	Sync_SlowBroadcast();


# 61 "/sprite/lib/include/kernel/sync.h" 2

# 1 "/sprite/lib/include/kernel/proc.h" 1
 





















# 31 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/proc.h" 1
 
















# 590 "/sprite/lib/include/proc.h"

# 32 "/sprite/lib/include/kernel/proc.h" 2

# 1 "/sprite/lib/include/sync.h" 1
 


























# 210 "/sprite/lib/include/sync.h"

# 33 "/sprite/lib/include/kernel/proc.h" 2

# 1 "/sprite/lib/include/kernel/syncLock.h" 1
 


























# 1 "/sprite/lib/include/list.h" 1
 
















# 270 "/sprite/lib/include/list.h"

# 28 "/sprite/lib/include/kernel/syncLock.h" 2

# 1 "/sprite/lib/include/user/sync.h" 1
 


























# 210 "/sprite/lib/include/user/sync.h"

# 29 "/sprite/lib/include/kernel/syncLock.h" 2



 








 






 












 









typedef struct Sync_ListInfo {
    List_Links	links;		 
    Address	lock;		 

} Sync_ListInfo;



 




typedef enum Sync_LockClass {
    SYNC_SEMAPHORE,			
    SYNC_LOCK
} Sync_LockClass;

 


typedef struct Sync_Semaphore {
     


    int value;				 

# 107 "/sprite/lib/include/kernel/syncLock.h"



    char *name;				 
    Address holderPC;			 
    Address holderPCBPtr;		 








} Sync_Semaphore;

 



typedef struct Sync_KernelLock{
     


    Boolean inUse;			 
    Boolean waiting;	        	 
# 142 "/sprite/lib/include/kernel/syncLock.h"



    char *name;				 
    Address holderPC;			 
    Address holderPCBPtr;		 








} Sync_KernelLock;









# 34 "/sprite/lib/include/kernel/proc.h" 2

# 1 "/sprite/lib/include/list.h" 1
 
















# 270 "/sprite/lib/include/list.h"

# 35 "/sprite/lib/include/kernel/proc.h" 2

# 1 "/sprite/lib/include/kernel/timer.h" 1
 



















# 1 "/sprite/lib/include/list.h" 1
 
















# 270 "/sprite/lib/include/list.h"

# 21 "/sprite/lib/include/kernel/timer.h" 2








# 1 "/sprite/lib/include/spriteTime.h" 1
 
















# 142 "/sprite/lib/include/spriteTime.h"

# 29 "/sprite/lib/include/kernel/timer.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/timerTick.h" 1
 



















# 1 "/sprite/lib/include/spriteTime.h" 1
 
















# 142 "/sprite/lib/include/spriteTime.h"

# 21 "/sprite/lib/include/sun4.md/kernel/timerTick.h" 2


 


 



typedef Time Timer_Ticks;


 



 
 extern unsigned int 	timer_IntZeroSeconds; 
 extern unsigned int 	timer_IntOneMillisecond;   
 extern unsigned int 	timer_IntOneSecond;
 extern unsigned int 	timer_IntOneMinute;
 extern unsigned int 	timer_IntOneHour; 
 extern Timer_Ticks	timer_TicksZeroSeconds;
 extern Time 		timer_MaxIntervalTime; 



 
















 







extern void Timer_AddIntervalToTicks();
extern void Timer_GetCurrentTicks();


 




































# 30 "/sprite/lib/include/kernel/timer.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/timerMach.h" 1
 



















 









# 31 "/sprite/lib/include/kernel/timer.h" 2

# 1 "/sprite/lib/include/kernel/syncLock.h" 1
 



















# 164 "/sprite/lib/include/kernel/syncLock.h"


# 32 "/sprite/lib/include/kernel/timer.h" 2




 

















 















































































































  


 






























typedef struct {
    List_Links	links;		 

    void	(*routine)();	 
    Timer_Ticks	time;		 
 
    ClientData	clientData;	 


    Boolean	processed;	 


    unsigned int interval;	 


} Timer_QueueElement;

 



typedef struct {
    int		callback;	 
    int		profile;	 
    int		spurious;	 
    int		schedule;	 
    int		resched;	 
    int		desched;	 
} Timer_Statistics;

extern Timer_Statistics	timer_Statistics;
extern	Time		timer_UniversalApprox;
extern Sync_Semaphore 	timer_ClockMutex;

 



















extern void	Timer_ScheduleRoutine();
extern Boolean  Timer_DescheduleRoutine();

extern void Timer_GetTimeOfDay();
extern void Timer_GetRealTimeOfDay();
extern void Timer_GetRealTimeFromTicks();
extern void Timer_SetTimeOfDay();

extern void Timer_LockRegister();


 




extern void 	Timer_TimerInit();
extern void 	Timer_TimerStart();
extern void	Timer_TimerInactivate();

 



extern	void	Timer_TimerServiceInterrupts();

 


extern void 	Timer_TimerGetInfo();
extern void	Timer_DumpQueue();
extern void	Timer_DumpStats();


# 36 "/sprite/lib/include/kernel/proc.h" 2

# 1 "/sprite/lib/include/kernel/sig.h" 1
 












# 98 "/sprite/lib/include/kernel/sig.h"


# 37 "/sprite/lib/include/kernel/proc.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/mach.h" 1
 
















# 325 "/sprite/lib/include/sun4.md/kernel/mach.h"

# 38 "/sprite/lib/include/kernel/proc.h" 2



 














 





 









 







 

 


typedef struct {
    unsigned int	interval;	 

    ClientData		clientData;	 
    ClientData		token;		 
} Proc_CallInfo;

 



typedef struct {
    int			refCount;	 

    int			size;		 
    struct ProcEnvironVar *varArray;	 
} Proc_EnvironInfo;

 



 




typedef struct {
    List_Links links;			 
    struct Proc_ControlBlock *procPtr;	 
} Proc_PCBLink;


 






typedef union {
    Timer_Ticks	ticks;	 
    Time	time;	 
} Proc_Time;

typedef struct {
    int		type;		 
    Address	lockPtr;	 
} Proc_LockStackElement;

 






typedef struct Proc_ControlBlock {
    List_Links	links;		 

    int		processor;	 



    Proc_State	state;		 

 

    int		genFlags;	 
 
    int		syncFlags;	 
    int		schedFlags;	 
    int		exitFlags;	 


    List_Links		childListHdr;	 
    List_Links		*childList;	 
    Proc_PCBLink	siblingElement;	 

    Proc_PCBLink	familyElement;	 


     











    Proc_PID	processID;		 



    Proc_PID	parentID;		 

    int		familyID;		 

    int		userID;			 


    int		effectiveUserID;	 


     










    int		 event;		  
    Proc_PCBLink eventHashChain;  

     



    Sync_Condition	waitCondition;
    Sync_Condition	lockedCondition;

     




    int			waitToken;

     








    int 	 billingRate;	 

    unsigned int recentUsage;	 
    unsigned int weightedUsage;	 

    unsigned int unweightedUsage;  


     







    Proc_Time kernelCpuUsage;	 
    Proc_Time userCpuUsage;	 

    Proc_Time childKernelCpuUsage;	 

    Proc_Time childUserCpuUsage;	 

    int 	numQuantumEnds;		 


    int		numWaitEvents;		 


    unsigned int schedQuantumTicks;	 



     







 
    struct	Mach_State	*machStatePtr;


     






    struct	Vm_ProcInfo	*vmPtr;

     







    struct Fs_ProcessState	*fsPtr;

     







    int	termReason;		 

				 
    int	termStatus;		 

				 
    int	termCode;		 
				 


     







    int		sigHoldMask;		 
    int		sigPendingMask;		 
    					 

    int		sigActions[		32 ];
    					 

    int		sigMasks[		32 ];
					 

    int		sigCodes[		32 ];
    int		sigFlags;		 

    int		oldSigHoldMask;		 


     





    struct ProcIntTimerInfo	*timerArray;

     






    int			peerHostID;	 


    Proc_PID		peerProcessID; 	 

    struct Proc_ControlBlock
	             *rpcClientProcess;	 



     







     


    Proc_EnvironInfo	*environPtr;

     


    char	*argString;










     





     Sync_Semaphore		lockInfo;


     






    ReturnStatus (**kcallTable)();	 





    int specialHandling;		 





     







     short *Prof_Buffer;     

     int Prof_BufferSize;    
     int Prof_Offset;        
     int Prof_Scale;         

     int Prof_PC;            


     



    Address	remoteExecBuffer;	  

    Address	migCmdBuffer;		  

    int		migCmdBufSize;		  


} Proc_ControlBlock;

 
























 






extern Proc_ControlBlock  **proc_RunningProcesses;


 





extern Proc_ControlBlock **proc_PCBTable;


 



extern int proc_MaxNumProcesses;

 


extern Boolean proc_RefuseMigrations;

 












# 522 "/sprite/lib/include/kernel/proc.h"



 











 










 



# 562 "/sprite/lib/include/kernel/proc.h"
 



extern void		  	Proc_Init();
extern void		  	Proc_InitMainProc();
extern ReturnStatus		Proc_NewProc();
extern void			ProcStartUserProc();
extern void			Proc_ExitInt();
extern void			Proc_Exit();
extern void			Proc_DetachInt();
extern ReturnStatus		Proc_Detach();
extern void			Proc_InformParent();	
extern void			Proc_Reaper();
extern void			Proc_NotifyMigratedWaiters();
extern void			Proc_PutOnDebugList();
extern void			Proc_SuspendProcess();
extern void			Proc_ResumeProcess();
extern int			Proc_ExecEnv();
extern int			Proc_RemoteExec();
extern ReturnStatus 		Proc_GetHostIDs();


extern ReturnStatus		Proc_EvictForeignProcs();
extern ReturnStatus		Proc_EvictProc();
extern Boolean			Proc_IsMigratedProc();
extern void			Proc_FlagMigration();
extern void			Proc_MigrateTrap();
extern void			Proc_OkayToMigrate();
extern ReturnStatus		Proc_MigSendUserInfo();
extern ReturnStatus		Proc_DoRemoteCall();
extern void			Proc_SetEffectiveProc();
extern Proc_ControlBlock *	Proc_GetEffectiveProc();
extern ReturnStatus		Proc_ByteCopy();
extern ReturnStatus		Proc_MakeStringAccessible();
extern void			Proc_MakeUnaccessible();
extern void			Proc_MigrateStartTracing();
extern void			Proc_DestroyMigratedProc();
extern void			Proc_NeverMigrate();
extern ReturnStatus		Proc_MigGetStats();
extern ReturnStatus		Proc_MigZeroStats();
extern   void	        Proc_MigAddToCounter();

extern void			ProcInitMainEnviron();
extern void			ProcSetupEnviron();
extern void			ProcDecEnvironRefCount();

extern void			Proc_SetServerPriority();

extern	int			Proc_KillAllProcesses();
extern	void			Proc_WakeupAllProcesses();

extern	void			Proc_Unlock();
extern	void			Proc_Lock();
extern	Proc_ControlBlock	*Proc_LockPID();
extern	ReturnStatus		Proc_LockFamily();
extern	void			Proc_UnlockFamily();
extern	void			Proc_TakeOffDebugList();
extern	Boolean			Proc_HasPermission();

extern	void			Proc_ServerInit();
extern	void			Proc_CallFunc();
extern	void			Proc_CancelCallFunc();
extern	ClientData		Proc_CallFuncAbsTime();
extern	void			Proc_ServerProc();
extern	int			proc_NumServers;

extern  ReturnStatus		Proc_Dump();
extern  void			Proc_DumpPCB();

extern  void			Proc_RemoveFromLockStack();
extern  void			Proc_PushLockStack();

 






extern ReturnStatus		Proc_SetEnvironStub();
extern ReturnStatus		Proc_UnsetEnvironStub();
extern ReturnStatus		Proc_GetEnvironVarStub();
extern ReturnStatus		Proc_GetEnvironRangeStub();
extern ReturnStatus		Proc_InstallEnvironStub();
extern ReturnStatus		Proc_CopyEnvironStub();
extern int                      Proc_KernExec();


# 62 "/sprite/lib/include/kernel/sync.h" 2

# 1 "/sprite/lib/include/kernel/syncLock.h" 1
 



















# 164 "/sprite/lib/include/kernel/syncLock.h"


# 63 "/sprite/lib/include/kernel/sync.h" 2

# 1 "/sprite/lib/include/kernel/sys.h" 1
 











# 66 "/sprite/lib/include/kernel/sys.h"

# 64 "/sprite/lib/include/kernel/sync.h" 2

# 1 "/sprite/lib/include/sun4.md/kernel/mach.h" 1
 
















# 325 "/sprite/lib/include/sun4.md/kernel/mach.h"

# 65 "/sprite/lib/include/kernel/sync.h" 2



 








 






 












 



typedef struct Sync_Instrument {
    int numWakeups;		 
    int numWakeupCalls;		 
    int numSpuriousWakeups;	 
    int numLocks;		 
    int numUnlocks;		 
    int spinCount[60 +1];  
    int sched_MutexMiss;	 


    char pad[	0x10 ];

} Sync_Instrument;

 




typedef struct Sync_RegElement {
    List_Links 		links;			 
    int			hit;			 
    int			miss;			 
    int			type;			 
    char		*name;			 
    Sync_LockClass	class;			 
    int			priorCount;		 
    int			priorTypes[1 ];  
    int			activeLockCount;	 
    List_Links		activeLocks;		 
    int			deadLockCount;		 
} Sync_RegElement;

 


typedef struct semid_ds Sync_SysVSem;


 



typedef struct {
    List_Links	links;		 
    int		hostID;		 
    Proc_PID	pid;		 
    int		waitToken;	 
} Sync_RemoteWaiter;

 





 


    
extern Sync_Instrument 	sync_Instrument[	1 ];
extern Sync_Instrument	*sync_InstrumentPtr[	1 ];
extern int sync_BusyWaits;

extern 	void 		Sync_Init();

extern 	void 		Sync_WakeupProcess();
extern 	void 		Sync_EventWakeup();
extern 	void 		Sync_WakeWaitingProcess();
extern 	void 		Sync_UnlockAndSwitch();

extern 	Boolean 	Sync_SlowMasterWait();
extern 	Boolean 	Sync_SlowWait();
extern 	Boolean 	Sync_EventWait();
extern 	Boolean 	Sync_WaitTime();
extern 	Boolean 	Sync_WaitTimeInTicks();
extern 	Boolean 	Sync_WaitTimeInterval();

extern 	Boolean 	Sync_ProcWait();
extern 	void 		Sync_ProcWakeup();
extern 	void 		Sync_GetWaitToken();
extern 	void 		Sync_SetWaitToken();
extern 	ReturnStatus 	Sync_RemoteNotify();
extern 	ReturnStatus 	Sync_RemoteNotifyStub();
	
extern 	ReturnStatus 	Sync_SlowLockStub();
extern 	ReturnStatus 	Sync_SlowWaitStub();
extern 	ReturnStatus 	Sync_SlowBroadcastStub();

extern 	void 		Sync_PrintStat();

extern	void		Sync_LockStatInit();
extern	void		Sync_AddPriorInt();
extern	void		SyncDeleteCurrentInt();
extern 	void		SyncMergePriorInt();
extern	void		Sync_RegisterInt();
extern	void		Sync_CheckoutInt();
extern	void		Sync_PrintLockStats();

extern	ReturnStatus	Sync_SemgetStub();
extern	ReturnStatus	Sync_SemopStub();
extern	ReturnStatus	Sync_SemctlStub();

extern Sync_RegElement  *regQueuePtr;



 






































# 260 "/sprite/lib/include/kernel/sync.h"

# 319 "/sprite/lib/include/kernel/sync.h"



 




















# 355 "/sprite/lib/include/kernel/sync.h"


 






 


























 




























 


















 















 










 











 













































 























# 557 "/sprite/lib/include/kernel/sync.h"

# 569 "/sprite/lib/include/kernel/sync.h"












 













































 























# 660 "/sprite/lib/include/kernel/sync.h"

# 670 "/sprite/lib/include/kernel/sync.h"













 



























 















# 736 "/sprite/lib/include/kernel/sync.h"








 















# 769 "/sprite/lib/include/kernel/sync.h"






 






























 






























 






























 


































 


























 

























 




























# 26 "/sprite/lib/include/kernel/net.h" 2


 







 








typedef struct {
    Address		bufAddr;	 
    int			length;		 
    Sync_Semaphore	*mutexPtr;	 

    Boolean		done;		 
} Net_ScatterGather;

 



typedef struct {
    int	packetsRecvd;		 
    int	packetsSent;		 
    int	packetsOutput;		 
    int broadRecvd;		 
    int broadSent;		 
    int others;			 
    int overrunErrors;		 
    int crcErrors;		 
    int fcsErrors;		 
    int frameErrors;		 
    int rangeErrors;		 
    int collisions;		 
    int xmitCollisionDrop;	 

    int	xmitPacketsDropped;	 
    int	recvPacketsDropped;	 
    int matches;		 
    int recvAvgPacketSize;	 
    int recvAvgLargeSize;	 
    int recvAvgSmallSize;	 
    int sentAvgPacketSize;	 
    int sentAvgLargeSize;	 
    int sentAvgSmallSize;	 
    int	bytesSent;		 
    int	bytesReceived;		 
} Net_EtherStats;

 


extern	Net_EtherStats	net_EtherStats;

extern	void	Net_Init();
extern  void    Net_Bin();
extern	void	Net_Reset();
extern	void	Net_Input();
extern	ReturnStatus	Net_Output();
extern	void	Net_OutputRawEther();
extern	void	Net_RecvPoll();
extern	void	Net_EtherOutputSync();
extern	int	Net_Intr();
extern	void	Net_GatherCopy();

extern	ReturnStatus Net_InstallRouteStub();
extern	ReturnStatus Net_InstallRoute();
extern	void	Net_SpriteIDToName();
extern	char *	Net_SpriteIDToMachType();
extern	void	Net_RouteInit();
extern	void	Net_NameToAddr();
extern	void	Net_AddrToName();

extern Net_Route *Net_IDToRoute();
extern int	 Net_AddrToID();

extern int Net_RouteMTU();
extern int Net_MaxProtoHdrSize();

extern Net_Route *Net_Arp();
extern int	Net_RevArp();

extern void	Net_HostPrint();	 

extern void	Net_HdrDestString();


# 24 "Mig_GetAllInfo.c" 2

# 1 "/sprite/lib/include/errno.h" 1
 




















extern int	errno;		 

extern int	sys_nerr;	 
extern char	*sys_errlist[];	 


 














































































# 25 "Mig_GetAllInfo.c" 2


extern int errno;
extern char *strerror();
extern char *malloc();

 




 















int
Mig_GetAllInfo(infoPtr, numRecs)
    Mig_Info *infoPtr;		 
    int numRecs;		 
{
    Mig_InfoRequest request;
    static int init = 0;	 
    static char *buffer; 	 

    static unsigned int bufSize; 
    int status;			 
    int maxRecs = 20 ;	 
    int obtained;		 
    int got;			 
    int nextHost;		 
    int retry = 1;		 


    if (mig_GlobalPdev < 0) {
	if (MigOpenPdev(1 ) < 0) {
	    return(-1);
	}
    }

    if (!init) {
	init = 1;
	bufSize = 2 * sizeof(int) +  maxRecs * sizeof(Mig_Info);
	buffer = malloc(bufSize);
	if (buffer == (char *)  	0 ) {
	    errno = 	12 ;
	    init = 0;
	    return(-1);
	}
    }
    nextHost = 1;

    for (obtained = 0; obtained < numRecs;) {
	request.numRecs = maxRecs;
	request.firstHost = nextHost;

	if (MigSetAlarm() < 0) {
	    fprintf((&stdioErrFile) ,
		    "Error setting alarm for contact with migd.\n");
	    return(-1);
	}
	status = Fs_IOControl(mig_GlobalPdev, 	(	((1<<16)-1) +1) ,
			      sizeof(Mig_InfoRequest),
			      (char *) &request,
			      bufSize, buffer);
	if (MigClearAlarm() < 0) {
	    fprintf((&stdioErrFile) ,
		    "Error clearing alarm for contact with migd.\n");
	}
	if (status != 		0x00000000 ) {
	    close(mig_GlobalPdev);
	    mig_GlobalPdev = 0;
	    if (retry == 0 || MigOpenPdev(1 ) < 0) {
		fprintf((&stdioErrFile) ,
		       "Mig_GetAllInfo: error during ioctl to global master: %s\n",
		       Stat_GetMsg(status));
		errno = Compat_MapCode(status);
		return(-1);
	    }
	    retry = 0;
	    continue;
	}
	retry = 1;
	got = *((int *) buffer);
	if (got == 0) {
	    break;
	}
	obtained += got;
	bcopy(buffer + 2 * sizeof(int), (char *) infoPtr,
	      got * sizeof(Mig_Info));
	if (got < maxRecs) {
	    break;
	}
	nextHost = infoPtr[got-1].hostID + 1;
	infoPtr += got;
    }
    
    return(obtained);
    
}
