/* ***************************************************
   ***                                             ***
   ***   EOS Init (c) 2020-2021 by Elmer Hoeksema  ***
   ***                                             ***
   ***************************************************
  
   Copyright (c) 2020-2021, Elmer Hoeksema
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
       * Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "eos.h"

#define EOS_PI3ID                           0x0D03
#define EOS_PI4ID                           0x0D08
#define EOS_PI3BASE                         0x3F000000
#define EOS_PI4BASE                         0xFE000000

#define EOS_CORE1_PTR                       0x000000E0
#define EOS_CORE2_PTR                       0x000000E8
#define EOS_CORE3_PTR                       0x000000F0

#define EOS_GPIO_BASE                       eosBase + 0x00200000
#define EOS_GPFSEL0                         (EOS_GPIO_BASE)
#define EOS_GPFSEL1                         (EOS_GPIO_BASE + 0x04)
#define EOS_GPFSEL2                         (EOS_GPIO_BASE + 0x08)
#define EOS_GPSET0                          (EOS_GPIO_BASE + 0x1C)
#define EOS_GPCLR0                          (EOS_GPIO_BASE + 0x28)
#define EOS_GPLEV0		            (EOS_GPIO_BASE + 0x34)

#define EOS_UART0_BASE                      eosBase + 0x00201000
#define EOS_UART0_DR                        (EOS_UART0_BASE + 0x00)
#define EOS_UART0_FR                        (EOS_UART0_BASE + 0x18)

#define EOS_ASCII_ESC                       0x1B
#define EOS_TERM_CLS                        "[2J"
#define EOS_TERM_HOME                       "[H"

UI64 eosBase;

void stage2(UI32 piId, UI32 exceptionLevel, UI64 sp);
void hang();
void core1_start();
void core2_start();
void core3_start();
void core1_stage2();
void core2_stage2();
void core3_stage2();
void blink(UI32 knop, UI32 bit1, UI32 bit2, UI32 bit3, UI32 delay);
void putcs(char c);
char getcs();
void clearsScreen();
void prints(char* str);
void printsln(char* str);

/* entry point
   DO NOT USE ANY VARIABLES OR FUNCTION CALLS IN THIS FUNCTION!!!
   If you do, gcc will add a stack adjustment at the top of the function,
   before the stack is initialized. This will result in a hang at boot!!!
*/
void start() {
  asm volatile (
    " mov  x0, #0x00003000     \n " // set stack pointer
    " mov  sp, x0              \n "
    " mrs  x0, midr_el1        \n " // get pi version id
    " and  x0, x0, #0x0000FFF0 \n " // mask out other bits
    " lsr  x0, x0, #4          \n " // shift bits to end
    " mrs  x1, CurrentEL       \n " // get current exception level
    " and  x1, x1, #0x0000000C \n " // mask out other bits
    " lsr  x1, x1, #2          \n " // shift bits to end
    " mov  x2, sp              \n "
    " bl   stage2              \n " // go to stage 2
  );

  // Execution should never reach this line, but just to be sure
  for(;;) asm volatile ( " wfe \n "); // hang
}

void stage2(UI32 piId, UI32 exceptionLevel, UI64 sp) {
  // if not pi 3 or pi4 -> hang
  if((piId != EOS_PI3ID) && (piId != EOS_PI4ID)) hang();

  if(piId == EOS_PI3ID) {
    eosBase = EOS_PI3BASE;
  } else {
    eosBase = EOS_PI4BASE;
  }

  clearsScreen();
  printsln("EOS 1.2 (c) 2020-2021 by Elmer Hoeksema");

// stoplicht 1
  put32(EOS_GPFSEL0, get32(EOS_GPFSEL0) | 0x00048000);
  put32(EOS_GPFSEL1, get32(EOS_GPFSEL1) | 0x08000200);
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL1) & 0xFF70EFFF) | 0x00048000);
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xCEFFF1FF) | 0x08000200);
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xFFE3FFFF) | 0x08000000);

// stoplicht 2
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL0) & 0xFFFFFFC7) | 0x00000000); // GPIO 1
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xFFFFFE3F) | 0x00000040); // GPIO 12
  put32(EOS_GPFSEL2, (get32(EOS_GPFSEL2) & 0xFFFFFFF8) | 0x00000001); // GPIO 20
  put32(EOS_GPFSEL2, (get32(EOS_GPFSEL2) & 0xFFFFFFC7) | 0x00000008); // GPIO 21

