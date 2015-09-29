#NO_APP
	.stabs "schedQueue.c",100,0,0,Ltext
Ltext:
.stabs "int:t1=r1;-2147483648;2147483647;",128,0,0,0
.stabs "char:t2=r2;0;127;",128,0,0,0
.stabs "unsigned int:t3=r1;0;-1;",128,0,0,0
.stabs "long unsigned int:t4=r1;0;-1;",128,0,0,0
.stabs "short int:t5=r1;-32768;32767;",128,0,0,0
.stabs "long int:t6=r1;-2147483648;2147483647;",128,0,0,0
.stabs "long long int:t7=r1;0;-1;",128,0,0,0
.stabs "short unsigned int:t8=r1;0;65535;",128,0,0,0
.stabs "long long unsigned int:t9=r1;0;-1;",128,0,0,0
.stabs "signed char:t10=r1;-128;127;",128,0,0,0
.stabs "unsigned char:t11=r1;0;255;",128,0,0,0
.stabs "float:t12=r1;4;0;",128,0,0,0
.stabs "double:t13=r1;8;0;",128,0,0,0
.stabs "long double:t14=r1;8;0;",128,0,0,0
.stabs "void:t15=15",128,0,0,0
.stabs "rcsid:S16=ar1;0;102;2",38,0,0,_rcsid
.data
_rcsid:
	.ascii "$Header: /sprite/src/kernel/sched/RCS/schedQueue.c,v 8.2 88/11/22 19:35:56 jhh Exp $ SPRITE (Berkeley)\0"
