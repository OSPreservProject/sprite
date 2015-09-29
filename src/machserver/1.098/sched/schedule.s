LL0:
	.data
	.even
_rcsid:
	.word	9288
	.word	25953
	.word	25701
	.word	29242
	.word	8307
	.word	25448
	.word	25956
	.word	30060
	.word	25902
	.word	25388
	.word	30240
	.word	13614
	.word	13856
	.word	14392
	.word	12080
	.word	13871
	.word	12854
	.word	8240
	.word	14650
	.word	13622
	.word	14900
	.word	12832
	.word	28533
	.word	29556
	.word	25970
	.word	8261
	.word	30832
	.word	8228
	.word	8275
	.word	20562
	.word	18772
	.word	17696
	.word	10306
	.word	25970
	.word	27493
	.word	27749
	.word	31017
	.word	0
	.even
	.globl	_sched_Mutex
_sched_Mutex:
	.long	0
	.lcomm	_gatherInterval,4
	.lcomm	_gatherTicks,8
	.lcomm	_noRecentUsageCharge,4
	.lcomm	_quantumInterval,4
	.lcomm	_quantumTicks,4
	.even
_init:
	.long	0
	.even
	.globl	_sched_DoContextSwitch
_sched_DoContextSwitch:
	.long	0
	.lcomm	_forgetUsageElement,32
	.comm	_sched_Instrument,0x34
	.text
|#PROC# 0
	.globl	_Sched_Init
_Sched_Init:
	link	a6,#0
	addl	#-LF297,sp
	moveml	#LS297,sp@
	movl	_timer_IntOneMillisecond,d0
	lsll	#2,d0
	movl	d0,d1
	lsll	#3,d1
	addl	d1,d0
	addl	d1,d1
	addl	d1,d0
	movl	d0,_quantumInterval
	movl	_timer_IntOneMillisecond,d0
	lsll	#2,d0
	movl	d0,d1
	lsll	#2,d1
	addl	d1,d0
	movl	d0,_gatherInterval
	pea	_gatherTicks
	movl	_gatherInterval,sp@-
	movl	_gatherTicks+0x4,sp@-
	movl	_gatherTicks,sp@-
	jbsr	_Timer_AddIntervalToTicks
	lea	sp@(0x10),sp
	movl	_gatherInterval,d0
	lsrl	#0x1,d0
	movl	d0,d1
	addl	d1,d1
	addl	d1,d0
	lsrl	#0x3,d0
	movl	d0,_noRecentUsageCharge
	movl	_quantumInterval,d0
	movl	_gatherInterval,d1
	jsr	uldivt
	movl	d0,_quantumTicks
	pea	_sched_Instrument
	pea	0x34
	jbsr	_Byte_Zero
	addqw	#0x8,sp
	movl	_schedReadyQueueHdrPtr,sp@-
	jbsr	_List_Init
	addqw	#0x4,sp
	movl	#_Sched_ForgetUsage,_forgetUsageElement+0x8
	clrl	_forgetUsageElement+0x14
	movl	_timer_IntOneSecond,_forgetUsageElement+0x1c
	pea	0x1
	pea	_forgetUsageElement
	jbsr	_Timer_ScheduleRoutine
	addqw	#0x8,sp
	movl	#0x1,_init
LE297:
	unlk	a6
	rts
	LF297 = 0
	LS297 = 0x0
	LFF297 = 0
	LSS297 = 0x0
	LP297 =	0x18
	.data
	.text
|#PROC# 0
	.globl	_Sched_ForgetUsage
_Sched_ForgetUsage:
	link	a6,#0
	addl	#-LF299,sp
	moveml	#LS299,sp@
	addql	#0x1,_sync_Instrument+0xc
	tstl	_mach_AtInterruptLevel
	jne	L301
|#ASMOUT#
movw #0x2700,sr
|#ENDASM#
	movl	_mach_NumDisableIntrsPtr,a0
	addql	#0x1,a0@
L301:
	movl	_sched_Mutex,d0
	addql	#0x1,_sched_Mutex
	cmpl	#0x1,d0
	jne	L302
	.data1
