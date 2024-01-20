/*
* Auxiliary and common elements
*/

#include "sd_driver_secondary.h"
#include "crc-buffer.h"
#include "string.h"

// Implementations -----------------------------------------------------------

sd_error sd_card_receive_byte(
  SPI_HandleTypeDef *const hspi, 
  uint8_t* data
)
{
  uint8_t dummy_data = 0xff;

  return HAL_SPI_TransmitReceive(
    hspi, &dummy_data, data, 1, SD_TRANSMISSION_TIMEOUT
  );
}

sd_error sd_card_receive_bytes(
  SPI_HandleTypeDef *const hspi,
  uint8_t* data,
  const uint16_t size
)
{
  sd_error status = SD_OK;

  for (uint16_t i = 0; i < size; i++)
    status |= sd_card_receive_byte(hspi, data + i);
  
  return status;
}

sd_error sd_card_transmit_byte(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data
)
{
  return HAL_SPI_Transmit(
    hspi, (uint8_t *)data, sizeof(uint8_t), SD_TRANSMISSION_TIMEOUT
  );
}

sd_error sd_card_transmit_bytes(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data,
  const uint16_t size
)
{
  sd_error status = SD_OK;

  for (uint16_t i = 0; i < size; i++)
    status |= HAL_SPI_Transmit(hspi, (uint8_t *)(data + i), 1, SD_TRANSMISSION_TIMEOUT);
  
  return status;
}

sd_command sd_card_get_cmd_without_crc(
  const uint8_t cmd_num, const uint32_t arg
)
{
  sd_command cmd = (sd_command) {
    .start_block = 0x40 | cmd_num,
  };

  for (uint8_t i = 0; i < 4; i++) // Reverse memcpy
  {
    uint8_t shift = i << 3;
    cmd.argument[3 - i] = (arg & (0xff << shift)) >> shift;
  }
	
  return cmd;
}

sd_command sd_card_get_cmd(const uint8_t cmd_num, const uint32_t arg)
{
  crc_buffer_7 crc_buffer;
  sd_command cmd = sd_card_get_cmd_without_crc(cmd_num, arg);

  cmd.crc_block = crc_buffer_calculate_crc_7(
    &crc_buffer, (uint8_t*)&cmd, sizeof(cmd) - 1
  );
	
  return cmd;
}

sd_error sd_card_receive_data_block(
  SPI_HandleTypeDef *const hspi,
  uint8_t* data,
  const uint16_t data_size
)
{
  crc_buffer_16 crc_buffer = { 0 };
  crc_16_result received_crc = { 0 };
  uint8_t token = 0x0;

  // The token is sent with a significant delay
  sd_error status = sd_card_wait_response(hspi, &token, 0xff);
  if (token != 0xfe)
    return HAL_ERROR;

  status |= sd_card_receive_bytes(hspi, data, data_size);
  status |= sd_card_receive_bytes(hspi, (uint8_t*)&received_crc, 2);

  crc_16_result crc_result = crc_buffer_calculate_crc_16(
    &crc_buffer, data, data_size
  );

  // In the calculated CRC16, the bytes are in reverse order
  if (!(received_crc.i8[1] == crc_result.i8[0] && 
    received_crc.i8[0] == crc_result.i8[1]))
    return SD_CRC_ERROR;
  else
    return status;
}

// We are trying to get a non-zero byte.
// The received byte is written to the argument
sd_error sd_card_wait_response(
  SPI_HandleTypeDef *const hspi,
  uint8_t* received_value,
  const uint8_t idle_value
)
{
  sd_error status = SD_OK;
  uint32_t captured_tick = HAL_GetTick();

  do
  {
    if ((HAL_GetTick() - captured_tick) > SD_TRANSMISSION_TIMEOUT)
      return HAL_TIMEOUT;

    status |= sd_card_receive_byte(hspi, received_value);
  } while (*received_value == idle_value);

  return status;
}

sd_error sd_card_receive_cmd_response(
	SPI_HandleTypeDef *const hspi, 
	uint8_t* response, 
	const uint8_t response_size
)
{
  sd_r1_response r1 = 0;
  // Max length of cmd response - 5 bytes 
  // (excluding starting r1)
  uint8_t buffer[5] = { 0 };
	
  // We receive the first byte - r1
  sd_error status = sd_card_wait_response(hspi, &r1, 0xff);
  *response = r1;

  if (status)
    return status;
	
  // If there are no errors, then we can continue to receive data
  if (response_size > 1)
  {
    status = sd_card_receive_bytes(
      hspi, (uint8_t*)buffer, response_size - 1
    );
	
    memcpy(response + 1, buffer, response_size - 1);
  }
	
  return status;
}

bool is_partial_block_possible(
  SPI_HandleTypeDef *const hspi
)
{
  sd_command cmd_send_csd = sd_card_get_cmd(9, 0);
  uint8_t csd_register[16] = { 0 };

  // We request the CSD register to check the ability to set the block size
  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi,
    (uint8_t*)&cmd_send_csd,
    sizeof(cmd_send_csd),
    SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_data_block(hspi, csd_register, 16);
  DISELECT_SD();

  if (status)
    return false;
  else
    return csd_register[6] & 0x80;
}

sd_error sd_card_set_block_len(
  SPI_HandleTypeDef *const hspi,
  const uint32_t length
)
{
  sd_command cmd_set_blocklen = sd_card_get_cmd(16, length);
  sd_r1_response r1 = { 0 };
  sd_error status = 0x0;

  // The maximum block length is given by 512 Bytes
  if (!is_partial_block_possible(hspi) || length > 512)
    return SD_INCORRECT_ARGUMENT;
  
  SEND_CMD(hspi, cmd_set_blocklen, r1, status);
  if (r1)
    return SD_TRANSMISSION_ERROR;

  return status;
}
