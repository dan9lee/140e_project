#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "fork.h"
#include "sched.h"
#include "mini_uart.h"
#include "utils.h"
#include "mm.h"
#include "mbox.h"
#include "peripherals/irq.h"
#include "peripherals/mbox.h"
#include "bootloader.h"

unsigned waitMaster=1;
int strncmp(const char* _s1, const char* _s2, unsigned n) {
	const unsigned char *s1 = (void*)_s1, *s2 = (void*)_s2;
	while(n--) {
    	if(*s1++!=*s2++)
    		return s1[-1] - s2[-1];
	}
	return 0;
}
static int readline(char *buf, int sz) {
	// printf("starting to read\r\n");
	for(int i = 0; i < sz; i++) {
		if((buf[i] = uart_recv()) == '\n') {
			buf[i] = 0;
			return i;
		}
		// printf("read a character\r\n");
		// uart_send(buf[i]);
	}
	// just return partial read?
	
	return -1;
}

void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
    unsigned int ra;
    ra=get32(GPFSEL1);
    ra&=~(7<<18);
    ra|=1<<18;
    put32(GPFSEL1,ra);

    	// while(c!='s') c=uart_recv();
	char buf[1024];
	int n;

	// for(unsigned i = 0; i < 20; i++) put_uint(0x12345678);

	while((n = readline(buf, sizeof(buf)))) {
        put32(GPSET0,1<<16);
        for(ra=0;ra<0x10000;ra++) burn();
        put32(GPCLR0,1<<16);
        for(ra=0;ra<0x10000;ra++) burn();
        // while(1) printf("hello!\n");

		if(strncmp(buf, "echo", 4) == 0) {
			printf("%s\n", buf);

        } else if(strncmp(buf, "get", 3) ==0) {
			unsigned core = buf[4]-'0';

			printf("Getting output from core %u\n", core);
		} else if(strncmp(buf, "send", 4) == 0) {
			unsigned core = buf[5]-'0';
			unsigned addr_lower = get_uint();
			unsigned addr_upper = get_uint();
			unsigned long addr = ((unsigned long) addr_upper << 32) + addr_lower;
			printf("core %u got addr %u %u\n", core, addr_upper, addr_lower);
			load((unsigned *)addr);
			waitMaster=0;
			delay(100000);
			mbox_sendA(core, 0, (unsigned*) addr);



		} else if(strncmp(buf, "stop", 4) == 0) {
			unsigned core = buf[5]-'0';
			mbox_send(core, 1, 999);

		} else {
	        printf("%s\n", buf);
		}		
	}



}



void kernel_child() {
	for(unsigned i = 0; i < 4; i++) {
		mbox_init(i);
	}
	irq_vector_init();
	enable_irq();
	// unsigned cpu = getCore();
	hang();
	// while(waitMaster) {;}
	// delay(cpu* 10000);
	// printf("CPU %u awake\r\n", cpu);
	// while(1) {;}
	// if(cpu == 1){

	// 	while(waitMaster){;}
	// 	BRANCHTO(0x1000000);
	// }
	// hang();
}
