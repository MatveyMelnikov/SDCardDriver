#include "crc-buffer.h"
#include <stdbool.h>

// 'i >> 3' equivalent 'i / 8' and 'i & 0x7' equivalent 'i % 8'
#define GET_INPUT_BIT(pData, i) \
	*((pData) + (i >> 3)) & (1 << (7 - (i & 0x7)))

uint8_t
crc_buffer_calculate_crc_7(
	crc_buffer_7* buffer,
	uint8_t* data,
	const uint8_t data_length
)
{
	*buffer = 0x0;

	for (uint8_t i = 0; i < data_length * 8; i++)
	{
		bool inputBit = GET_INPUT_BIT(data, i);
		// XOR input bit with the bit evicted from the buffer
		inputBit ^= (bool)(*buffer & (1 << 6));

		*buffer <<= 1;
		// According to the documentation, we XOR the 
		// 4th position with the input bit
		*buffer ^= (inputBit << 3);
		*buffer |= inputBit;
	}

	return (*buffer << 1) | 1;
}

crc_16_result
crc_buffer_calculate_crc_16(
	crc_buffer_16* buffer,
	uint8_t* data,
	const uint16_t data_length
)
{
	*buffer = 0x0;

	for (uint16_t i = 0; i < data_length * 8; i++)
	{
		bool inputBit = GET_INPUT_BIT(data, i);
		// XOR input bit with the bit evicted from the buffer
		inputBit ^= (bool)(*buffer & (1 << 15));

		*buffer <<= 1;
		// According to the documentation, we XOR the 6th and 
		// 13th positions with the input bit.
		*buffer ^= (inputBit << 5) | (inputBit << 12);
		//*buffer ^= (inputBit << 12);
		*buffer |= inputBit;
	}

	return (crc_16_result) { *buffer };
}