L303:
	.ascii	"Deadlock!!! (sched_Mutex @ 0x%x)\12\0"
	.text
	pea	_sched_Mutex
	pea	L303
	pea	0x1
	jbsr	_Sys_Panic
	lea	sp@(0xc),sp
L302:
	moveq	#0,d7
L306:
	cmpl	_proc_MaxNumProcesses,d7
	jge	L305
	movl	d7,d0
	asll	#0x2,d0
	movl	_proc_PCBTable,a0
	movl	a0@(0,d0:l),a5
	tstl	a5@(0xc)
	jne	L307
	jra	L304
L307:
	movl	a5@(0x80),d0
	movl	d0,d1
	lsll	#2,d1
	addl	d1,d0
	lsrl	#0x3,d0
	movl	d0,a5@(0x80)
	movl	a5@(0x7c),d0
	movl	d0,d1
	lsll	#2,d1
	addl	d1,d0
	lsrl	#0x3,d0
	movl	d0,a5@(0x7c)
L304:
	addql	#0x1,d7
	jra	L306
L305:
	pea	0x1
	pea	_forgetUsageElement
	jbsr	_Timer_RescheduleRoutine
	addqw	#0x8,sp
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L308
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L309
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L309:
L308:
LE299:
	moveml	a6@(-0x8),#0x2080
	unlk	a6
	rts
	LF299 = 8
	LS299 = 0x2080
	LFF299 = 0
	LSS299 = 0x0
	LP299 =	0x14
	.data
	.text
|#PROC# 0
	.globl	_Sched_GatherProcessInfo
_Sched_GatherProcessInfo:
	link	a6,#0
	addl	#-LF310,sp
	moveml	#LS310,sp@
	tstl	_init
	jne	L312
	jra	LE310
L312:
	addql	#0x1,_sync_Instrument+0xc
	tstl	_mach_AtInterruptLevel
	jne	L313
|#ASMOUT#
movw #0x2700,sr
|#ENDASM#
	movl	_mach_NumDisableIntrsPtr,a0
	addql	#0x1,a0@
L313:
	movl	_sched_Mutex,d0
	addql	#0x1,_sched_Mutex
	cmpl	#0x1,d0
	jne	L314
	.data1
L315:
	.ascii	"Deadlock!!! (sched_Mutex @ 0x%x)\12\0"
	.text
	pea	_sched_Mutex
	pea	L315
	pea	0x1
	jbsr	_Sys_Panic
	lea	sp@(0xc),sp
L314:
	moveq	#0,d7
L318:
	cmpl	_mach_NumProcessors,d7
	jge	L317
	movl	d7,d0
	asll	#0x2,d0
	movl	_proc_RunningProcesses,a0
	movl	a0@(0,d0:l),a5
	cmpl	#-0x1,a5
	jne	L319
	pea	_sched_Instrument+0xc
	movl	_gatherTicks+0x4,sp@-
	movl	_gatherTicks,sp@-
	movl	_sched_Instrument+0x10,sp@-
	movl	_sched_Instrument+0xc,sp@-
	jbsr	_Time_Add
	lea	sp@(0x14),sp
	jra	L316
L319:
	movl	d7,sp@-
	jbsr	_Mach_ProcessorState
	addqw	#0x4,sp
	cmpl	#0x1,d0
	jne	L320
	pea	a5@(0x84)
	movl	_gatherTicks+0x4,sp@-
	movl	_gatherTicks,sp@-
	movl	a5@(0x88),sp@-
	movl	a5@(0x84),sp@-
	jbsr	_Time_Add
	lea	sp@(0x14),sp
	jra	L321
L320:
	pea	a5@(0x8c)
	movl	_gatherTicks+0x4,sp@-
	movl	_gatherTicks,sp@-
	movl	a5@(0x90),sp@-
	movl	a5@(0x8c),sp@-
	jbsr	_Time_Add
	lea	sp@(0x14),sp
