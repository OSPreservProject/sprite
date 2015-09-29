/*
 * sys/ptrace.h
 *
 *	Constants for ptrace system call.  Ptrace is not currently
 *      implemented on Sprite.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.5 90/01/12 12:03:25 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _SYS_PTRACE_H
#define _SYS_PTRACE_H

enum ptracereq {
        PTRACE_TRACEME = 0,
        PTRACE_CHILDDONE = 0,
        PTRACE_PEEKTEXT = 1,
        PTRACE_PEEKDATA = 2,
        PTRACE_PEEKUSER = 3,
        PTRACE_POKETEXT = 4,
        PTRACE_POKEDATA = 5,
        PTRACE_POKEUSER = 6,
        PTRACE_CONT = 7,
        PTRACE_KILL = 8,
        PTRACE_SINGLESTEP = 9,
        PTRACE_ATTACH = 10,
        PTRACE_DETACH = 11,
        PTRACE_GETREGS = 12,
        PTRACE_SETREGS = 13,
        PTRACE_GETFPREGS = 14,
        PTRACE_SETFPREGS = 15,
        PTRACE_READDATA = 16,
        PTRACE_WRITEDATA = 17,
        PTRACE_READTEXT = 18,
        PTRACE_WRITETEXT = 19,
        PTRACE_GETFPAREGS = 20,
        PTRACE_SETFPAREGS = 21,
        PTRACE_GETWINDOW = 22,
        PTRACE_SETWINDOW = 23,
        PTRACE_22 = 22,
        PTRACE_23 = 23,
        PTRACE_SYSCALL = 24,
        PTRACE_DUMPCORE = 25,
        PTRACE_SETWRBKPT = 26,
        PTRACE_SETACBKPT = 27,
        PTRACE_CLRDR7 = 28,
        PTRACE_26 = 26,
        PTRACE_27 = 27,
        PTRACE_28 = 28,
        PTRACE_GETUCODE = 29
};

#endif /* SYS_PTRACE_H */

