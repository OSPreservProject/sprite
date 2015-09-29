|
|   fp_globals.s
|

        .data
        .globl _Fmode
	.globl _Fstatus
	.globl __skybase
	.globl _fp_state_mc68881
	.globl _fp_state_skyffp
	.globl _fp_state_software
	.globl _fp_state_sunfpa
	.globl _fp_switch

_fp_switch:
	.long 0x00000000

_fp_state_software:
	.long 0x00000000

_fp_state_skyffp:
	.long 0x00000000

_fp_state_mc68881:
	.long 0x00000000

_fp_state_sunfpa:
	.long 0x00000000

__skybase:
	.long 0x00000000

_Fmode:
	.long 0x00000080

_Fstatus:
	.long 0x00000000

