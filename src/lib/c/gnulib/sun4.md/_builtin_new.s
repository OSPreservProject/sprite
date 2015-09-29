        .seg    "text"                  ! [internal]
        .proc   66
        .global ___builtin_new
___builtin_new:
!#PROLOGUE# 0
!#PROLOGUE# 1
        save    %sp,-96,%sp
        call    _malloc,1
        mov     %i0,%o0
        mov     %o0,%i0
        tst     %i0
        bne     L77004
        sethi   %hi(___new_handler),%g1
        ld      [%g1+%lo(___new_handler)],%g1
        call    %g1,0
        nop
L77004:
        ret
        restore
        .seg    "data"                  ! [internal]
