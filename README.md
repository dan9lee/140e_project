# Introduction

We developed a bare metal implementation of a multicore bootloader for the Raspberry Pi 3B.

The master core (core 0) communicates with our unix-side pi-shell over UART. We can use the pi-shell to specify a program to run on a particular slave core (1-3). The master core receives the program, writes it to memory, then sends the specified slave core a message containing the address of the program. The slave receives an interrupt then jumps to the address - thus it starts running the program. We can also send the slave an interrupt to stop running the program or to run a different program. Using this framework, we have a flexible way to control and run 3 programs simultaneously. This is especially useful for conducting hard, real-time tasks.

This project was created by Daniel Lee and Justin Rose for the CS140E final project. Special thanks to Dawson Engler for his amazing guidance and for teaching us so much this quarter! We would also like to acknowledge [s-matyekuvich](https://github.com/s-matyukevich/raspberry-pi-os) for his incredibly useful raspi3 tutorial. Our bootloader built off of his code base.

## How to Run

Place the necessary [firmware](https://github.com/raspberrypi/firmware/tree/master/boot) files (bootcode.bin, start.elf) and config.txt in bootloader/src on the pi's SD card. To run the bootloader, place bootloader/kernel8.img on the pi's SD card. Attach the pi to your computer via UART-USB. In the shell/shell-unix-side, run pi-shell. This should start a shell with the PIX:> prompt. Enter "echo hello world" to test that the pi is communicating properly. Within shell-unix-side, there are three blink_n.img files. Each one is statically linked to n * 0x1000000. If you enter "send 2 blink3.img" that will send the blink3 program to be run on core 2 of the raspberry pi. Similarly, entering "send 1 blink2.img" will send blink2 to be run on core 1 of the raspberry pi. If you enter stop 2, this will send core 2 a mailbox interrupt which triggers it to halt its program execution.

## Differences from Raspi A+

There are many differences between the Raspi 3B and the Raspi A+ we used in class. [dwelch](https://github.com/dwelch67/raspberrypi) provides an overview. The Raspi 3B has 4 cores, 1GB of device memory, 32/64-bit support, and uses ARMv8, compared to the Raspi A+ single core, 512MB of device memory, 32-bit support, and ARMv6. What is really frustrating is that there is very little documentation for the Raspi 3B's hardware. Although it uses the Broadcom BCM2837 SoC, we only have acces to the BCM2835 documentation along with an [extra revision sheet](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf). Big idea is that Raspi 3B is very different from A+ and it was difficult to adapt code from our labs. Thus, we built off of s-matyekuvich's repository.

# Boot up

Let's get into the nitty-gritty. We will examine bootloader/src. So how does the Raspi boot up? When the device is powered on, the GPU reads config.txt on the SD card to get some configuration parameters. Then, it loads kernel8.img into memory and starts executing from .text.boot (in linker.ld). Now, refer to boot.S. All four cores start executing at \_start. The first thing they do is check their core number from the lower 8 bits of the mpidr_el1 register. s-matyekuvich and most other online OS examples only use a single core, so they hang on all cores except the master (core 0). Instead, we have the master and slave cores diverge and follow two separate paths. All cores set relevant system registers, disabling MMU, disabling caches, set endianness, hypervisor state to choose 64-bit mode, and the state of exception level 2/3. We borrowed this code from s-mat. But basically, when we eret (exception return), we jump down to exception level 1 and start executing at el1_entry/el1_entry_child for the master/slave cores respectively. Next the master zeros its bss and all four cores set their respective stack pointers (based on values we define in mm.h). Then, the master branches to kernel_main and all three slaves branch to kernel_child.

# Mailbox Interrupts

The slaves have a very simple kernel because most of their work is defined in the interrupt handler. All the cores do is initialize their mailboxes, initialize the interrupt request vector, enable interrupts, then hang. Let's look a bit more closesly at each of these steps.


# Memory

## Blink

## Virtual Memory (attempted but failed)


# Conclusion 
