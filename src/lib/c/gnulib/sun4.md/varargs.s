
.global ___builtin_saveregs
___builtin_saveregs:
	st %i0,[%fp+68]
	st %i1,[%fp+72]
	st %i2,[%fp+76]
	st %i3,[%fp+80]
	st %i4,[%fp+84]
	retl
	st %i5,[%fp+88]