L321:
	movl	_gatherInterval,d0
	addl	d0,a5@(0x78)
	movl	a5@(0x10),d0
	andl	#0x2,d0
	jeq	L322
	cmpl	#0x2,a5@(0x74)
	jeq	L322
	tstl	a5@(0xac)
	jeq	L323
	subql	#0x1,a5@(0xac)
L323:
	tstl	a5@(0xac)
	jne	L324
	pea	a5@
	jbsr	_QuantumEnd
	addqw	#0x4,sp
L324:
L322:
L316:
	addql	#0x1,d7
	jra	L318
L317:
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L325
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L326
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L326:
L325:
LE310:
	moveml	a6@(-0x8),#0x2080
	unlk	a6
	rts
	LF310 = 8
	LS310 = 0x2080
	LFF310 = 0
	LSS310 = 0x0
	LP310 =	0x1c
	.data
	.text
|#PROC# 0
	.globl	_Sched_ContextSwitchInt
_Sched_ContextSwitchInt:
	link	a6,#0
	addl	#-LF327,sp
	moveml	#LS327,sp@
	addql	#0x1,_sched_Instrument
	movl	_proc_RunningProcesses,a0
	movl	a0@,a5
	andl	#-0x2,a5@(0x18)
	pea	a5@
	jbsr	_RememberUsage
	addqw	#0x4,sp
	cmpl	#0x2,a6@(0x8)
	jne	L329
	addql	#0x1,a5@(0xa4)
	movl	_schedReadyQueueHdrPtr,a0
	movl	_schedReadyQueueHdrPtr,d0
	cmpl	a0@(0x4),d0
	jne	L330
	movl	_quantumTicks,a5@(0xac)
	jra	LE327
L330:
	pea	0x1
	pea	a5@
	jbsr	_Sched_InsertInQueue
	addqw	#0x8,sp
	movl	d0,a4
	cmpl	a5,a4
	jne	L331
	movl	_quantumTicks,a5@(0xac)
	jra	LE327
L331:
	movl	#0x2,a5@(0xc)
	jra	L332
L329:
	cmpl	#0x3,a6@(0x8)
	jne	L333
	addql	#0x1,a5@(0xa8)
L333:
	movl	a6@(0x8),a5@(0xc)
	jbsr	_IdleLoop
	movl	d0,a4
L332:
	movl	#0x1,a4@(0xc)
	movl	_proc_RunningProcesses,a0
	movl	a4,a0@
	movl	_quantumTicks,a4@(0xac)
	cmpl	a5,a4
	jne	L334
	jra	LE327
L334:
	addql	#0x1,_sched_Instrument+0x8
	pea	a4@
	pea	a5@
	jbsr	_Mach_ContextSwitch
	addqw	#0x8,sp
LE327:
	moveml	a6@(-0x8),#0x3000
	unlk	a6
	rts
	LF327 = 8
	LS327 = 0x3000
	LFF327 = 0
	LSS327 = 0x0
	LP327 =	0x10
	.data
	.text
|#PROC# 0
_RememberUsage:
	link	a6,#0
	addl	#-LF335,sp
	moveml	#LS335,sp@
	movl	a6@(8),a5
	movl	a5@(0x74),d6
	movl	a5@(0x78),d0
	movl	d0,d1
	addl	d1,d1
	addl	d1,d0
	lsrl	#0x3,d0
	movl	d0,d7
	tstl	d7
	jne	L337
	movl	_noRecentUsageCharge,d7
L337:
	addl	d7,a5@(0x80)
	tstl	d6
	jlt	L338
	cmpl	#0x2,d6
	jeq	L339
	movl	d7,d0
	lsrl	d6,d0
	addl	d0,a5@(0x7c)
L339:
	jra	L340
L338:
	movl	d6,d0
	negl	d0
	movl	d7,d1
	lsll	d0,d1
	addl	d1,a5@(0x7c)
L340:
	clrl	a5@(0x78)
LE335:
	moveml	a6@(-0xc),#0x20c0
	unlk	a6
	rts
	LF335 = 12
	LS335 = 0x20c0
	LFF335 = 0
	LSS335 = 0x0
	LP335 =	0x8
	.data
	.text
