#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "fork.h"
#include "sched.h"
#include "mini_uart.h"
#include "sys.h"
#include "user.h"
#include "bootloader.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "mm.h"
// void kernel_process(){
// 	printf("Kernel process started. EL %d\r\n", get_el());
// 	unsigned long begin = (unsigned long) VA_START + 0x1000000;
// 	unsigned long end = (unsigned long) VA_START + 0x1000000 + 13148/4;
// 	unsigned long process = (unsigned long) begin;
// 	printf("About to move to user mode\r\n");
// 	int err = move_to_user_mode(begin, end - begin, process - begin);
// 	printf("Finished moving to user mode\r\n");
// 	if (err < 0){
// 		printf("Error while moving process to user mode\n\r");
// 	} 
// }


// void kernel_main()
// {
// 	uart_init();
// 	init_printf(NULL, putc);
// 	unsigned c = 0;
// 	while(c != 's') {
// 		c = uart_recv();
// 		uart_send(c);
// 	}
	
// 	irq_vector_init();
// 	timer_init();
// 	enable_interrupt_controller();
// 	enable_irq();

// 	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0);
// 	if (res < 0) {
// 		printf("error while starting kernel process");
// 		return;
// 	}

// 	while (1){
// 		schedule();
// 	}	
// }



// unsigned waitMaster=1;

void kernel_main(void)
{
	uart_init();


    unsigned int ra;
    ra=get32(GPFSEL1);
    ra&=~(7<<18);
    ra|=1<<18;
    put32(GPFSEL1,ra);
	delay_ms(1000);
    for(unsigned i=0; i < 10; i++)
    {
        put32(GPSET0,1<<16);
        for(ra=0;ra<0x10000;ra++) burn();
        put32(GPCLR0,1<<16);
        for(ra=0;ra<0x10000;ra++) burn();
    }
	unsigned rcv = get_uint();

	while(rcv != 0x12345678) {
		// delay_ms(1000);
		// put_uint(rcv);
		rcv = get_uint();
	}
	// if(rcv != ) die(rcv);
	unsigned sz = get_uint();

	unsigned cksum = get_uint();

	// check if the binary is too large
	// if(sz > (unsigned)BRANCHTO - ARMBASE){
	// 	die(NAK);
	// }

	// echo back SOH, cksum(sz), cksum
	put_uint(SOH);
	put_uint(crc32(&sz, sizeof(sz)));
	put_uint(cksum);

	// wait for ACK
	if(get_uint() != ACK) {
		die(NAK);
	}

	// read the bytes and copy them to ARMBASE one at a time

	unsigned long va = VA_START + 0x1000000;
// void map_page(struct task_struct *task, unsigned long va, unsigned long page){
	// map_page( , 0x1000000, get_free_page());
 //    for(unsigned i=0; i < 10; i++)
 //    {
 //        put32(GPSET0,1<<16);
 //        for(ra=0;ra<0x10000;ra++) burn();
 //        put32(GPCLR0,1<<16);
 //        for(ra=0;ra<0x10000;ra++) burn();
 //    }
	// int new_table;
	// unsigned long pud = map_table((unsigned long *)(pg_dir), PGD_SHIFT, va, &new_table);

	// unsigned long pmd = map_table((unsigned long *)(pud + VA_START) , PUD_SHIFT, va, &new_table);
	// unsigned long pte = map_table((unsigned long *)(pmd + VA_START), PMD_SHIFT, va, &new_table);
	// map_table_entry((unsigned long *)(pte + VA_START), va, get_free_page());

	unsigned * program = (unsigned *) va;
	unsigned int i;


	for(i = 0; i < sz/sizeof(unsigned); i++) {


		program[i] = get_uint();

	}

	// check for EOT
	if(get_uint() != EOT) die(NAK);

	// verify that the program was correct
	if(cksum != crc32(program, sz)) die(NAK);

	// acknowledge
	put_uint(ACK);

	delay_ms(500);

	init_printf(0, putc);

	printf("Program loaded!\r\n");
	printf("Going to %x %x\n", ((unsigned long) program) >> 32, program);
    for(unsigned i=0; i < 10; i++)
    {
        put32(GPSET0,1<<16);
        for(ra=0;ra<0x10000;ra++) burn();
        put32(GPCLR0,1<<16);
        for(ra=0;ra<0x10000;ra++) burn();
    }	
	// run what client sent.
    BRANCHTO((unsigned long) program);
	// int counter = 1;
 //  while(1){
	// 	printf("Counter is: %d\r\n", counter);
	// 	for(ra=0;ra<0x100000;ra++) burn();
	// 	counter++;
	// }
	// should not get back here, but just in case.
	// rpi_reboot();
	// irq_vector_init();
	// timer_init();
	// enable_interrupt_controller();
	// enable_irq();

	// int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0);
	// if (res < 0) {
	// 	printf("error while starting kernel process");
	// 	return;
	// }

	// while (1){
	// 	schedule();
	// }	
// }

}