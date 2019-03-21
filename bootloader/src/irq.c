#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"
#include "peripherals/mbox.h"
#include "mbox.h"
const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void enable_interrupt_controller()
{
	put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
	printf("%s, ESR: %x, address: %x\r\n", entry_error_messages[type], esr, address);
}

void handle_irq(void)
{
	unsigned cpuId = getCore();

	unsigned irqSourceAddr = CORE0_IRQ_SOURCE + 4*cpuId;	//QA7, pg16

	unsigned src = get32(irqSourceAddr);
	unsigned val = (src >> 4) & (0b1111);		//QA7 pg16 get mailbox number
	unsigned mboxNum = 0;

	while(!(val & 1 << mboxNum)) {
		mboxNum++;
		if(mboxNum > 3) {
			mboxNum = -1;
			break;
		}
	}

	if(val) {
		handle_mbox_irq(mboxNum);	//handle mailbox interrupt for offending mailbox
	} 
}