// stoplicht 3
  put32(EOS_GPFSEL2, (get32(EOS_GPFSEL2) & 0xFFFC7FFF) | 0x00000000); // GPIO 25
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xF8FFFFFF) | 0x01000000); // GPIO 18
  put32(EOS_GPFSEL2, (get32(EOS_GPFSEL2) & 0xFFFFF1FF) | 0x00000200); // GPIO 23
  put32(EOS_GPFSEL2, (get32(EOS_GPFSEL2) & 0xFFFF8FFF) | 0x00001000); // GPIO 24

// stoplicht 4
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xFFFFFFC7) | 0x00000000); // GPIO 11
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL0) & 0xFFE3FFFF) | 0x00040000); // GPIO 6
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL0) & 0xFFFC7FFF) | 0x00008000); // GPIO 5
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL0) & 0xFFFFFFF8) | 0x00000001); // GPIO 0

  put32(EOS_CORE1_PTR, (UI32)((UI64)core1_start));
  put32(EOS_CORE2_PTR, (UI32)((UI64)core2_start));
  put32(EOS_CORE3_PTR, (UI32)((UI64)core3_start));
  asm volatile ( " sev \n ");

  blink(11, 6, 5, 0, 3000000);

  hang();
}

void hang() {
  for(;;) asm volatile ( " wfe \n "); // hang
}

void core1_start() {
  asm volatile (
    " mov  x0, #0x00004000     \n " // set stack pointer
    " mov  sp, x0              \n "
    " bl   core1_stage2        \n " // go to stage 2
  );
}

void core2_start() {
  asm volatile (
    " mov  x0, #0x00005000     \n " // set stack pointer
    " mov  sp, x0              \n "
    " bl   core2_stage2        \n " // go to stage 2
  );
}

void core3_start() {
  asm volatile (
    " mov  x0, #0x00006000     \n " // set stack pointer
    " mov  sp, x0              \n "
    " bl   core3_stage2        \n " // go to stage 2
  );
}

void core1_stage2() {
  blink(16, 9, 13, 19, 3000000);
  hang();
}

void core2_stage2() {
  blink(1, 12, 20, 21, 3000000);
  hang();
}

void core3_stage2() {
  blink(25, 18, 23, 24, 3000000);
  hang();
}

void blink(UI32 knop, UI32 bit1, UI32 bit2, UI32 bit3, UI32 delay) {
    put32(EOS_GPSET0, 0x00000000 | BIT(bit1) | BIT(bit2) | BIT(bit3));
  for(;;) {
    if(!(get32(EOS_GPLEV0) & BIT(knop)))	
    {  
      
      put32(EOS_GPCLR0, 0x00000000 | BIT(bit1));
      for(UI32 i = 0; i < delay; i++);
      put32(EOS_GPSET0, 0x00000000 | BIT(bit1));
      put32(EOS_GPCLR0, 0x00000000 | BIT(bit2));
      for(UI32 i = 0; i < delay; i++);
      put32(EOS_GPSET0, 0x00000000 | BIT(bit2));
      put32(EOS_GPCLR0, 0x00000000 | BIT(bit3));
      for(UI32 i = 0; i < delay; i++);
      put32(EOS_GPSET0, 0x00000000 | BIT(bit3));

    }
  }
}

void putcs(char c) {
  while(get32(EOS_UART0_FR) & BIT(5));
  put32(EOS_UART0_DR, c);
}

char getcs() {
  while(get32(EOS_UART0_FR) & BIT(4));
  UI32 data = get32(EOS_UART0_DR);

  return data;
}

void clearsScreen() {
  // Use VT100 commands to clear screen and move cursor to position 0, 0
  putcs((char)EOS_ASCII_ESC);
  prints(EOS_TERM_CLS);
  putcs((char)EOS_ASCII_ESC);
  prints(EOS_TERM_HOME);
}

void prints(char* str) {
  for (UI32 i = (UI32)0; str[i] != '\0'; i ++) {
    putcs((char)str[i]);
  }
}

void printsln(char* str) {
  prints(str);
  putcs('\r');
  putcs('\n');
}
