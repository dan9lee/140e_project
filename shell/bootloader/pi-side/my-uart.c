#include "rpi.h"
#include "gpio.h"

#define AUX_ENABLES 0x20215004
#define BAUD_VAL 270

static struct uart_periph { // order of registers and base address on page 8
	unsigned IO,
					 IER,
					 IIR,
					 LCR,
					 MCR,
					 LSR,
					 MSR,
					 SCRATCH,
					 CNTL,
					 STAT,
					 BAUD;

} *uart_mmio = (void *)0x20215040;


static void enable_uart_mmio_regs(void){
	// disable transmitter and receiver page 17
	put32(&uart_mmio->CNTL, 0);
	// clear out IO reg for sending/receiving data page 11
	put32(&uart_mmio->IO, 0);
	// clear to no interrupts page 12
	put32(&uart_mmio->IER, 0);
	// clear receive/transmit FIFO by setting bit 1 and 2 page 13
	put32(&uart_mmio->IIR, 0b110);
	// errata write 0b11 for 8bit; credits to dwelch page 14
	put32(&uart_mmio->LCR, 0b11);
	// don't need this so clear for good measure page 14
	put32(&uart_mmio->MCR, 0);
	// read register, but set to zero for good measure page 15
	put32(&uart_mmio->LSR, 0);
	// don't need this so clear for good measure page 15
	put32(&uart_mmio->MSR, 0);
	// don't need this so clear for good measure page 16
	put32(&uart_mmio->SCRATCH, 0);
	// read only register so clear for good measure page 18
	put32(&uart_mmio->STAT, 0);
	// sets our baud rate register to the desired baud value 115200
	// via formula on page 11 (register described on page 19)
	put32(&uart_mmio->BAUD, BAUD_VAL);
	// enable transmitter and receiver page 17
	put32(&uart_mmio->CNTL, 0b11);
}

void uart_init(void) {
	dev_barrier(); // start with barrier for safety

	// Lab 4 had gpio_init() but $(LIBPI_PATH)/rpi.h doesn't define
	//gpio_init(); // setting up gpio tx/rx function page 102
	gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
	gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
	dev_barrier(); // doc says need device barrier page 7

	//enable aux for mini uart page 9
	put32((void *)AUX_ENABLES, 0b1);
	dev_barrier();

	//enable uart mmio registers
	enable_uart_mmio_regs();
	dev_barrier();
}

int uart_getc(void) {
	// first bit of STAT register says whether symbol available page 19
	while((get32(&uart_mmio->STAT) & 0b1) != 0b1){
		;
	}

	// to get from FIFO read queue, get last byte of IO reg page 11
	return get32(&uart_mmio->IO) & 0xFF;
}
void uart_putc(unsigned c) {
	// second bit of STAT register says whether space is available
	while((get32(&uart_mmio->STAT) & 0b10) != 0b10){
		;
	}

	// to push to FIFO write queue, set last byte of IO reg page 11
	put32(&uart_mmio->IO, c & 0xFF);
}
