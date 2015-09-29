
|
|	@(#)db.s 1.1 86/09/27
|	Copyright (c) 1986 by Sun Microsystems, Inc.
|
|	these routines split and join loop counts for DBcc instruction
|	usage in the memory routines:
|	they also allocate _obs_value, and _exp_value for the memory
|	routines.
|
|	[b|w|l]db_pack
|		does the packing necessary for the loop count -> return
|		left from a DBcc instruction. does the +=1 required.
|		d1(16MSB) d0(16LSB) -> d0
|	[b|w|l]db_unpack
|		does the stuff to go from count -> loop count for
|		DBcc madness. Does shifting and -=1 required.
|		d0 -> d1(16MSB) d0(16LSB)
|
	.text

	.globl	bdb_unpack, wdb_unpack, ldb_unpack
	.globl	bdb_pack, wdb_pack, ldb_pack
	.globl	_obs_value, _exp_value

ldb_unpack:
        lsrl    #1, d0
wdb_unpack:
        lsrl    #1, d0
bdb_unpack:
        subql   #1, d0          | adjust for dbcc brain mode
        movl    d0, d1          | get bits over to d1
        clrw    d1              | clear what will be the 16 MSB
        swap    d1              | move them
        andl    #0xffff, d0     | clear 16 MSB of d0 too.
        rts                     | we done
|
|       these pack a bc count into d0.  There are three entry points
|       just in case we do something funny in the future. (and to be
|       consistent)
|
ldb_pack:
wdb_pack:
bdb_pack:
        swap    d1              | move MSB to MSB
        movw    d0, d1          | move LSB over to d1
        movl    d1, d0          | return back to d0 for return
        addql   #1, d0          | bump up for dbcc damage in head
        rts                     | give it back to them

|	.asciz	"@(#)db.s 1.5 1/12/85 Copyright Sun Micro"
|	.even
	.data
_obs_value:
	.long	0
_exp_value:
	.long	0
