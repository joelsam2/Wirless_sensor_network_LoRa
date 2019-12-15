#include "LoRa.h"
#include "lpc40xx.h"
#include "spi_lora.h"
#include <stdint.h>
#include <stdio.h>

struct lora_variables {
  uint32_t frequency_set;
  uint32_t packet_counter;
  uint8_t is_implicit;
  uint8_t version;
} lora_variables = {0, 0, 0, 0};

typedef enum bandwidth {
  bandwidth_7800 = 0,
  bandwidth_10400 = 1,
  bandwidth_15600 = 2,
  bandwidth_20800 = 3,
  bandwidth_31250 = 4,
  bandwidth_41700 = 5,
  bandwidth_62500 = 6,
  bandwidth_125000 = 7,
  bandwidth_250000 = 8,
  bandwidth_500000 = 9,
} bandwidth;

uint8_t ssp_read(uint8_t address) {

  uint8_t read_result;
  chip_select();

  read_result = ssp2__exchange_byte1(address);
  read_result = ssp2__exchange_byte1(0x00);

  chip_deselect();
  return read_result;
}

void ssp_write(uint8_t address, uint8_t value) {

  uint8_t write_result;
  chip_select();
  uint8_t spi_address = (address | (1 << 8));

  write_result = ssp2__exchange_byte1(spi_address);
  write_result = ssp2__exchange_byte1(value);

  chip_deselect();
  return write_result;
}

uint8_t receiver_rssi() {
  uint8_t value;
  if (lora_variables.frequency_set < 862E6)
    value = 164;
  else
    value = 157;

  uint8_t result = ssp_read(REG_PKT_RSSI_VALUE);
  return (result - value);
}

void set_frequency(uint32_t frequency) {
  lora_variables.frequency_set = frequency;
  printf("frequency is %d,%d\n", frequency, lora_variables.frequency_set);

  int fstep = (32000000 / 524288);
  uint32_t frf = (uint32_t)(frequency / fstep);

  ssp_write(REG_FRF_MSB, (uint8_t)(frf >> 16));
  ssp_write(REG_FRF_MID, (uint8_t)(frf >> 8));
  ssp_write(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

void explicitHeaderMode() {
  lora_variables.is_implicit = 0;

  ssp_write(REG_MODEM_CONFIG1, ssp_read(REG_MODEM_CONFIG1) & 0xFE); // clear last bit to enable explicit mode
}

// coding rate values = 1,2,3,4
void set_coding_rate(uint8_t coding_rate) {
  if ((coding_rate < 1) || coding_rate == 1) {
    coding_rate = 1;
    ssp_write(REG_MODEM_CONFIG1, (ssp_read(REG_MODEM_CONFIG1) & 0xf1) | (coding_rate << 1));
  } else if (coding_rate == 2) {
    coding_rate = 2;
    ssp_write(REG_MODEM_CONFIG1, (ssp_read(REG_MODEM_CONFIG1) & 0xf1) | (coding_rate << 1));
  } else if (coding_rate == 3) {
    coding_rate = 3;
    ssp_write(REG_MODEM_CONFIG1, (ssp_read(REG_MODEM_CONFIG1) & 0xf1) | (coding_rate << 1));
  } else if ((coding_rate > 4) || (coding_rate == 4)) {
    coding_rate = 4;
    ssp_write(REG_MODEM_CONFIG1, (ssp_read(REG_MODEM_CONFIG1) & 0xf1) | (coding_rate << 1));
  }
}

void setSpreadingFactor(uint8_t sf) {
  if (sf < 6)
    sf = 6;
  if (sf > 12)
    sf = 12;

  if (sf == 6) {
    ssp_write(REG_DETECT_OPTIMIZE, 0xC5); // page 114
    ssp_write(REG_DETECTION_THRESHOLD, 0x0C);
    set_implicit_header();
  } else {
    ssp_write(REG_DETECT_OPTIMIZE, 0xC3);
    ssp_write(REG_DETECTION_THRESHOLD, 0x0A);
  }

  ssp_write(REG_MODEM_CONFIG2, (ssp_read(REG_MODEM_CONFIG2) & HIGHER_NIBBLE) | ((sf << 4) & LOWER_NIBBLE));
}

void set_bandwidth(bandwidth input_bandwidth) {
  ssp_write(REG_MODEM_CONFIG1, (ssp_read(REG_MODEM_CONFIG1) & HIGHER_NIBBLE) | (input_bandwidth << 4));
}

void set_implicit_header() {
  lora_variables.is_implicit = 1;

  ssp_write(REG_MODEM_CONFIG1, ssp_read(REG_MODEM_CONFIG1) | 0x01);
}

void setPreambleLength(uint64_t length) {
  uint8_t msb = (uint8_t)(length >> 8);
  uint8_t lsb = (uint8_t)(length >> 0);
  ssp_write(REG_PREAMBLE_MSB_LORA, msb);
  ssp_write(REG_PREAMBLE_LSB_LORA, lsb);
}

void enable_crc() { ssp_write(REG_MODEM_CONFIG2, ssp_read(REG_MODEM_CONFIG2) | 0x04); }

void disable_crc() { ssp_write(REG_MODEM_CONFIG2, ssp_read(REG_MODEM_CONFIG2) & 0xfb); }

void mode_idle() { ssp_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY); }

void mode_sleep() { ssp_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP); }