.stabs "Boolean:t1",128,0,0,0
.stabs "ReturnStatus:t1",128,0,0,0
.stabs "Address:t17=*2",128,0,0,0
.stabs "ClientData:t18=*1",128,0,0,0
.stabs "Mach_ProcessorStates:t19=eMACH_USER:0,MACH_KERNEL:1,;",128,0,0,0
.stabs "Mach_SetJumpState:T20=s52pc:1,0,32;regs:21=ar1;0;11;1,32,384;;",128,0,0,0
.stabs "Mach_SetJumpState:t20",128,0,0,0
.stabs "Mach_VOR:t22=s2stackFormat:1,0,4;vectorOffset:1,6,10;;",128,0,0,0
.stabs "Mach_SpecStatWord:t23=s2rerun:1,0,1;fill1:1,1,1;\\",128,0,0,0
.stabs "ifetch:1,2,1;dfetch:1,3,1;readModWrite:1,4,1;\\",128,0,0,0
.stabs "highByte:1,5,1;byteTrans:1,6,1;readWrite:1,7,1;\\",128,0,0,0
.stabs "fill2:1,8,4;funcCode:1,12,4;;",128,0,0,0
.stabs "Mach_ShortAddrBusErr:t24=s24specStatWord:23,16,16;pipeStageC:5,32,16;\\",128,0,0,0
.stabs "pipeStageB:5,48,16;faultAddr:1,64,32;dataOutBuf:1,128,32;;",128,0,0,0
.stabs "Mach_LongAddrBusErr:t25=s84specStatWord:23,16,16;pipeStageC:5,32,16;\\",128,0,0,0
.stabs "pipeStageB:5,48,16;faultAddr:1,64,32;int1:3,96,32;\\",128,0,0,0
.stabs "dataOutBuf:1,128,32;int2:26=ar1;0;1;3,160,64;\\",128,0,0,0
.stabs "stageBAddr:1,224,32;int3:3,256,32;dataInBuf:1,288,32;\\",128,0,0,0
.stabs "int4:27=ar1;0;10;3,320,352;;",128,0,0,0
.stabs "Mach_AddrBusErr:t25",128,0,0,0
.stabs "Mach_ExcStack:t28=s92statusReg:5,0,16;pc:1,16,32;\\",128,0,0,0
.stabs "vor:22,48,16;tail:29=u84instrAddr:1,0,32;\\",128,0,0,0
.stabs "addrBusErr:25,0,672;;,64,672;;",128,0,0,0
.stabs "Mach_BusErrorReg:t30=s4fill:1,0,24;pageInvalid:1,24,1;\\",128,0,0,0
.stabs "protError:1,25,1;timeOut:1,26,1;vmeBusErr:1,27,1;\\",128,0,0,0
.stabs "fpaBusErr:1,28,1;fpaEnErr:1,29,1;res1:1,30,1;\\",128,0,0,0
.stabs "watchdog:1,31,1;;",128,0,0,0
.stabs "Mach_RegState:t31=s72regs:32=ar1;0;15;1,0,512;\\",128,0,0,0
.stabs "pc:1,512,32;statusReg:1,544,32;;",128,0,0,0
.stabs "Mach_UserState:t33=s76userStackPtr:17,0,32;trapRegs:34=ar1;0;15;1,32,512;\\",128,0,0,0
.stabs "excStackPtr:35=*28,544,32;lastSysCall:1,576,32;;",128,0,0,0
.stabs "Mach_State:T36=s244userState:33,0,608;switchRegs:37=ar1;0;15;1,608,512;\\",128,0,0,0
.stabs "kernStackStart:17,1120,32;setJumpStatePtr:38=*20,1152,32;\\",128,0,0,0
.stabs "sigExcStackSize:1,1184,32;sigExcStack:28,1216,736;;",128,0,0,0
.stabs "Mach_State:t36",128,0,0,0
.stabs "Mach_TrapStack:t39=s116trapType:1,0,32;busErrorReg:30,32,32;\\",128,0,0,0
.stabs "tmpRegs:40=ar1;0;3;1,64,128;excStack:28,192,736;;",128,0,0,0
.stabs "Mach_IntrStack:t41=s108tmpRegs:42=ar1;0;3;1,0,128;\\",128,0,0,0
.stabs "excStack:28,128,736;;",128,0,0,0
.stabs "Mach_SigContext:t43=s172trapInst:1,0,32;userState:33,32,608;\\",128,0,0,0
.stabs "excStack:28,640,736;;",128,0,0,0
.stabs "Time:t44=s8seconds:1,0,32;microseconds:1,32,32;;",128,0,0,0
.stabs "Time_Parts:t45=s40year:1,0,32;month:1,32,32;\\",128,0,0,0
.stabs "dayOfYear:1,64,32;dayOfMonth:1,96,32;dayOfWeek:1,128,32;\\",128,0,0,0
.stabs "hours:1,160,32;minutes:1,192,32;seconds:1,224,32;\\",128,0,0,0
.stabs "localOffset:1,256,32;dst:1,288,32;;",128,0,0,0
.stabs "Sig_Action:t46=s12action:1,0,32;handler:47=*48=f1,32,32;\\",128,0,0,0
.stabs "sigHoldMask:1,64,32;;",128,0,0,0
.stabs "Sig_Context:t49=s176oldHoldMask:1,0,32;machContext:43,32,1376;;",128,0,0,0
.stabs "Sig_Stack:t50=s12sigNum:1,0,32;sigCode:1,32,32;\\",128,0,0,0
.stabs "contextPtr:51=*49,64,32;;",128,0,0,0
.stabs "Proc_PID:t3",128,0,0,0
.stabs "Proc_ResUsage:t52=s40kernelCpuUsage:44,0,64;userCpuUsage:44,64,64;\\",128,0,0,0
.stabs "childKernelCpuUsage:44,128,64;childUserCpuUsage:44,192,64;\\",128,0,0,0
.stabs "numQuantumEnds:1,256,32;numWaitEvents:1,288,32;;",128,0,0,0
.stabs "Proc_DebugReq:t53=ePROC_GET_THIS_DEBUG:0,PROC_GET_NEXT_DEBUG:1,PROC_CONTINUE:2,\\",128,0,0,0
.stabs "PROC_SINGLE_STEP:3,PROC_GET_DBG_STATE:4,PROC_SET_DBG_STATE:5,\\",128,0,0,0
.stabs "PROC_READ:6,PROC_WRITE:7,PROC_DETACH_DEBUGGER:8,;",128,0,0,0
.stabs "Proc_DebugState:t54=s480processID:3,0,32;termReason:1,32,32;\\",128,0,0,0
.stabs "termStatus:1,64,32;termCode:1,96,32;regState:31,128,576;\\",128,0,0,0
.stabs "sigHoldMask:1,704,32;sigPendingMask:1,736,32;sigActions:55=ar1;0;31;1,768,1024;\\",128,0,0,0
.stabs "sigMasks:56=ar1;0;31;1,1792,1024;sigCodes:57=ar1;0;31;1,2816,1024;;",128,0,0,0
.stabs "Proc_EnvironVar:t58=s8name:17,0,32;value:17,32,32;;",128,0,0,0
.stabs "Proc_TimerInterval:t59=s16interval:44,0,64;curValue:44,64,64;;",128,0,0,0
.stabs "Sync_Lock:T60=s8inUse:1,0,32;waiting:1,32,32;;",128,0,0,0
.stabs "Sync_Lock:t60",128,0,0,0
.stabs "Sync_Condition:T61=s4waiting:1,0,32;;",128,0,0,0
.stabs "Sync_Condition:t61",128,0,0,0
.stabs "List_Links:T62=s8prevPtr:63=*62,0,32;nextPtr:63,32,32;;",128,0,0,0
.stabs "List_Links:t62",128,0,0,0
.stabs "Timer_Ticks:t44",128,0,0,0
.stabs "Timer_QueueElement:t64=s32links:62,0,64;routine:65=*66=f15,64,32;\\",128,0,0,0
.stabs "time:44,96,64;clientData:18,160,32;processed:1,192,32;\\",128,0,0,0
.stabs "interval:3,224,32;;",128,0,0,0
.stabs "Timer_Statistics:t67=s24callback:1,0,32;profile:1,32,32;\\",128,0,0,0
.stabs "spurious:1,64,32;schedule:1,96,32;resched:1,128,32;\\",128,0,0,0
.stabs "desched:1,160,32;;",128,0,0,0
.stabs "Proc_CallInfo:t68=s12interval:3,0,32;clientData:18,32,32;\\",128,0,0,0
.stabs "token:18,64,32;;",128,0,0,0
.stabs "Proc_EnvironInfo:t69=s12refCount:1,0,32;size:1,32,32;\\",128,0,0,0
.stabs "varArray:70=*71=xsProcEnvironVar:,64,32;;",128,0,0,0
.stabs "Proc_State:t72=ePROC_UNUSED:0,PROC_RUNNING:1,PROC_READY:2,\\",128,0,0,0
.stabs "PROC_WAITING:3,PROC_EXITING:4,PROC_DEAD:5,PROC_MIGRATED:6,\\",128,0,0,0
.stabs "PROC_NEW:7,PROC_SUSPENDED:8,;",128,0,0,0
.stabs "Proc_PCBLink:t73=s12links:62,0,64;procPtr:74=*75=xsProc_ControlBlock:,64,32;;",128,0,0,0
.stabs "Proc_Time:t76=u8ticks:44,0,64;time:44,0,64;;",128,0,0,0
.stabs "Proc_ControlBlock:T75=s632links:62,0,64;processor:1,64,32;\\",128,0,0,0
.stabs "state:72,96,32;genFlags:1,128,32;syncFlags:1,160,32;\\",128,0,0,0
.stabs "schedFlags:1,192,32;exitFlags:1,224,32;childListHdr:62,256,64;\\",128,0,0,0
.stabs "childList:63,320,32;siblingElement:73,352,96;familyElement:73,448,96;\\",128,0,0,0
.stabs "processID:3,544,32;parentID:3,576,32;familyID:1,608,32;\\",128,0,0,0
.stabs "userID:1,640,32;effectiveUserID:1,672,32;event:1,704,32;\\",128,0,0,0
.stabs "eventHashChain:73,736,96;waitCondition:61,832,32;\\",128,0,0,0
.stabs "lockedCondition:61,864,32;waitToken:1,896,32;billingRate:1,928,32;\\",128,0,0,0
.stabs "recentUsage:3,960,32;weightedUsage:3,992,32;unweightedUsage:3,1024,32;\\",128,0,0,0
.stabs "kernelCpuUsage:76,1056,64;userCpuUsage:76,1120,64;childKernelCpuUsage:76,1184,64;\\",128,0,0,0
.stabs "childUserCpuUsage:76,1248,64;numQuantumEnds:1,1312,32;\\",128,0,0,0
.stabs "numWaitEvents:1,1344,32;schedQuantumTicks:3,1376,32;\\",128,0,0,0
.stabs "machStatePtr:77=*36,1408,32;vmPtr:78=*79=xsVm_ProcInfo:,1440,32;\\",128,0,0,0
.stabs "fsPtr:80=*81=xsFs_ProcessState:,1472,32;termReason:1,1504,32;\\",128,0,0,0
.stabs "termStatus:1,1536,32;termCode:1,1568,32;sigHoldMask:1,1600,32;\\",128,0,0,0
.stabs "sigPendingMask:1,1632,32;sigActions:82=ar1;0;31;1,1664,1024;\\",128,0,0,0
.stabs "sigMasks:83=ar1;0;31;1,2688,1024;sigCodes:84=ar1;0;31;1,3712,1024;\\",128,0,0,0
.stabs "sigFlags:1,4736,32;oldSigHoldMask:1,4768,32;timerArray:85=*86=xsProcIntTimerInfo:,4800,32;\\",128,0,0,0
.stabs "peerHostID:1,4832,32;peerProcessID:3,4864,32;rpcClientProcess:74,4896,32;\\",128,0,0,0
.stabs "environPtr:87=*69,4928,32;argString:17,4960,32;kcallTable:88=*47,4992,32;\\",128,0,0,0
.stabs "specialHandling:1,5024,32;;",128,0,0,0
.stabs "Proc_ControlBlock:t75",128,0,0,0
.stabs "Proc_PCBArgString:t89=s256argString:90=ar1;0;255;2,0,2048;;",128,0,0,0
.stabs "Sys_PanicLevel:t91=eSYS_WARNING:0,SYS_FATAL:1,;",128,0,0,0
.stabs "Sync_Instrument:T92=s20numWakeups:1,0,32;numWakeupCalls:1,32,32;\\",128,0,0,0
.stabs "numSpuriousWakeups:1,64,32;numLocks:1,96,32;numUnlocks:1,128,32;;",128,0,0,0
.stabs "Sync_Instrument:t92",128,0,0,0
.stabs "Sync_Semaphore:T93=s20value:1,0,32;miss:1,32,32;\\",128,0,0,0
.stabs "name:17,64,32;pc:17,96,32;lineInfo:17,128,32;;",128,0,0,0
.stabs "Sync_Semaphore:t93",128,0,0,0
.stabs "Sync_RemoteWaiter:t94=s20links:62,0,64;hostID:1,64,32;\\",128,0,0,0
.stabs "pid:3,96,32;waitToken:1,128,32;;",128,0,0,0
.stabs "Sched_Instrument:T95=s52numContextSwitches:96=ar1;0;0;1,0,32;\\",128,0,0,0
.stabs "numInvoluntarySwitches:97=ar1;0;0;1,32,32;numFullCS:98=ar1;0;0;1,64,32;\\",128,0,0,0
.stabs "noProcessRunning:99=ar1;0;0;44,96,64;idleTime:100=ar1;0;0;44,160,64;\\",128,0,0,0
.stabs "idleTicksLow:101=ar1;0;0;3,224,32;idleTicksOverflow:102=ar1;0;0;3,256,32;\\",128,0,0,0
.stabs "idleTicksPerSecond:3,288,32;numReadyProcesses:1,320,32;\\",128,0,0,0
.stabs "noUserInput:44,352,64;;",128,0,0,0
.stabs "Sched_Instrument:t95",128,0,0,0
.stabs "schedReadyQueueHdrPtr:G63",32,0,0,0
.globl _schedReadyQueueHdrPtr
	.even
