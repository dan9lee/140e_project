.globl irq_vector_init
irq_vector_init:
	adr	x0, vectors		// load VBAR_EL1 with virtual
	msr	vbar_el1, x0		// vector table address (d10-2745)
	ret

.globl enable_irq
enable_irq:
	msr    daifclr, #2 	// set "i" bit for interrupts (B1-80)
	ret

.globl disable_irq
disable_irq:
	msr	daifset, #2		// clear "i" bit for interrupts
	ret
