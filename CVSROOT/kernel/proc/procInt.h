/*
 * procInt.h --
 *
 *	Declarations internal to the proc module.
 *
 * Copyright (C) 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $ProcInt: proto.h,v 1.4 86/03/20 14:00:21 andrew Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCINT
#define _PROCINT

/*
 * Information used by the Proc_Wait command for child termination information.
 */

typedef struct {
    Proc_PID 			processID;
    int 			termReason;
    int 			termStatus;
    int 			termCode;
    int				numQuantumEnds;
    int				numWaitEvents;
    Timer_Ticks			kernelCpuUsage;
    Timer_Ticks			userCpuUsage;
    Timer_Ticks			childKernelCpuUsage;
    Timer_Ticks			childUserCpuUsage;
} ProcChildInfo;


/*
 * Procedures internal to the proc module.
 */

extern void ProcRemoteExit();
extern void ProcExitProcess();
extern	void			ProcExitInit();

extern	void			ProcFamilyHashInit();
extern	void			ProcEnvironInit();

extern	void			ProcDebugWakeup();
extern	void			ProcFreePCB();
extern	Proc_ControlBlock	*ProcGetUnusedPCB();
extern	void			ProcFamilyRemove();
extern	ReturnStatus		ProcFamilyInsert();

extern	int			ProcTableMatch();
extern	void			ProcAddToGroupList();
#endif _PROCINT
#endif /* _PROCINT */
