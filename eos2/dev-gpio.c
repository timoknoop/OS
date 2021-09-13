#include "eos.h"
#include "dev-gpio.h"

void blink() {
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xFFE3FFFF) | 0x00040000);
  for(;;) {
    put32(EOS_GPCLR0, 0x00010000);
    for(UI32 i = 0; i < 2000000; i++);
    put32(EOS_GPSET0, 0x00010000);
    for(UI32 i = 0; i < 2000000; i++);
  }
}