|#PROC# 030
_IdleLoop:
	link	a6,#0
	addl	#-LF341,sp
	moveml	#LS341,sp@
	movl	_schedReadyQueueHdrPtr,a4
L343:
	cmpl	a4@(0x4),a4
	jne	L344
	movl	_proc_RunningProcesses,a0
	movl	#-0x1,a0@
	cmpl	#-0x1,_sched_Instrument+0x1c
	jne	L345
	clrl	_sched_Instrument+0x1c
	addql	#0x1,_sched_Instrument+0x20
	jra	L346
L345:
	addql	#0x1,_sched_Instrument+0x1c
L346:
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L347
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L348
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L348:
L347:
	addql	#0x1,_sync_Instrument+0xc
	tstl	_mach_AtInterruptLevel
	jne	L349
|#ASMOUT#
movw #0x2700,sr
|#ENDASM#
	movl	_mach_NumDisableIntrsPtr,a0
	addql	#0x1,a0@
L349:
	movl	_sched_Mutex,d0
	addql	#0x1,_sched_Mutex
	cmpl	#0x1,d0
	jne	L350
	.data1
L351:
	.ascii	"Deadlock!!! (sched_Mutex @ 0x%x)\12\0"
	.text
	pea	_sched_Mutex
	pea	L351
	pea	0x1
	jbsr	_Sys_Panic
	lea	sp@(0xc),sp
L350:
	jra	L343
L344:
	movl	a4@(0x4),a5
	cmpl	#0x2,a5@(0xc)
	jeq	L352
	.data1
L353:
	.ascii	"Non-ready process found in ready queue.\12\0"
	.text
	pea	L353
	pea	0x1
	jbsr	_Sys_Panic
	addqw	#0x8,sp
L352:
	movl	a5@,a0
	movl	a5@(0x4),a0@(0x4)
	movl	a5@(0x4),a0
	movl	a5@,a0@
	subql	#0x1,_sched_Instrument+0x28
	movl	a5,d0
	jra	LE341
LE341:
	moveml	a6@(-0x8),#0x3000
	unlk	a6
	rts
	LF341 = 8
	LS341 = 0x3000
	LFF341 = 0
	LSS341 = 0x0
	LP341 =	0x14
	.data
	.text
|#PROC# 0
	.globl	_Sched_TimeTicks
_Sched_TimeTicks:
	link	a6,#0
	addl	#-LF354,sp
	moveml	#LS354,sp@
	pea	a6@(-0x8)
	pea	0x5
	movl	_time_OneSecond+0x4,sp@-
	movl	_time_OneSecond,sp@-
	jbsr	_Time_Multiply
	lea	sp@(0x10),sp
	.data1
L356:
	.ascii	"Idling for 5 seconds...\0"
	.text
	pea	L356
	jbsr	_Sys_Printf
	addqw	#0x4,sp
	movl	_sched_Instrument+0x1c,d7
	movl	a6@(-0x4),sp@-
	movl	a6@(-0x8),sp@-
	jbsr	_Sync_WaitTime
	addqw	#0x8,sp
	movl	_sched_Instrument+0x1c,d0
	subl	d7,d0
	movl	d0,d7
	.data1
L357:
	.ascii	" %d ticks\12\0"
	.text
	movl	d7,sp@-
	pea	L357
	jbsr	_Sys_Printf
	addqw	#0x8,sp
	movl	d7,d0
	jsr	Ffltd
	lea	L2000000,a0
	jsr	Fdivd
	jsr	Fintd
	movl	d0,_sched_Instrument+0x24
LE354:
	moveml	a6@(-0xc),#0x80
	unlk	a6
	rts
	LF354 = 12
	LS354 = 0x80
	LFF354 = 8
	LSS354 = 0x0
	LP354 =	0x18
L2000000:	.long	0x40140000,0x0
	.data
	.text
