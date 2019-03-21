# Introduction

We developed a bare metal implementation of a multicore bootloader for the Raspberry Pi 3B and a parallel reduce program/interface.

For the shell, The master core (core 0) communicates with our unix-side pi-shell over UART. We can use the pi-shell to specify a program to run on a particular slave core (1-3). The master core receives the program, writes it to memory, then sends the specified slave core a message containing the address of the program. The slave receives an interrupt then jumps to the address - thus it starts running the program. We can also send the slave an interrupt to stop running the program or to run a different program. Using this framework, we have a flexible way to control and run 3 programs simultaneously. This is especially useful for conducting hard, real-time tasks. The shell code is in directories bootloader/ and shell/. The parallel reduce code is in parallel-pi/ and test programs are integers/.

This project was created by Daniel Lee and Justin Rose for the CS140E final project. Special thanks to Dawson Engler for his amazing guidance and for teaching us so much this quarter! We would also like to acknowledge [s-matyekuvich](https://github.com/s-matyukevich/raspberry-pi-os) for his incredibly useful raspi3 tutorial. Our bootloader built off of his code base. In fact, we spent a lot of time going through and understanding his tutorial.

## How to Run

Place the necessary [firmware](https://github.com/raspberrypi/firmware/tree/master/boot) files (bootcode.bin, start.elf) and config.txt in bootloader/src on the pi's SD card. To run the bootloader, place bootloader/kernel8.img on the pi's SD card. Attach the pi to your computer via UART-USB. In the shell/shell-unix-side, run pi-shell. This should start a shell with the PIX:> prompt. Enter "echo hello world" to test that the pi is communicating properly. Within shell-unix-side, there are three blink_n.img files. Each one is statically linked to n * 0x1000000. If you enter "send 2 blink3.img" that will send the blink3 program to be run on core 2 of the raspberry pi. Similarly, entering "send 1 blink2.img" will send blink2 to be run on core 1 of the raspberry pi. If you enter stop 2, this will send core 2 a mailbox interrupt which triggers it to halt its program execution.

To run the parallel-pi code, add the same firmware files and kernel8.img from parallel-pi to the sd card. To run user programs like integers2, use ``$ my-install integers2/kernel8.img`` where ``my-install`` is the program we wrote earlier in the quarter.

To build the code we used [Docker](https://www.docker.com/). The ``build.sh`` script in each directory has the necessary Docker command to run a container that builds the code. It's as simple as ``./build.sh`` to make and ``./build.sh clean`` to clean. This was a much easier development flow since we didn't need to worry about potential Mac issues with building.

## Differences from Raspi A+

There are many differences between the Raspi 3B and the Raspi A+ we used in class. [dwelch](https://github.com/dwelch67/raspberrypi) provides an overview. The Raspi 3B has 4 cores, 1GB of device memory, 32/64-bit support, and uses ARMv8, compared to the Raspi A+ single core, 512MB of device memory, 32-bit support, and ARMv6. What is really frustrating is that there is very little documentation for the Raspi 3B's hardware. Although it uses the Broadcom BCM2837 SoC, we only have acces to the BCM2835 documentation along with an [extra revision sheet](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf). The [ARMv8 doc](https://static.docs.arm.com/ddi0487/ca/DDI0487C_a_armv8_arm.pdf?_ga=2.24464634.1940315503.1552730203-875611155.1549335030) is also very necessary. Big idea is that Raspi 3B is very different from A+ and it was difficult to adapt code from our labs. Thus, we built off of s-matyekuvich's repository.

# Boot up

Let's get into the nitty-gritty. We will examine bootloader/src. So how does the Raspi boot up? When the device is powered on, the GPU reads config.txt on the SD card to get some configuration parameters. Then, it loads kernel8.img into memory and starts executing from .text.boot (in linker.ld). Now, refer to boot.S. All four cores start executing at \_start. The first thing they do is check their core number from the lower 8 bits of the mpidr_el1 register. s-matyekuvich and most other online OS examples only use a single core, so they hang on all cores except the master (core 0). Instead, we have the master and slave cores diverge and follow two separate paths. All cores set relevant system registers, disabling MMU, disabling caches, set endianness, mask interrupts, set hypervisor state to choose 64-bit mode, etc. We borrowed this code from s-mat. But basically, when we eret (exception return), we jump down to exception level 1 and start executing at el1_entry/el1_entry_child for the master/slave cores respectively. Next the master zeros its bss and all four cores set their respective stack pointers (based on values we define in mm.h). Then, the master branches to kernel_main and all three slaves branch to kernel_child.

# Mailbox Interrupts

The slaves have a very simple kernel because most of their work is defined in the interrupt handler. All the cores do is initialize their mailboxes, initialize the interrupt request vector, enable interrupts, then hang. Let's look a bit more closesly at each of these steps.

In mbox.c, we define mbox_init(). This function checks which core we are executing on, then uses this core number to determine the address of the core's local interrupt controller. We set the 1<<mboxNum of this register to enable interrupts for the specified mailbox on this core.

irq_vector_init() simply places the address of "vectors" into the vbar_el1, or vector base address register (for exception level 1). This vectors table is defined in entry.S and defines how we handle certain interrupts. We call el1_irq when we get an interrupt request at exception level 1. This function saves register state, calls our main interrupt request handler, handle_irq, then restores register state so we can safely enter and exit the kernel.

handle_irq is defined in irq.c. This function checks the source of the interrupt request based on the calling core number. If the interrupt is coming from one of the core's four mailboxes, we call handle_mbox_irq().

handle_mbox_irq() is the primary functionality for the slave cores. We've designated messages in mailbox 0 to mean we received an address of a new program to branch to. Thus, if the irq came from mailbox 0, we read the contents of mailbox 0, enable interrupts, then branch to the address we received. It is important that we enable interrupts because we are in the interrupt handler, meaning interrupts are automatically disabled, but we want to still receive messages from the master.
Messages in mailbox 1 mean the master wants us to stop execution. If we receive a message in this mailbox, we simply enable interrupts then hang, simulating that we've stopped the program.

mbox_send() takes a core number, mailbox number, and message as parameters. This function finds the address of the specified core's mailbox of interest and writes the message into that mailbox. The message can either be a value or an address. Writing addresses is particularly useful for our bootloader code. When we send a message to a core, that core receives a mailbox interrupt.

mbox_read() takes just the mailbox number as a parameter. On each core, we only read from our own four mailboxes. Thus, we use our core number and the specified mailbox number to read the message. Once we read the message, we clear the mailbox to allow other cores to continue writing to it.

One important feature of the mailboxes is that, when we are interested in reading from a mailbox, we use a different register from writing to that mailbox. The MBOX_SET registers are write-only and we can only write 1's. The MBOX_RDCLR registers are read/clear only - we can read a value and if we write a 1, this clears the corresponding bit. This improves atomicity for mailbox read/writes.

Figuring out how to use the mailboxes was fairly tricky. There aren't any good examples of using mailboxes/mailbox interrupts for multicore communication on the raspberry pi, and there is very [little documentation](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) about the raspi3 local peripherals. However, the mailboxes now provide a very clean and effective way to communicate between cores.

It took a very long time for us to distinguish between the mailbox described in the BCM document and the mailboxes described in QA7. We believe the mailbox described in the BCM document is a system-wide register for communicating with the GPU. This is associated with the general interrupt controller. However, each core has its own local interrupt controller along with 4 core-specific mailboxes for a total of 16 mailboxes and 4 local interrupt controllers. **THESE ARE COMPLETELY DIFFERENT FROM THE SYSTEM MAILBOX/INTERRUPT CONTROLLER**. It took us a while and a lot of experimentation to realize that these mailboxes/interrupt controllers are completely distinct.

# Pi Shell

Our multicore bootloader builds on the simple shell we used in lab 10. Basically, the master core receives directions from the unix-side pi-shell. We currently support functionality for three commands (echo, send, stop). Echo simply echoes whatever the user types in the pi-shell. Send first sends the pi the address to write a program to, then sends the program with a similar protocol as our bootloader. The program is written to the specified address then the master sends the address to the designated core's mailbox 0 to start its execution. Stop simply sends a message to the designated core's mailbox 1, forcing it to halt execution. The unix-side pi-shell is basically a simple, modified version of the lab10 pi-shell.

## Blink

We link blink_n.img at n * 0x1000000 specified in the linker.ld file. This is a simple test program that blinks on a specific gpio pin. We use these 3 programs linked at 3 distinct addresses to test execution on 3 cores.

# Parallel Pi
Our parallel pi is a cute trick to make a somewhat similar interface available that roughly is like some higher level multithreading coding paradigms like ISPC. The goal was to create a way to send over simple user programs via ``my-install`` like:
```
void kernel_main(int *arr, int n, int index, int c)
{
	for(int i=index; i<n; i+=c){
		int sum = 0;
		for(int j=i; j< i + 100;j++){
			sum += j;
		}
		arr[i] = sum;
	}
}
```

where arr is an array we would like to process, n is the total number of elements, i is the "core" index, and c is the total number of working cores. Thus, this abstracted away in the user program the core/thread orchestrating related code as all it needed to worry about was the abstract task it was trying to accomplish. For example the above program stores in ``arr[i]`` the sum of the numbers from `i` to `i+99` inclusive.

The way this was implemented was pretty straightforward. We modified our ``BRANCH_TO`` function to take additional registers which would be passed via `x0` - `x3`. Since the [AArch64 standard](https://en.wikichip.org/wiki/arm/aarch64#Calling_convention) for these registers is that they are parameter/return registers, we could use them to get code to our other program.



## Virtual Memory (attempted but failed)
Our ultimate goal was to get the Virtual Memory working for the system, but we ultimately failed. Although we were able to get the memory working on Parallel Pi to identity map the kernel memory space with a prepended ASID value of `0xffff` to our 64 bit addresses, and we also could make user pages, our code consistently hanged when we tried to branch from kernel memory mapped via the `ttbr1_el1` register to user code mapped via the `ttbr0_el1` register. Additionally we tried to get locks and mutexes working via the `ldrx` and `strx` commands as documented on page B2.9 of the manual, but our code would also hang when trying this as well. Although we think that these issues were most likely a small fix (changing a register/clearing a stale cache), the were notoriously hard to debug and we ultimately could not fix them. It is with great disappointment that we failed at this task, but on the bright side we got basic identity mapping working in the kernel.


# Conclusion
It was extremely rewarding to write some bare metal multi-threaded code on the RPi 3. If we were to do it again, we would really want to go through in detail the ARM v8a documentation starting very early, because the 64 bit architecture was quite different from the 32 bit v6 architecture we used for all our other labs in the class. The ARM v8-A Manual PDF is 6666 pages long, and it was much more difficult than anticipated to switch to both the new architecture paradigm as well as the multicore one. Hopefully over break when we both have more time, we will be able to figure out the underlying issue behind why the MMU would not branch. 