_schedReadyQueueHdrPtr:
	.long _schedReadyQueueHeader
.text
LC0:
	.ascii "Deadlock!!!(%s @ 0x%x)\12Holder PC: 0x%x Current PC: 0x%x\12Holder: %s Current: line 50, file schedQueue.c\12\0"
LC1:
	.ascii "line 50, file schedQueue.c\0"
	.even
.globl _Sched_SetClearUsageFlag
_Sched_SetClearUsageFlag:
	.stabd 68,0,46
	link a6,#-4
	moveml #0x2030,sp@-
LBB2:
	.stabd 68,0,49
	movel _proc_RunningProcesses,a0
	movel a0@,a6@(-4)
	.stabd 68,0,50
	addql #1,_sync_Instrument+12
	tstl _mach_AtInterruptLevel
	jne L2
#APP
	movw #0x2700,sr
#NO_APP
	movel _mach_NumDisableIntrsPtr,d0
	movel d0,a0
	addql #1,a0@
L2:
	movel _sched_MutexPtr,a0
	moveq #1,d1
	cmpl a0@,d1
	jne L3
	movel _sched_MutexPtr,a0
	movel a0@(16),sp@-
LBB3:
#APP
	1$:
	lea	1$,a0

#NO_APP
LBE3:
	movel a0,sp@-
	movel _sched_MutexPtr,a0
	movel a0@(12),sp@-
	movel _sched_MutexPtr,sp@-
	movel _sched_MutexPtr,a0
	movel a0@(8),sp@-
	pea LC0
	jbsr _panic
	addw #24,sp
	jra L4