|#PROC# 0
_QuantumEnd:
	link	a6,#0
	addl	#-LF358,sp
	moveml	#LS358,sp@
	movl	a6@(8),a5
	orl	#0x1,a5@(0x18)
	movl	#0x1,a5@(0x274)
	tstl	_mach_KernelMode
	jne	L360
	movl	#0x1,_sched_DoContextSwitch
L360:
LE358:
	moveml	a6@(-0x4),#0x2000
	unlk	a6
	rts
	LF358 = 4
	LS358 = 0x2000
	LFF358 = 0
	LSS358 = 0x0
	LP358 =	0x8
	.data
	.text
|#PROC# 0
	.globl	_Sched_PrintStat
_Sched_PrintStat:
	link	a6,#0
	addl	#-LF361,sp
	moveml	#LS361,sp@
	.data1
L363:
	.ascii	"Sched Statistics\12\0"
	.text
	pea	L363
	jbsr	_Sys_Printf
	addqw	#0x4,sp
	.data1
L364:
	.ascii	"numContextSwitches = %d\12\0"
	.text
	movl	_sched_Instrument,sp@-
	pea	L364
	jbsr	_Sys_Printf
	addqw	#0x8,sp
	.data1
L365:
	.ascii	"numFullSwitches    = %d\12\0"
	.text
	movl	_sched_Instrument+0x8,sp@-
	pea	L365
	jbsr	_Sys_Printf
	addqw	#0x8,sp
	.data1
L366:
	.ascii	"numInvoluntary     = %d\12\0"
	.text
	movl	_sched_Instrument+0x4,sp@-
	pea	L366
	jbsr	_Sys_Printf
	addqw	#0x8,sp
	pea	a6@(-0x8)
	movl	_sched_Instrument+0x10,sp@-
	movl	_sched_Instrument+0xc,sp@-
	jbsr	_Timer_TicksToTime
	lea	sp@(0xc),sp
	.data1
L367:
	.ascii	"Idle Time          = %d.%06d seconds\12\0"
	.text
	movl	a6@(-0x4),sp@-
	movl	a6@(-0x8),sp@-
	pea	L367
	jbsr	_Sys_Printf
	lea	sp@(0xc),sp
LE361:
	unlk	a6
	rts
	LF361 = 8
	LS361 = 0x0
	LFF361 = 8
	LSS361 = 0x0
	LP361 =	0x14
	.data
	.text
|#PROC# 0
	.globl	_Sched_LockAndSwitch
_Sched_LockAndSwitch:
	link	a6,#0
	addl	#-LF368,sp
	moveml	#LS368,sp@
	addql	#0x1,_sync_Instrument+0xc
	tstl	_mach_AtInterruptLevel
	jne	L370
|#ASMOUT#
movw #0x2700,sr
|#ENDASM#
	movl	_mach_NumDisableIntrsPtr,a0
	addql	#0x1,a0@
L370:
	movl	_sched_Mutex,d0
	addql	#0x1,_sched_Mutex
	cmpl	#0x1,d0
	jne	L371
	.data1
L372:
	.ascii	"Deadlock!!! (sched_Mutex @ 0x%x)\12\0"
	.text
	pea	_sched_Mutex
	pea	L372
	pea	0x1
	jbsr	_Sys_Panic
	lea	sp@(0xc),sp
L371:
	addql	#0x1,_sched_Instrument+0x4
	pea	0x2
	jbsr	_Sched_ContextSwitchInt
	addqw	#0x4,sp
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L373
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L374
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L374:
L373:
LE368:
	unlk	a6
	rts
	LF368 = 0
	LS368 = 0x0
	LFF368 = 0
	LSS368 = 0x0
	LP368 =	0x14
	.data
	.text
|#PROC# 0
	.globl	_Sched_ContextSwitch
_Sched_ContextSwitch:
	link	a6,#0
	addl	#-LF375,sp
	moveml	#LS375,sp@
	addql	#0x1,_sync_Instrument+0xc
	tstl	_mach_AtInterruptLevel
	jne	L377
|#ASMOUT#
movw #0x2700,sr
|#ENDASM#
	movl	_mach_NumDisableIntrsPtr,a0
	addql	#0x1,a0@