void set_lora_tx_power(uint8_t level) {
  if (level < 2) {
    level = 2;
  } else if (level > 17) {
    level = 17;
  }

  ssp_write(REG_PA_CONFIG, PA_BOOST | (level - 2));
}

int available() { return (ssp_read(REG_RX_NB_BYTES) - lora_variables.packet_counter); }

int read_data_in_payload() {
  if (!available()) {
    return -1;
  }

  lora_variables.packet_counter++;
  uint8_t result = ssp_read(REG_FIFO);
  return result;
}

int parse_received_packet(uint8_t header) {
  int packet_length = 0;
  int irqFlags = ssp_read(REG_IRQ_FLAGS);

  if (header > 0) {
    set_implicit_header();
    ssp_write(REG_PAYLOAD_LENGTH_LORA, header & 0xff);
  } else {
    explicitHeaderMode();
  }

  ssp_write(REG_IRQ_FLAGS, irqFlags);

 if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
    lora_variables.packet_counter = 0;

    // read packet length
    if (lora_variables.is_implicit) {
      packet_length = ssp_read(REG_PAYLOAD_LENGTH_LORA);
    } else {
      packet_length = ssp_read(REG_RX_NB_BYTES);
    }

    // set FIFO address to current RX address
    ssp_write(REG_FIFO_ADDR_PTR, ssp_read(REG_FIFO_RX_CURRENT_ADDR));

    // put in standby mode
    mode_idle();
  } else if (ssp_read(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
    ssp_write(REG_FIFO_ADDR_PTR, 0);

    // put in single RX mode
    ssp_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
  }

  return packet_length;
}

void lora_delay() {
  for (int i = 0; i < 500000; i++) {
    for (int j = 0; j < 20000; j++)
      ;
  }
}

void lora_reset() {
  LPC_GPIO0->DIR |= (1 << 0);
  LPC_GPIO0->PIN &= ~(1 << 0);
  lora_delay();
  LPC_GPIO0->PIN |= (1 << 0);
  lora_delay();
}


int lora_init(uint32_t frequency) {

  lora_reset();

  // Enable SPI of ARM CORTEX
  ssp2__init();
  chip_deselect();

  // check version
  lora_variables.version = ssp_read(REG_VERSION);
  printf("Version is %x\n", lora_variables.version);

  mode_sleep();

  // set frequency
  set_frequency(frequency);
  ssp_write(REG_FIFO_TX_BASE_ADDR, 0x00);
  ssp_write(REG_FIFO_RX_BASE_ADDR, 0x00);
  ssp_write(REG_LNA, ssp_read(REG_LNA) | 0x03);
  set_lora_tx_power(17);

  mode_idle();
  printf("Initialization successful\n");

  return 1;
}
