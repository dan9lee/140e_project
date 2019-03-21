#ifndef	_BOOTLOADER_H
#define	_BOOTLOADER_H

#define PROGRAM_ADDRESS (VA_START + 0x1000000)
void load_program(void);
unsigned timer_get_time(void);
void rpi_reboot(void);
void clean_reboot(void);

#endif  /*_MINI_UART_H */