L377:
	movl	_sched_Mutex,d0
	addql	#0x1,_sched_Mutex
	cmpl	#0x1,d0
	jne	L378
	.data1
L379:
	.ascii	"Deadlock!!! (sched_Mutex @ 0x%x)\12\0"
	.text
	pea	_sched_Mutex
	pea	L379
	pea	0x1
	jbsr	_Sys_Panic
	lea	sp@(0xc),sp
L378:
	movl	a6@(0x8),sp@-
	jbsr	_Sched_ContextSwitchInt
	addqw	#0x4,sp
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L380
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L381
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L381:
L380:
LE375:
	unlk	a6
	rts
	LF375 = 0
	LS375 = 0x0
	LFF375 = 0
	LSS375 = 0x0
	LP375 =	0x14
	.data
	.text
|#PROC# 0
	.globl	_Sched_StartKernProc
_Sched_StartKernProc:
	link	a6,#0
	addl	#-LF382,sp
	moveml	#LS382,sp@
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L384
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L385
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L385:
L384:
	movl	a6@(0x8),a0
	jsr	a0@
	pea	0
	jbsr	_Proc_Exit
	addqw	#0x4,sp
LE382:
	unlk	a6
	rts
	LF382 = 0
	LS382 = 0x0
	LFF382 = 0
	LSS382 = 0x0
	LP382 =	0xc
	.data
	.text
|#PROC# 0
	.globl	_Sched_MakeReady
_Sched_MakeReady:
	link	a6,#0
	addl	#-LF386,sp
	moveml	#LS386,sp@
	movl	a6@(8),a5
	addql	#0x1,_sync_Instrument+0xc
	tstl	_mach_AtInterruptLevel
	jne	L388
|#ASMOUT#
movw #0x2700,sr
|#ENDASM#
	movl	_mach_NumDisableIntrsPtr,a0
	addql	#0x1,a0@
L388:
	movl	_sched_Mutex,d0
	addql	#0x1,_sched_Mutex
	cmpl	#0x1,d0
	jne	L389
	.data1
L390:
	.ascii	"Deadlock!!! (sched_Mutex @ 0x%x)\12\0"
	.text
	pea	_sched_Mutex
	pea	L390
	pea	0x1
	jbsr	_Sys_Panic
	lea	sp@(0xc),sp
L389:
	movl	#0x2,a5@(0xc)
	pea	a5@
	jbsr	_Sched_MoveInQueue
	addqw	#0x4,sp
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L391
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L392
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L392:
L391:
LE386:
	moveml	a6@(-0x4),#0x2000
	unlk	a6
	rts
	LF386 = 4
	LS386 = 0x2000
	LFF386 = 0
	LSS386 = 0x0
	LP386 =	0x14
	.data
	.text
|#PROC# 0
	.globl	_Sched_StartUserProc
_Sched_StartUserProc:
	link	a6,#0
	addl	#-LF393,sp
	moveml	#LS393,sp@
	addql	#0x1,_sync_Instrument+0x10
	clrl	_sched_Mutex
	tstl	_mach_AtInterruptLevel
	jne	L395
	movl	_mach_NumDisableIntrsPtr,a0
	movl	a0@,d0
	subql	#0x1,d0
	movl	d0,a0@
	jne	L396
|#ASMOUT#
movw #0x2000,sr
|#ENDASM#
L396:
L395:
	movl	_proc_RunningProcesses,a0
	movl	a0@,a5
	pea	a5@
	jbsr	_Proc_Lock
	addqw	#0x4,sp
	orl	#0x80,a5@(0x10)
	pea	a5@
	jbsr	_Proc_Unlock
	addqw	#0x4,sp
	movl	a6@(0x8),sp@-
	pea	a5@
	jbsr	_Mach_StartUserProc
	addqw	#0x8,sp
LE393:
	moveml	a6@(-0x4),#0x2000
	unlk	a6
	rts
	LF393 = 4
	LS393 = 0x2000
	LFF393 = 0
	LSS393 = 0x0
	LP393 =	0x10
	.data
