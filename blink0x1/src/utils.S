.globl get_el
get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

.globl put32
put32:
	str w1,[x0]
	ret

.globl get32
get32:
	ldr w0,[x0]
	ret

.globl putA32
putA32:
	str w1,[x0]
	ret

.globl getA32
getA32:
	ldr w0,[x0]
	ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

.globl getCore
getCore:
	mrs	x0, mpidr_el1
	and	x0, x0,#0xFF
	ret

.globl hang
hang: 
	b 	hang

.globl dmb
dmb:
	dmb sy
	ret

.globl dsb
dsb:
	dsb sy
	ret

.globl isb
isb:
	isb
	ret
/*
.globl write_fence
write_fence:
	ldar r0
	ret
.globl read_fence
read_fence:
	stlr r0
	ret
*/

.globl burn
burn:
	add x1, x1, #0
	ret

