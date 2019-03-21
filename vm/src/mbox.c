#include "utils.h"
#include "mbox.h"
#include "peripherals/mbox.h"
#include "printf.h"
// unsigned IRQCNTLS[] = {CORE0_MBOX_IRQCNTL, CORE1_MBOX_IRQCNTL, CORE2_MBOX_IRQCNTL, CORE3_MBOX_IRQCNTL};



unsigned mbox_init(unsigned mbox){

	unsigned cpu = getCore();
	unsigned irqcntlAddr = CORE0_MBOX_IRQCNTL + 4*cpu;
	unsigned val = get32(irqcntlAddr);
	val |= (1 << mbox);
	put32(irqcntlAddr, val);
	return 0;
}

unsigned mbox_send(unsigned cpu, unsigned mbox, unsigned msg){

	

	unsigned core_x_mbox_x_set_addr = CORE0_MBOX0_SET + 4*4*cpu + 4*mbox;
	put32(core_x_mbox_x_set_addr, msg);

	return 0;
}

unsigned mbox_sendA(unsigned cpu, unsigned mbox, unsigned* msg){

	unsigned core_x_mbox_x_set_addr = CORE0_MBOX0_SET + 4*4*cpu + 4*mbox;
	putA32(core_x_mbox_x_set_addr, msg);

	return 0;
}

unsigned mbox_read(unsigned mbox){

	unsigned cpu = getCore();
	unsigned core_x_mbox_x_rdclr_addr = CORE0_MBOX0_RDCLR + 4*4*cpu + 4*mbox;
	unsigned toReturn = get32(core_x_mbox_x_rdclr_addr);
	put32(core_x_mbox_x_rdclr_addr, toReturn);
	return toReturn;

}

unsigned* mbox_readA(unsigned mbox){

	unsigned cpu = getCore();
	unsigned core_x_mbox_x_rdclr_addr = CORE0_MBOX0_RDCLR + 4*4*cpu + 4*mbox;
	unsigned* toReturn = getA32(core_x_mbox_x_rdclr_addr);
	putA32(core_x_mbox_x_rdclr_addr, toReturn);
	return toReturn;

}

void handle_mbox_irq(unsigned mbox) {
	unsigned cpu = getCore();
	unsigned* msg = mbox_readA(mbox);
	printf("Core %u mbox %u got %u\r\n", cpu, mbox, *msg);
}