/*
 * vmMachInt.h
 *
 *     	Internal machine dependent virtual memory data structures and procedure 
 *	headers.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMMACHINT
#define _VMMACHINT

typedef unsigned int	VmMachPTE;	

extern	Address		vmMachPTESegAddr;
extern	Address		vmMachPMEGSegAddr;

/*
 * Assembly language routines in vmSunAsm.s.
 */
extern	int		VmMachCopyEnd();
extern	VmMachPTE	VmMachGetPageMap();
extern	void		VmMachSetPageMap();
extern	int		VmMachGetSegMap();
extern	void		VmMachSetSegMap();
extern	VmMachPTE	VmMachReadPTE();
extern	void		VmMachWritePTE();
extern	int		VmMachGetUserContext();
extern	int		VmMachGetKernelContext();
extern	int		VmMachGetContextReg();
extern	void		VmMachSetUserContext();
extern	void		VmMachSetKernelContext();
extern	void		VmMachSetContextReg();

#endif _VMMACHINT
