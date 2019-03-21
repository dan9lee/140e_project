.section ".text.boot"

// Very simple boot code for user programs
// just branch to main and don't touch r0-r3 which contain parameters
.globl _start
_start:
	mov x20, x30
	bl kernel_main
	mov x30, x20
	ret