L3:
	movel _sched_MutexPtr,d2
	movel d2,a2
	addql #1,a2@
	movel _sched_MutexPtr,a2
LBB4:
#APP
	1$:
	lea	1$,a3

#NO_APP
LBE4:
	movel a3,a2@(12)
	movel _sched_MutexPtr,a2
	movel #LC1,a2@(16)
L4:
	.stabd 68,0,51
	movel a6@(-4),d2
	movel d2,a2
	moveq #2,d1
	orl d1,a2@(24)
	.stabd 68,0,52
	addql #1,_sync_Instrument+16
	movel _sched_MutexPtr,a2
	clrl a2@
	tstl _mach_AtInterruptLevel
	jne L5
	movel _mach_NumDisableIntrsPtr,a2
	subql #1,a2@
	movel _mach_NumDisableIntrsPtr,a2
	tstl a2@
	jne L6
#APP
	movw #0x2000,sr
#NO_APP
L6:
L5:
LBE2:
	.stabd 68,0,53
L1:
	moveml a6@(-16),#0xc04
	unlk a6
	rts
.stabs "Sched_SetClearUsageFlag:F15",36,0,0,_Sched_SetClearUsageFlag
.stabs "procPtr:74",128,0,0,-4
.stabn 192,0,0,LBB2
.stabs "__pc:r17",64,0,0,8
.stabn 192,0,0,LBB3
.stabn 224,0,0,LBE3
.stabs "__pc:r17",64,0,0,11
.stabn 192,0,0,LBB4
.stabn 224,0,0,LBE4
.stabn 224,0,0,LBE2
	.even
