        .seg    "text"                  ! [internal]
        .proc   16
        .global ___builtin_delete
___builtin_delete:
!#PROLOGUE# 0
!#PROLOGUE# 1
        save    %sp,-96,%sp
        tst     %i0
        be      L77004
        nop
        call    _free,1
        mov     %i0,%o0
L77004:
        ret
        restore
        .proc   16
        .global ___builtin_vec_delete
___builtin_vec_delete:
!#PROLOGUE# 0
!#PROLOGUE# 1
        save    %sp,-96,%sp
        add     %i1,1,%l7
        mov     %l7,%o0
        call    .mul,2
        mov     %i2,%o1
        add     %i0,%o0,%i1
        mov     %i0,%l6
        mov     0,%i0
        cmp     %i0,%l7
        bge,a   LY3
        tst     %i4
        sub     %i1,%i2,%i1
LY2:                                    ! [internal]
        mov     %i1,%o0
        mov     %i5,%o1
        mov     %i3,%g1
        call    %g1,2
        inc     %i0
        cmp     %i0,%l7
        bl,a    LY2
        sub     %i1,%i2,%i1
        tst     %i4
LY3:                                    ! [internal]
        be      L77012
        nop
        call    _free,1
        mov     %l6,%o0
L77012:
        ret
        restore
        .seg    "data"                  ! [internal]
