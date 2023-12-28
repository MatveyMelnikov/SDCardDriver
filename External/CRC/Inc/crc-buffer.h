/*
Original code: https://github.com/MatveyMelnikov/CRC
These are inverted methods that are suitable for command structures to the SD card
*/

#ifndef CRC_BUFFER_H
#define CRC_BUFFER_H

#include <stdint.h>

typedef uint8_t crc_buffer_7;
typedef uint16_t crc_buffer_16;

/*
* Union for convenient division of the result by bits. For example:
* crc_16_result test = { 0xFF55 };
* test.i16; // Integral 16 bit acquisition
* test.u8[0]; // Receiving the first byte
* test.u8[1]; // Receiving the second byte
*/
typedef union {
	uint16_t i16;
	uint8_t i8[2];
} crc_16_result;

/*
* Calculates the CRC7 for the received data and outputs the last byte of 
* the message frame by sp (7 bits of crc7 + '1'). 
* Number of protected bits - 40 or 120
*/
uint8_t
crc_buffer_calculate_crc_7(
	crc_buffer_7* buffer,
	uint8_t* data,
	const uint8_t data_length
);

/*
* Calculates the CRC16 for the received data and outputs the 
* two bytes of CRC16. Number of protected bits - up to 16384
*/
crc_16_result
crc_buffer_calculate_crc_16(
	crc_buffer_16* buffer,
	uint8_t* data,
	const uint16_t data_length
);

#endif
