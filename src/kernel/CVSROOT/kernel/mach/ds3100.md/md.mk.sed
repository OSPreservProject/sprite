#
# This file is used automatically by mkmf to massage md.mk, in order
# to guarantee that bootSysAsm.o is the first object file in the list.
# It must eventually be the first file in the linked kernel.
#
/^OBJS/s|= \(.*\) \([^/ ]*\)/loMem.o|= \2/loMem.o \1|
