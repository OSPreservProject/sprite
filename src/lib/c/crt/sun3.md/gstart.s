|*
|* gstart.s --
|*
|*      Program entry point for profiled programs.  This is the
|*      default entry point for programs compiled with the `-pg'
|*      option to the C compiler.
|*
|* Copyright 1966, 1988 Regents of the University of California
|* Permission to use, copy, modify, and distribute this
|* software and its documentation for any purpose and without
|* fee is hereby granted, provided that the above copyright
|* notice appear in all copies.  The University of California
|* makes no representations about the suitability of this
|* software for any purpose.  It is provided "as is" without
|* express or implied warranty.
|*

    .data
    .asciz "$Header$"
    .even

    .text
    .long	0
    trap	#15		| Processes starting off in debug mode will
				|     start here.
    .globl	gstart
gstart:
    movl	sp@,d2		| argc
    lea		sp@(4),a3	| argv
    movl	d2, d0		| pass argc + 1 as one param 
    addql	#1, d0          |  	to lmult
    movl	#4, d1		| want 4 * (argc + 1)
    jsr  	lmult		| 
    movl        a3, d1		| take argv and ... 
    addl        d0, d1		| ... go past it by (argc + 1) 4-byte fields
    movl	d1,_environ	| set the global _environ variable
    movl	d1,sp@-		| push envp
    movl	a3,sp@-		| push argv
    movl	d2,sp@-		| push argc
    lea		0,a6		| stack frame link 0 in main -- for dbx
|-------
    pea         _etext
    pea         0x2000          | Start of text segment
    jsr         _monstartup     | monstartup(0x2000, etext)
    addql       #8,sp
    pea         __mcleanup
    jsr         _atexit         | atexit(_mcleanup)
    addql       #4,sp
    clrl        _errno          | errno = 0
|-------
    jsr		_main		| main(argc, argv, envp)
    addw	#12,sp
    movl	d0,sp@-
    jsr		_exit		| exit( ... )
    addql	#4,sp
    rts