.globl _Sched_MoveInQueue
_Sched_MoveInQueue:
	.stabd 68,0,79
	link a6,#-12
	moveml #0x3c,sp@-
	movel a6@(8),a2
LBB5:
	.stabd 68,0,89
	moveq #2,d0
	andl a2@(24),d0
	tstl d0
	jeq L8
	.stabd 68,0,90
	clrl a2@(120)
	.stabd 68,0,91
	clrl a2@(124)
	.stabd 68,0,92
	clrl a2@(128)
L8:
	.stabd 68,0,107
	movel _proc_RunningProcesses,a0
	movel a0@,a3
	.stabd 68,0,109
	moveq #-1,d1
	cmpl a3,d1
	jeq L9
	movel a2@(124),d1
	cmpl a3@(124),d1
	jcc L9
	.stabd 68,0,110
	movel a3,a0
	moveq #1,d1
	orl d1,a0@(24)
	.stabd 68,0,111
	movel d1,a3@(628)
L9:
	.stabd 68,0,114
	movel _schedReadyQueueHdrPtr,a5
	.stabd 68,0,115
	cmpl a5@(4),a5
	jne L10
	.stabd 68,0,120
	movel a2,a5@(4)
	.stabd 68,0,121
	movel a2,a5@
	.stabd 68,0,122
	movel a5,a2@(4)
	.stabd 68,0,123
	movel a5,a2@
	.stabd 68,0,127
	moveq #1,d1
	movel d1,_sched_Instrument+40
	.stabd 68,0,128
	jra L7
