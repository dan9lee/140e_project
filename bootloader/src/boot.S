#include "arm/sysregs.h"

#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		// multiprocessor affinity register (d10.2.86)
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master			// cpu0 branches to master
	b child 				// all other cores branch to child

proc_hang: 
	b 	proc_hang


// note documentation page numbers in header file
master:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED // little endian, i/d-cache disabled, mmu disabled
	msr	sctlr_el1, x0		// g7.2.120

	ldr	x0, =HCR_VALUE // hypervisor configuration, el1 uses 64-bit
	msr	hcr_el2, x0			//d10.2.45

	ldr	x0, =SCR_VALUE // security - el2 uses 64-bit
	msr	scr_el3, x0			//d10.2.99

	ldr	x0, =SPSR_VALUE // processor state --> el1 after eret. mask interrupts
	msr	spsr_el3, x0		//c5.2.20

	adr	x0, el1_entry	// address we return to after eret
	msr	elr_el3, x0			//c5.2.7

	eret				

el1_entry:
	adr	x0, bss_begin	//memzero the bss
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	mov	sp, #STACK_CORE0 //defined in mm.h
	bl	kernel_main
	b 	proc_hang		// should never come here

child:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0		

	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0

	ldr	x0, =SPSR_VALUE
	msr	spsr_el3, x0

	adr	x0, el1_entry_child		
	msr	elr_el3, x0

	eret	

el1_entry_child:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF
	mov x1, #STACK_SIZE		
	mul x0, x0, x1
	add x0, x0, #STACK_CORE0	//set each core's stack pointer based on mm.h
	mov	sp, x0
	bl	kernel_child
	b 	proc_hang		// should never come here
