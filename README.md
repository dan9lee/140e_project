# Introduction

We developed a bare metal implementation of a multicore bootloader for the Raspberry Pi 3B.

The master core (core 0) communicates with our unix-side pi-shell over UART. We can use the pi-shell to specify a program to run on a particular slave core (1-3). The master core receives the program, writes it to memory, then sends the specified slave core a message containing the address of the program. The slave receives an interrupt then jumps to the address - thus it starts running the program. We can also send the slave an interrupt to stop running the program or to run a different program. Using this framework, we have a flexible way to control and run 3 programs simultaneously. This is especially useful for conducting hard, real-time tasks.

This project was created by Daniel Lee and Justin Rose for the CS140E final project. Special thanks to Dawson Engler for his amazing guidance and for teaching us so much this quarter! We would also like to acknowledge [s-matyekuvich](https://github.com/s-matyukevich/raspberry-pi-os) for his incredibly useful raspi3 tutorial. Our bootloader built off of his code base.

## How to Run

Place the necessary [firmware](https://github.com/raspberrypi/firmware/tree/master/boot) files (bootcode.bin, start.elf) and config.txt in bootloader/src on the pi's SD card. To run the bootloader, place bootloader/kernel8.img on the pi's SD card. Attach the pi to your computer via UART-USB. In the shell/shell-unix-side, run pi-shell. This should start a shell with the PIX:> prompt. Enter "echo hello world" to test that the pi is communicating properly. Within shell-unix-side, there are three blink_n.img files. Each one is statically linked to n * 0x1000000. If you enter "send 2 blink3.img" that will send the blink3 program to be run on core 2 of the raspberry pi. Similarly, entering "send 1 blink2.img" will send blink2 to be run on core 1 of the raspberry pi. If you enter stop 2, this will send core 2 a mailbox interrupt which triggers it to halt its program execution.

## Differences from Raspi A+

# Boot up

# Interrupts

## Mailboxes

# Memory

## Blink

## Virtual Memory (attempted but failed)


# Conclusion 
