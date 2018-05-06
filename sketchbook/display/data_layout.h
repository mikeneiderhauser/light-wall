#ifndef __ALW_STRUCTURES__
#define __ALW_STRUCTURES__

#include <stdint.h>

#define A6281_CFG_REG 0b01000111111100011111110001111111
#define PKD __attribute__((__packed__))
typedef struct PKD{
  union {
    struct PKD{
      uint32_t grn : 10;  // Bits 0 - 9   :  Green
      uint32_t red : 10;  // Bits 10 - 19 :  Red
      uint32_t blu : 10;  // Bits 20 - 29 :  Blue
      uint32_t adr : 1;   // Bit  30      :  Address (Always 0)
      uint32_t dc : 1 ;   // Bit  31      :  Don't Care Bit
    };
    struct PKD{
      uint32_t DCR0 : 7;  // Bits 0 - 6   : DC Reg 0 -> PWM Reg 0 config -> 1111111
      uint32_t CKMD : 2;  // Bits 7 - 8   : Clock Mode -> 00 = 800 kHz, 10 = 400 kHz, 01 = External, 11 = 200 kHz  -> 00 = 800 kHz
      uint32_t DC_0 : 1;  // Bit 9        : Don't Care Bit -> 0
      uint32_t DCR1 : 7;  // Bits 10 - 16 : DC Reg 1 -> PWM Reg 1 config -> 1111111
      uint32_t DC_1 : 3;  // Bits 17 - 19 : Don't Care Bits -> 000
      uint32_t DCR2 : 7;  // Bits 20 - 26 : DC Reg 2 -> PWM Reg 2 config -> 1111111
      uint32_t DC_2 : 1;  // Bit 27       : Don't Care Bit -> 0
      uint32_t MNFG : 2;  // Bits 28 - 29 : Mnfg Reserved -> 00
      uint32_t ADDR : 1;  // Bit 30       : Address 1 -> 1
      uint32_t DC_3 : 1;  // Bit 31       : Don't Care Bit -> 0
    };
    uint32_t data; // 32 bit raw  
  };
} lp2_pixel;  // sizeof(lp2_pixel) = 4 bytes
#endif /* __ALW_STRUCTURES__ */
