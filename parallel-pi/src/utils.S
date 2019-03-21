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

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

/* Some code we planed on using for virtual memory */
.globl set_pgd
set_pgd:
	msr	ttbr0_el1, x0
	tlbi vmalle1is
  	DSB ISH              // ensure completion of TLB invalidation
	isb
	ret

.globl get_pgd
get_pgd:
	mov x1, 0
	ldr x0, [x1]
	mov x0, 0x1000
	msr	ttbr0_el1, x0
	ldr x0, [x1]
	ret

/* simple helper to get the core number */
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

/* NOTE: Calling convention changed. Pass user program parameters through x0-x3 */
.globl BRANCHTO
BRANCHTO:
	br x4

.globl burn
burn:
	add x1, x1, #0
	ret
