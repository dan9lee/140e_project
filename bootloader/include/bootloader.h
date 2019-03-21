#ifndef	_BOOTLOADER_H
#define	_BOOTLOADER_H
enum {
	ARMBASE=0x1000000, // where program gets linked.  we could send this.
        SOH = 0x12345678,   // Start Of Header

        BAD_CKSUM = 0x1,
        BAD_START,
        BAD_END,
	TOO_BIG,
	ACK,   // client ACK
        NAK,   // Some kind of error, restart
        EOT,   // end of transmission
};
#define GPFSEL1 0x3F200004
#define GPSET0  0x3F20001C
#define GPCLR0  0x3F200028
typedef unsigned u32;
typedef unsigned char u8;
u32 crc32(const void *buf, unsigned size);
void delay_cycles(unsigned ticks);
unsigned timer_get_time(void);
void delay_us(unsigned us);
void delay_ms(unsigned ms);
void rpi_reboot(void);
void send_byte(unsigned char uc);
unsigned char get_byte(void);
unsigned get_uint(void);
void put_uint(unsigned u);
void die(int code);
unsigned load(unsigned* addr);

#endif  /*_BOOTLOADER_H */
