#include "lpc40xx.h"
#include <stdint.h>

void ssp2__init(void);
uint8_t ssp2__exchange_byte1(uint8_t data_out);
void chip_select();
void chip_deselect();
