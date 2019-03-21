#include "utils.h"
#include "mbox.h"
#include "peripherals/mbox.h"
#include "printf.h"
#include "irq.h"
// unsigned IRQCNTLS[] = {CORE0_MBOX_IRQCNTL, CORE1_MBOX_IRQCNTL, CORE2_MBOX_IRQCNTL, CORE3_MBOX_IRQCNTL};



unsigned mbox_init(unsigned mbox){

	unsigned cpu = getCore();
	unsigned irqcntlAddr = CORE0_MBOX_IRQCNTL + 4*cpu;		//QA7 pg 14
	unsigned val = get32(irqcntlAddr);
	val |= (1 << mbox);
	put32(irqcntlAddr, val);
	return 0;
}

unsigned mbox_send(unsigned cpu, unsigned mbox, unsigned msg){

	

	unsigned core_x_mbox_x_set_addr = CORE0_MBOX0_SET + 4*4*cpu + 4*mbox; //QA7 pg15
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
	unsigned core_x_mbox_x_rdclr_addr = CORE0_MBOX0_RDCLR + 4*4*cpu + 4*mbox; //QA7 pg 15
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

//this is the main functionality for the slaves
void handle_mbox_irq(unsigned mbox) {
	unsigned cpu = getCore();
	if(mbox == 0) {			//we designate mbox 0 to receive program addresses
		unsigned* program = mbox_readA(mbox);
		printf("Core %u mbox %u going to %x\r\n", cpu, mbox, (unsigned long) program);
		enable_irq();		//enable interrupts so master still communicates during execution
		BRANCHTO(program);	//jump to the program address
		disable_irq();
	} else if(mbox==1) {	//mbox 1 designated to receive stop messages
		unsigned msg = mbox_read(mbox);
		printf("Core %u mbox %u got %u. Stopping\r\n", cpu, mbox, msg);

		enable_irq();		//enable interrupts so master continues communication
		hang();				//hang so we stop the execution
		disable_irq();

	}
}