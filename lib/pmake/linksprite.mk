#
# Rules for linking Sprite programs.
#
# LINKSPRITE is for programs being linked to run under Sprite
# LINKCOMPAT is for programs being linked to run under UNIX.
#
# For both of these, the program should have all of its object files
# and libraries for sources.
#
# To use, do something like this:
#
# fish : fish.o -lm LINKSPRITE
#
# fish.o will be compiled (with -I/sprite/lib/include and -m68010 in effect
# by default) and then linked with the math library and Sprite C library to
# form 'fish'.
#
# Header sources may be given as 'fs.h', 'dev/pdev.h', etc.
#
#ifndef LDFLAGS
LDFLAGS	=
#endif LDFLAGS
CFLAGS += -I/sprite/lib/include -I/sprite/att/lib/include -m68010

.PATH.h:	/sprite/lib/include /sprite/att/lib/include
.PATH.a:	/sprite/lib /sprite/att/lib
.PATH.ln:	/sprite/lib/lint

LINKSPRITE:	.USE -lc
	ld -e start -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

LINKCOMPAT:	.USE /sprite2/users/sprite/compat/libcompat.a
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)  /lib/libc.a
