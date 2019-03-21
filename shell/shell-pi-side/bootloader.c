/* simplified bootloader.  */
#include "rpi.h"
#include "pi-shell.h"

#define __SIMPLE_IMPL__
#include "../bootloader/shared-code/simple-boot.h"

static void send_byte(unsigned char uc) {
	uart_putc(uc);
}
static unsigned char get_byte(void) {
        return uart_getc();
}

unsigned get_uint(void) {
	unsigned u = get_byte();
        u |= get_byte() << 8;
        u |= get_byte() << 16;
        u |= get_byte() << 24;
	return u;
}
void put_uint(unsigned u) {
        send_byte((u >> 0)  & 0xff);
        send_byte((u >> 8)  & 0xff);
        send_byte((u >> 16) & 0xff);
        send_byte((u >> 24) & 0xff);
}

static void die(unsigned err) {
	put_uint(err);
	delay_ms(100); 	// let the output queue drain.
	rpi_reboot();
}

// load_code:
//	1. figure out if the requested address range is available.
//	2. copy code to that region.
//	3. return address of first executable instruction: note we have
//	a 8-byte header!  (see ../hello-fixed/memmap)
int load_code(void) {
	unsigned addr=0;

	// let unix know we are ready.
	put_uint(ACK);

	// printk("pi is sanity checking");

	unsigned version = get_uint();
	addr = get_uint();
	unsigned nbytes = get_uint();
	unsigned cksum = get_uint();

	if(version != 2) {
		die(NAK);
	}

	if(addr < LAST_USED_ADDRESSES || addr +nbytes > MAX_ADDRESS) {
		die(NAK);
	}

	if(nbytes % 4 !=0 ){
		die(NAK);
	}




	// bootloader code.

  put_uint(ACK);
	unsigned * program = (unsigned *) addr;
	unsigned int i;
	for(i = 0; i < nbytes/4; i++) {
		program[i] = get_uint();
	}

	// check for EOT
	if(get_uint() != EOT) die(NAK);

	// verify that the program was correct
	if(cksum != crc32(program, nbytes)) die(NAK);

	// acknowledge
	put_uint(ACK);
        // give time to flush out; ugly.   implement `uart_flush()`


	delay_ms(500);
	BRANCHTO(addr+8);

	/* return address */
	return addr;
}
