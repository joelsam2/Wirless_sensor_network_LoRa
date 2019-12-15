#include "LoRa.h"
#include "lpc40xx.h"
#include <lpc_peripherals.h>
#include <stdio.h>

void ssp2__init(void) {

  LPC_SC->PCONP |= (1 << 20); // Power On SSP Processor
  LPC_SSP2->CPSR = 8;         // Set SPI clk frequency to 8 MHz

  LPC_IOCON->P1_0 = 0x00000004; // SCLK
  LPC_IOCON->P1_1 = 0x00000004; // MOSI
  LPC_IOCON->P1_4 = 0x00000004; // MISO
  LPC_GPIO0->DIR |= (1 << 7);   // CS

  // Control Registers
  LPC_SSP2->CR0 = 0x0000007; // 8 - bit transfer mode
  LPC_SSP2->CR1 |= (1 << 1); // Enable SSP processor
}

// Active Low
void chip_select() { LPC_GPIO0->CLR = (1 << 7); } // CS

void chip_deselect() { LPC_GPIO0->SET = (1 << 7); } // CS

// Send and receive data by checking status register
uint8_t ssp2__exchange_byte1(uint8_t data_out) {

  while (!(LPC_SSP2->SR & (1 << 1)))
    ;
  LPC_SSP2->DR = (uint8_t)data_out;
  while (LPC_SSP2->SR & (1 << 4))
    ;
  return (uint8_t)(LPC_SSP2->DR & 0xFF);
}
