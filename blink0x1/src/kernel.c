
#include "utils.h"
#include "mini_uart.h"

#define GPFSEL1 0x3F200004
#define GPSET0  0x3F20001C
#define GPCLR0  0x3F200028
void kernel_main(void)
{
    unsigned int ra;
    ra=get32(GPFSEL1);
    ra&=~(7<<18);
    ra|=1<<18;
    put32(GPFSEL1,ra);

    while(1)
    {
        put32(GPSET0,1<<16);
        for(ra=0;ra<0x100000;ra++) burn();
        put32(GPCLR0,1<<16);
        for(ra=0;ra<0x100000;ra++) burn();
    }
}

void kernel_child(void)
{
	hang();

}
