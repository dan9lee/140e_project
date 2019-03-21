#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "demand.h"
#include "trace.h"

#define __SIMPLE_IMPL__
#include "../shared-code/simple-boot.h"



static void send_byte(int fd, unsigned char b) {
	if(write(fd, &b, 1) < 0)
		panic("write failed in send_byte\n");
}
static unsigned char get_byte(int fd) {
	unsigned char b;
	int n;
	if((n = read(fd, &b, 1)) != 1)
		panic("read failed in get_byte: expected 1 byte, got %d\n",n);
	return b;
}

// NOTE: the other way to do is to assign these to a char array b and
//	return *(unsigned)b
// however, the compiler doesn't have to align b to what unsigned
// requires, so this can go awry.  easier to just do the simple way.
// we do with |= to force get_byte to get called in the right order
// 	(get_byte(fd) | get_byte(fd) << 8 ...)
// isn't guaranteed to be called in that order b/c | is not a seq point.
unsigned get_uint(int fd) {
        unsigned u = get_byte(fd);
        u |= get_byte(fd) << 8;
        u |= get_byte(fd) << 16;
        u |= get_byte(fd) << 24;
				trace_read32(u);
        return u;
}

void put_uint(int fd, unsigned u) {
	// mask not necessary.
				trace_write32(u);
        send_byte(fd, (u >> 0)  & 0xff);
        send_byte(fd, (u >> 8)  & 0xff);
        send_byte(fd, (u >> 16) & 0xff);
        send_byte(fd, (u >> 24) & 0xff);
}

// simple utility function to check that a u32 read from the
// file descriptor matches <v>.
void expect(const char *msg, int fd, unsigned v) {
	unsigned x = get_uint(fd);
	if(x != v){
		put_uint(fd, NAK);
		panic("%s: expected %x, got %x\n", msg, v,x);
	}
}

// unix-side bootloader: send the bytes, using the protocol.
// read/write using put_uint() get_unint().
void simple_boot(int fd, const unsigned char * buf, unsigned n) {
	// send SOH
	put_uint(fd, SOH);

	// send size
	fprintf(stderr, "Sending %d bytes\n", n);
	put_uint(fd, n);

	// send 32-bit crc
	u32 checksome = crc32(buf, n);
	put_uint(fd, checksome);

	expect("Didn\'t receive SOH from Pi", fd, SOH);
	expect("Didn\'t receive crc32(nbytes)", fd, crc32(&n, sizeof(unsigned)));
	expect("Didn\'t receive proper checksome", fd, checksome);

	// send ACK
	put_uint(fd, ACK);

	// send the program
	unsigned *prog = (unsigned *) buf;
	for(unsigned i=0; i< n/sizeof(unsigned); i++){
		put_uint(fd, prog[i]);
	}
	// put EOF
	put_uint(fd, EOT);

	expect("Didn\'t receive ACK from Pi", fd, ACK);
}
