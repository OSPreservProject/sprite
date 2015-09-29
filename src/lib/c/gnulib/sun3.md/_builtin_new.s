        .data
        .text
LL0:
|#PROC# 022

        LF13    =       4
        LS13    =       0
        LFF13   =       4
        LSS13   =       0
        LP13    =       12
        .data
        .text
        .globl  ___builtin_new
___builtin_new:
|#PROLOGUE# 0

        link    a6,#-4
|#PROLOGUE# 1

        movl    a6@(8),sp@-
        jbsr    _malloc
        addqw   #4,sp
        movl    d0,a6@(-4)
        jne     L16
        movl    ___new_handler,a0
        jsr     a0@
L16:
        movl    a6@(-4),d0
        unlk    a6
        rts
