#include "rpi.h"
#include "gpio.h"

#define GPIO_BASE 0x20200000
#define GPSET0  (GPIO_BASE+0x1c)
#define GPCLR0  (GPIO_BASE+0x28)
#define GPLEV0  (GPIO_BASE+0x34)

// NOTE: saw the Dawson's code was already checked in
// Reindented and changed a few things, but wanted to make
// clear that this was not written from scratch.

int gpio_set_function(unsigned pin, unsigned val) {
  //make sure correct pin range (look at device for max pin num)
  if(pin >= 32){
    return -1;
  }
  //make sure val is correct (can only be 3 bits)
	if((val & 0b111) != val){
  		return -1;
  }

  //broadcom page 90-2 on function select
  unsigned *gpio_reg = ((unsigned *)GPIO_BASE + pin/10);
  unsigned offset = (pin%10)*3;

  //masking as showed in lab
  unsigned v = get32(gpio_reg);
  v &= ~(0b111 << offset);
  v |= (val << offset);
  put32(gpio_reg, v);
  return 0;
}

unsigned gpio_get_function(unsigned pin){
  //make sure correct pin range (look at device for max pin num)
  if(pin >= 32){
    return -1;
  }

  //broadcom page 90-2 on function select
  unsigned *gpio_reg = ((unsigned *)GPIO_BASE + pin/10);
  unsigned offset = (pin%10)*3;

  //masking as showed in lab
  unsigned v = get32(gpio_reg);
  return (v >> offset) & 0b111;
}

int gpio_set_output(unsigned pin) {
	return gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

int gpio_set_input(unsigned pin) {
	return gpio_set_function(pin, GPIO_FUNC_INPUT);
}

int gpio_set_off(unsigned pin) {
  //make sure correct pin range (look at device for max pin num)
	if(pin >= 32)
		return -1;

  //broadcom page 95; need to turn clear "on"
	PUT32(GPCLR0, 1 << pin);
	return 0;
}
int gpio_set_on(unsigned pin) {
  //make sure correct pin range (look at device for max pin num)
	if(pin >= 32)
		return -1;

  //broadcom page 95; turn on via set reg
	PUT32(GPSET0, 1 << pin);
	return 0;
}

int gpio_write(unsigned pin, unsigned v) {
	if(v)
		return gpio_set_on(pin);
	else
		return gpio_set_off(pin);
}

unsigned gpio_read(unsigned pin) {
	assert(pin < 32);
  //broadcom page 96; get value via lev reg
  unsigned bank  = (GPLEV0 + pin/32);
  unsigned off = (pin%32);
	return (GET32(bank) >> off) & 1;
}
