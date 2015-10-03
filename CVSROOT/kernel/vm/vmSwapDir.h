/*
 * vmSwapDir.h --
 *
 *	Definitions for the swap directory monitor.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMSWAPDIR
#define _VMSWAPDIR

/*
 * The name of the swap directory.
 */
#define	VM_SWAP_DIR_NAME	"/swap/"

extern void Vm_OpenSwapDirectory _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
extern void VmReopenSwapDirectory _ARGS_((void));
extern Fs_Stream *VmGetSwapStreamPtr _ARGS_((void));
extern void VmDoneWithSwapStreamPtr _ARGS_((void));
extern Boolean VmSwapStreamOk _ARGS_((void));

#endif /* _VMSWAPDIR */