L10:
	.stabd 68,0,145
	moveq #1,d1
	movel d1,a6@(-8)
	.stabd 68,0,146
	clrl a6@(-12)
	.stabd 68,0,147
	movel a5@(4),a4
L11:
	cmpl a4,a5
	jeq L12
	.stabd 68,0,148
	cmpl a4,a2
	jne L14
	.stabd 68,0,149
	clrl a6@(-8)
L14:
	.stabd 68,0,151
	tstl a6@(-12)
	jeq L15
	tstl a6@(-8)
	jne L15
	.stabd 68,0,152
	jra L12
L15:
	.stabd 68,0,154
	tstl a6@(-12)
	jeq L16
	.stabd 68,0,155
	jra L13
L16:
	.stabd 68,0,157
	movel a2@(124),d1
	cmpl a4@(124),d1
	jcc L17
	.stabd 68,0,162
	cmpl a2@(4),a4
	jne L18
	tstl a6@(-8)
	jne L18
	.stabd 68,0,163
	jra L7
L18:
	.stabd 68,0,165
	movel a4,a6@(-4)
	.stabd 68,0,166
	moveq #1,d1
	movel d1,a6@(-12)
L17:
	.stabd 68,0,147
L13:
	movel a4@(4),a4
	jra L11
L12:
	.stabd 68,0,169
	tstl a6@(-8)
	jne L19
	.stabd 68,0,170
	movel a2,sp@-
	jbsr _List_Remove
	addqw #4,sp
	jra L20
L19:
	.stabd 68,0,172
	addql #1,_sched_Instrument+40
L20:
	.stabd 68,0,174
	tstl a6@(-12)
	jeq L21
	.stabd 68,0,176
	movel a6@(-4),a0
	movel a0@,sp@-
	movel a2,sp@-
	jbsr _List_Insert
	addqw #8,sp
	jra L22
L21:
	.stabd 68,0,181
	movel a5@,sp@-
	movel a2,sp@-
	jbsr _List_Insert
	addqw #8,sp
L22:
LBE5:
	.stabd 68,0,183
L7:
	moveml a6@(-28),#0x3c00
	unlk a6
	rts
.stabs "Sched_MoveInQueue:F15",36,0,0,_Sched_MoveInQueue
.stabs "procPtr:p74",160,0,0,8
.stabs "procPtr:r74",64,0,0,10
.stabs "curProcPtr:r74",64,0,0,11
.stabs "itemProcPtr:r74",64,0,0,12
.stabs "queuePtr:r63",64,0,0,13
.stabs "followingItemPtr:63",128,0,0,-4
.stabs "insert:1",128,0,0,-8
.stabs "foundInsertPoint:1",128,0,0,-12
.stabn 192,0,0,LBB5
.stabn 224,0,0,LBE5
	.even
