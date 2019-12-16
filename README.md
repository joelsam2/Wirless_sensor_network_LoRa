# Wirless_sensor_network_LoRa

Two transmitters sending temperature data to one receiver at the same frequency using delay (TDMA).

Project Workflow - 
On the transmitter side, temperature sensors (TMP36) were interfaced to each transmitter. We used Arduino microcontroller (Atmega 328P) and interfaced it to LoRa transmitters which were continuously sending temperature readings in data packets. At the receiver node, LoRa module was interfaced to NXP LPC408x processor (ARM Cortex M4). We developed SPI and LoRa driver code in C language. The drivers were developed based on the state diagrams given in SX1276/77/78/79 datasheet and we also referenced the arduino library code. Developing the driver code and understanding the entire physical memory mapping of LoRa registers was an important and challenging part of our project. For changing the three important parameters different registers were accessed. For changing the spreading factor register regmodemconfig2 is used. Similarly, for changing the bandwidth and coding rate register regmodemconfig1 was accessed. At the receiver end, for parsing the data we first checked the interrupt flags to check if the received data is corrupted or not in register regirqflags. Then we calculated the received payload in bytes by accessing the register regrxnbbytes. Data was then read from regfifo. Multiple pointers were to be set to correct base addresses to ensure correct decoding of received payload. Several other physical registers were accessed to configure multiple other factors such a CRC, explicit/implicit header (regmodemconfig1), frequency (regfrfmsb, regfrfmid, reffrflsb), set transmit power (regpaconfig), idle mode (regopmode), sleep mode (regopmode), preamble length (setpreamblemsb, setpreamblelsb). 

Tranmistter node - Arduino code for LoRa transmistter can be downloaded from https://www.arduinolibraries.info/libraries/lo-ra

Receiver node - We used NXP LPC408x processor (ARM Cortex M4) at the receiver end. 

SPI and LoRa drivers that we developed, can be simply used by importing the code and header files.
We have created multiple API's that can be called by importing these drivers in your ARM based development board.

STEPS TO FOLLOW AND API REFERENCES - 

1) lora_init(uint32_t frequency) - this API is used to initialize the LoRa module and must be called before any other API. Pass the ISM operating frequency of your country. For USA it is 915Mhz.
2) parse_received_packet(uint8_t header)  - This API is used to parse the received data packet. Pass value = 0 for explicit header mode and value = 1 for implicit header mode.
3) read_data_in_payload() - This API is used to read the data from received packet payload. It returns a character
4) set_implicit_header() - This API can be called to set the header as implicit. In implicit mode the Tx and Rx must be set with payload information before transmission
5) set_explicit_header() - This API can be called to set the header as explicit.
6) uint8_t receiver_rssi() - In order to calculate the received packet RSSI simply call this API and read the value it returns. Unit of RSSI is dBm.
7) setPreambleLength(uint64_t length) - This API is used to vary the packet preamble length. Defualt preamble is set to 8
8) enable_crc() - This API is used to enable the CRC. It is advised to enable crc for good signal transmission
9) diable_crc() - This API is used to disable the optional CRC check
10) set_lora_tx_power(uint8_t level) - This API is used to set the Transmitter power. For our module the value passed was 17 as default.
11) set_coding_rate(uint8_t coding_rate) - This API can used to change the coding rate. Values range from 1-4
12) setSpreadingFactor(uint8_t sf) - This API can be called to change the spreading factor to improve resistivity to interference. Values range from 6-12.
13) set_bandwidth(bandwidth input_bandwidth) - This API can used to change the bandwidth o Lora from 7.8Khz to 500Khz

Our drivers have been developed in such a way that, one can simply call the above API's and change various physical register values without actually having to write exhaustive code and understand memory mapping of LoRa physical registers, thus making it easier for future use and development.


