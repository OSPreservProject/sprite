/*
 * netDFMachInt.h --
 *
 *	Internal definitions for the ds5000-based FDDI controller
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$
 */

#ifndef _NETDFMACHINT
#define _NETDFMACHINT

#include <netInt.h>
#include <mach.h>

#define NET_DF_MACH_OPTION_ROM_OFFSET        0x218000

/*
 * Addresses for control registers as offsets from option slot address
 * (page 5)
 */

#define NET_DF_MACH_RESET_OFFSET             0x100200
#define NET_DF_MACH_CTRLA_OFFSET             0x100500
#define NET_DF_MACH_CTRLB_OFFSET             0x100502
#define NET_DF_MACH_INT_EVENT_OFFSET         0x100400
#define NET_DF_MACH_INT_MASK_OFFSET          0x100404
#define NET_DF_MACH_STATUS_OFFSET            0x100402

#define NET_DF_MACH_COMMAND_RING_OFFSET      0x200400
#define NET_DF_MACH_COMMAND_BUF_OFFSET       0x208000
#define NET_DF_MACH_COMMAND_BUF_END_OFFSET   0x20FFFF

#define NET_DF_MACH_UNSOL_RING_OFFSET        0x200800
#define NET_DF_MACH_NUM_UNSOL_DESC           0x000064

#define NET_DF_MACH_RMC_XMT_BUF_OFFSET       0x240000

#define NET_DF_MACH_ERROR_LOG_OFFSET         0x200000

#define NET_DF_MACH_ERR_INTERNAL_OFFSET      0x000000
#define NET_DF_MACH_ERR_EXTERNAL_OFFSET      0x000004
#define NET_DF_MACH_ERR_EVENT_OFFSET         0x000050
#define NET_DF_MACH_ERR_CTRLA_OFFSET         0x000052
#define NET_DF_MACH_ERR_CTRLB_OFFSET         0x000056
#define NET_DF_MACH_ERR_STATUS_OFFSET        0x000058

#endif

