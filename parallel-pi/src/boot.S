#include "arm/mmu.h"
#include "arm/sysregs.h"
#include "mm.h"
#include "peripherals/base.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1  // multiprocessor affinity register (d10.2.86)
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b child

proc_hang:
	b proc_hang

master:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED  // little endian, i/d-cache disabled, mmu disabled
	msr	sctlr_el1, x0   // g7.2.120

	ldr	x0, =HCR_VALUE // hypervisor configuration, el1 uses 64-bit
	msr	hcr_el2, x0  //d10.2.45

	ldr	x0, =SCR_VALUE  // security - el2 uses 64-bit
	msr	scr_el3, x0  //d10.2.99

	ldr	x0, =SPSR_VALUE  // processor state --> el1 after eret. mask interrupts
	msr	spsr_el3, x0    //c5.2.20

	adr	x0, el1_entry  // address we return to after eret
	msr	elr_el3, x0  //c5.2.7

	eret

el1_entry:
	adr	x0, bss_begin  // memzero the bss
	adr	x1, bss_end		// could have done in c, but sample code did here
	sub	x1, x1, x0
	bl 	memzero

	bl 	__create_page_tables  // code that creates kernel identity page tables

	mov	x0, #VA_START
	add	sp, x0, #STACK_CORE0  //defined in mm.h

	adrp	x0, pg_dir // defined in linker script; what sample code did for Pi 3
	msr	ttbr1_el1, x0 // D10.2.114; register for high asid (kernel) mapping

	ldr	x0, =(TCR_VALUE) // user and kernel pages should be 4kb
	msr	tcr_el1, x0 // D10.2.103

	ldr	x0, =(MAIR_VALUE) // mair used for shared page attributes; no cache related
	msr	mair_el1, x0 //D10.2.82


	ldr	x2, =kernel_main

	mov	x0, #SCTLR_MMU_ENABLED //enable MMU
	msr	sctlr_el1, x0

	br 	x2

child:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED //same as in master probably could consilidate
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
	adr	x0, bss_begin // child is going to set up bss and kernel page tables
	adr	x1, bss_end // avoids race condition at start of program and since
	sub	x1, x1, x0 // setting up tables is idempotent, we know we are safe
	bl 	memzero

	bl __create_page_tables // sets up page tables

	mrs	x0, mpidr_el1 // gets core number and sets up stack by offset
	and	x0, x0,#0xFF
	mov x1, #STACK_SIZE
	mul x0, x0, x1
	add x0, x0, #STACK_CORE0
	mov x1, #VA_START
	add sp, x0, x1

	adrp	x0, pg_dir //sets page table
	msr	ttbr1_el1, x0

	ldr	x0, =(TCR_VALUE) //same as in master
	msr	tcr_el1, x0

	ldr	x0, =(MAIR_VALUE)
	msr	mair_el1, x0

	ldr	x2, =kernel_child

	mov	x0, #SCTLR_MMU_ENABLED
	msr	sctlr_el1, x0

	br 	x2

	.macro	create_pgd_entry, tbl, virt, tmp1, tmp2
	create_table_entry \tbl, \virt, PGD_SHIFT, \tmp1, \tmp2
	create_table_entry \tbl, \virt, PUD_SHIFT, \tmp1, \tmp2
	.endm

	.macro	create_table_entry, tbl, virt, shift, tmp1, tmp2
	lsr	\tmp1, \virt, #\shift
	and	\tmp1, \tmp1, #PTRS_PER_TABLE - 1			// table index
	add	\tmp2, \tbl, #PAGE_SIZE // next page is always adjacent
	orr	\tmp2, \tmp2, #MM_TYPE_PAGE_TABLE
	str	\tmp2, [\tbl, \tmp1, lsl #3]
	add	\tbl, \tbl, #PAGE_SIZE					// next level table page
	.endm

	.macro	create_block_map, tbl, phys, start, end, flags, tmp1
	lsr	\start, \start, #SECTION_SHIFT
	and	\start, \start, #PTRS_PER_TABLE - 1			// table index
	lsr	\end, \end, #SECTION_SHIFT
	and	\end, \end, #PTRS_PER_TABLE - 1				// table end index
	lsr	\phys, \phys, #SECTION_SHIFT
	mov	\tmp1, #\flags
	orr	\phys, \tmp1, \phys, lsl #SECTION_SHIFT			// table entry
9999:	str	\phys, [\tbl, \start, lsl #3]				// store the entry
	add	\start, \start, #1					// next entry
	add	\phys, \phys, #SECTION_SIZE				// next block
	cmp	\start, \end
	b.ls	9999b
	.endm

__create_page_tables:
	mov	x29, x30						// save return address

	adrp	x0, pg_dir
	mov	x1, #PG_DIR_SIZE
	bl 	memzero

	adrp	x0, pg_dir
	mov	x1, #VA_START
	create_pgd_entry x0, x1, x2, x3

	/* Mapping kernel and init stack*/
	mov 	x1, xzr							// start mapping from physical offset 0
	mov 	x2, #VA_START						// first virtual address
	ldr	x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)		// last virtual address
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

	/* Mapping device memory*/
	mov 	x1, #DEVICE_BASE					// start mapping from device base address
	ldr 	x2, =(VA_START + DEVICE_BASE)				// first virtual address
	ldr	x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)	// last virtual address
	create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

	mov	x30, x29						// restore return address
	ret