.globl _Sched_InsertInQueue
_Sched_InsertInQueue:
	.stabd 68,0,210
	link a6,#0
	moveml #0x203c,sp@-
	movel a6@(8),a2
LBB6:
	.stabd 68,0,219
	moveq #2,d2
	andl a2@(24),d2
	tstl d2
	jeq L24
	.stabd 68,0,220
	clrl a2@(120)
	.stabd 68,0,221
	clrl a2@(124)
	.stabd 68,0,222
	clrl a2@(128)
L24:
	.stabd 68,0,237
	movel _proc_RunningProcesses,a5
	movel a5@,a3
	.stabd 68,0,239
	moveq #-1,d1
	cmpl a3,d1
	jeq L25
	movel a2@(124),d1
	cmpl a3@(124),d1
	jcc L25
	.stabd 68,0,240
	movel a3,a5
	moveq #1,d1
	orl d1,a5@(24)
	.stabd 68,0,241
	movel d1,a3@(628)
L25:
	.stabd 68,0,244
	movel _schedReadyQueueHdrPtr,a4
	.stabd 68,0,249
	movel a4@(4),a3
L26:
	cmpl a3,a4
	jeq L27
	.stabd 68,0,250
	movel a2@(124),d1
	cmpl a3@(124),d1
	jcc L29
	.stabd 68,0,251
	jra L27
L29:
	.stabd 68,0,249
L28:
	movel a3@(4),a3
	jra L26
L27:
	.stabd 68,0,254
	movel a3@,a3
	.stabd 68,0,255
	tstl a6@(12)
	jeq L30
	.stabd 68,0,259
	cmpl a4@(4),a4
	jeq L32
	cmpl a3,a4
	jeq L32
	jra L31
L32:
	.stabd 68,0,264
	movel a2,d0
	jra L23
	jra L33
L31:
	.stabd 68,0,274
	movel a3@(4),a2@(4)
	.stabd 68,0,276
	movel a3,a2@
	.stabd 68,0,278
	movel a3@(4),a5
	movel a2,a5@
	.stabd 68,0,280
	movel a2,a3@(4)
	.stabd 68,0,281
	movel a4@(4),a2
	.stabd 68,0,286
	movel a2@,a5
	movel a2@(4),a5@(4)
	.stabd 68,0,288
	movel a2@(4),a5
	movel a2@,a5@
	.stabd 68,0,289
	movel a2,d0
	jra L23
L33:
	jra L34
L30:
	.stabd 68,0,292
	addql #1,_sched_Instrument+40
	.stabd 68,0,296
	movel a3@(4),a2@(4)
	.stabd 68,0,297
	movel a3,a2@
	.stabd 68,0,298
	movel a3@(4),a5
	movel a2,a5@
	.stabd 68,0,299
	movel a2,a3@(4)
	.stabd 68,0,300
	moveq #-1,d0
	jra L23
L34:
LBE6:
	.stabd 68,0,302
L23:
	moveml a6@(-20),#0x3c04
	unlk a6
	rts
.stabs "Sched_InsertInQueue:F74",36,0,0,_Sched_InsertInQueue
.stabs "procPtr:p74",160,0,0,8
.stabs "returnProc:p1",160,0,0,12
.stabs "procPtr:r74",64,0,0,10
.stabs "itemProcPtr:r74",64,0,0,11
.stabs "queuePtr:r63",64,0,0,12
.stabn 192,0,0,LBB6
.stabn 224,0,0,LBE6
.stabs "schedReadyQueueHeader:G62",32,0,0,0
.comm _schedReadyQueueHeader,8
