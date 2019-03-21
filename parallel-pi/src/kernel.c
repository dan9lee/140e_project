#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "mini_uart.h"
#include "user.h"
#include "mm.h"
#include "bootloader.h"

#define BUFFER_SIZE 4096

// Global variables for coordinating
// Since only one core will access at a time, should be
// OK... but would have been nice to have a semaphore working
unsigned shouldRun[4] = {0, 0, 0, 0};
unsigned running[4] = {0, 0, 0, 0};
int computeBuffer[BUFFER_SIZE];
unsigned numCores = 1;

// clears the compute buffer in between iterations
void clear_buffer(){
	for (int j=0; j< BUFFER_SIZE; j++){
		computeBuffer[j] = 0;
	}
}

/* Function called by parent core. Look in boot.S */
void kernel_main()
{
	uart_init();
	load_program();

	// run what client sent.
	init_printf(0, putc);
	printf("The program loaded!\r\n");

	// ONE CORE ---------------
	printf("Running on one core\r\n");
	running[1] = 1; // signal other cores via memory
	shouldRun[1] = 1;
	unsigned start = timer_get_time();
	while(running[1]){;} // busy wait
	unsigned finished = timer_get_time();
	printf("Took total time %d\r\n", finished-start);
	for(int i=0; i<10; i++){
		printf("%d ", computeBuffer[i]);
	}
	printf("\n");
	clear_buffer();

	// TWO CORES --------------------
	numCores = 2;
	printf("Running on two cores\r\n");
	running[1] = 1;
	running[2] = 1;
	shouldRun[1] = 1;
	shouldRun[2] = 1;
	start = timer_get_time();
	while(running[1] && running[2]){;}
	finished = timer_get_time();
	printf("Took total time %d\r\n", finished-start);
	for(int i=0; i<10; i++){
		printf("%d ", computeBuffer[i]);
	}
	printf("\n");
	clear_buffer();

	// THREE CORES --------------------
	numCores = 3;
	printf("Running on three cores\r\n");
	running[1] = 1;
	running[2] = 1;
	running[3] = 1;
	shouldRun[1] = 1;
	shouldRun[2] = 1;
	shouldRun[3] = 1;
	start = timer_get_time();
	while(running[1] && running[2] && running[3]){;}
	finished = timer_get_time();
	printf("Took total time %d\r\n", finished-start);
	for(int i=0; i<10; i++){
		printf("%d ", computeBuffer[i]);
	}
	printf("\n");
	clean_reboot();
}

/* Function called by children cores. Look in boot.S */
void kernel_child(void)
{
	unsigned cpu = getCore();

	while(1){
		while(!shouldRun[cpu]){;}
		shouldRun[cpu] = 0;
		// Updated BRANCHTO defined in utils.S
		BRANCHTO(computeBuffer, 4096, cpu-1, numCores, PROGRAM_ADDRESS);
		running[cpu] = 0;
	}
}
