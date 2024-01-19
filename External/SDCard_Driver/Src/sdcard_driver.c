#include "sdcard_driver.h"
#include <string.h>

// Variables -----------------------------------------------------------------

sd_status sd_card_status = { 0 };

// Static functions ----------------------------------------------------------

static sd_error sd_card_receive_byte(
  SPI_HandleTypeDef *const hspi, 
  uint8_t* data
)
{
  uint8_t dummy_data = 0xff;

  return HAL_SPI_TransmitReceive(
    hspi, &dummy_data, data, 1, SD_TRANSMISSION_TIMEOUT
  );
}

static sd_error sd_card_receive_bytes(
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

static sd_error sd_card_transmit_byte(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data
)
{
  return HAL_SPI_Transmit(
    hspi, data, sizeof(uint8_t), SD_TRANSMISSION_TIMEOUT
  );
}

static sd_error sd_card_transmit_bytes(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data,
  const uint16_t size
)
{
  sd_error status = SD_OK;

  for (uint16_t i = 0; i < size; i++)
    status |= HAL_SPI_Transmit(hspi, data + i, 1, SD_TRANSMISSION_TIMEOUT);
  
  return status;
}

// To IDLE state
static sd_error sd_card_power_on(SPI_HandleTypeDef *const hspi)
{
  sd_error status = SD_OK;
  uint8_t dummy_data = 0xff;

  DISELECT_SD();

  // Clock occurs only during transmission
  for (uint8_t i = 0; i < 10; i++) // Need at least 74 ticks
    status |= HAL_SPI_Transmit(hspi, &dummy_data, 1, SD_TRANSMISSION_TIMEOUT);

  // Just in case, we get the result
  status |= sd_card_receive_byte(hspi, &dummy_data);
  return status;
}

static sd_error sd_card_enter_spi_mode(SPI_HandleTypeDef *const hspi)
{
  static bool sd_card_is_spi_mode = false;
  sd_command cmd0 = sd_card_get_cmd(0, 0x0);
  sd_r1_response cmd0_response = 0;

  if (sd_card_is_spi_mode)
    return SD_OK;

  sd_error status = sd_card_power_on(hspi);
  if (status)
    return status;

  SELECT_SD();
  status |= HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd0, sizeof(cmd0), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_byte(hspi, &cmd0_response);

  uint32_t tickstart = HAL_GetTick();
  do
  {
    status |= sd_card_receive_byte(hspi, &cmd0_response);
    if (HAL_GetTick() - tickstart > 500)
      return SD_TIMEOUT;
  } while (cmd0_response != R1_IN_IDLE_STATE);

  DISELECT_SD();

  if (status == SD_OK)
    sd_card_is_spi_mode = true;

  return status;
}

// Host should enable CRC verification before issuing ACMD41
static sd_error sd_card_crc_on_off(
  SPI_HandleTypeDef *const hspi,
  const bool crc_enable
) 
{
  sd_command cmd59 = sd_card_get_cmd(59, crc_enable ? 0x1 : 0x0);
  sd_r1_response r1 = { 0 };
  sd_error status = 0x0;

  SEND_CMD(hspi, cmd59, r1, status);
  if (r1 != R1_IN_IDLE_STATE)
    return SD_TRANSMISSION_ERROR;

  return status;
}

static sd_error sd_card_v1_init_process(SPI_HandleTypeDef *const hspi)
{
  // Reads OCR to get supported voltage
  sd_command cmd58 = sd_card_get_cmd(58, 0x0);
  sd_command cmd55 = sd_card_get_cmd(55, 0x0);
  sd_command acmd41 = sd_card_get_cmd(41, 0x0);
  sd_r3_response cmd58_response = { 0 };
  sd_r1_response cmd55_response = { 0 };
  sd_r1_response acmd41_response = { 0 };
  sd_error status = SD_OK;

  SEND_CMD(hspi, cmd58, cmd58_response, status);
	
  // Check voltage. The card must support 2.7-3.6V
  // MSB. Second byte is 23 - 16 bits of OCR
  // Third byte starts with 15 bit oof OCR
  if (!((cmd58_response.ocr_register_content[1] & 0x1f) &&
    (cmd58_response.ocr_register_content[2] & 0x80)))
    return HAL_ERROR;

  uint32_t tickstart = HAL_GetTick();
  while (true)
  {
    SEND_CMD(hspi, cmd55, cmd55_response, status);
    SEND_CMD(hspi, acmd41, acmd41_response, status);
		
    if (acmd41_response == R1_CLEAR_FLAGS)
      break;
    if (acmd41_response & R1_ILLEGAL_COMMAND)
      return SD_UNUSABLE_CARD;
    // Card initialization shall be completed within 1 second 
    // from the first ACMD41
    if (HAL_GetTick() - tickstart > 1000)
      return SD_TIMEOUT;
  }

  sd_card_status.version = 1;
  sd_card_status.capacity = STANDART;

  return status;
}

static sd_error sd_card_v2_init_process(
  SPI_HandleTypeDef *const hspi
)
{
  // Reads OCR to get supported voltage
  sd_command cmd58 = sd_card_get_cmd(58, 0x0);
  sd_command cmd55 = sd_card_get_cmd(55, 0x0);
  sd_command acmd41 = sd_card_get_cmd(41, 0x0);
  sd_r3_response cmd58_response = { 0 };
  sd_r1_response cmd55_response = { 0 };
  sd_r1_response acmd41_response = { 0 };
  sd_error status = SD_OK;

  SEND_CMD(hspi, cmd58, cmd58_response, status);
	
  // Check voltage. The card must support 2.7-3.6V
  // MSB. Second byte is 23 - 16 bits of OCR
  // Third byte starts with 15 bit oof OCR
  if (!((cmd58_response.ocr_register_content[1] & 0x1f) &&
    (cmd58_response.ocr_register_content[2] & 0x80)))
    return SD_UNUSABLE_CARD;

  uint32_t tickstart = HAL_GetTick();
  while (true)
	{
    SEND_CMD(hspi, cmd55, cmd55_response, status);
    SEND_CMD(hspi, acmd41, acmd41_response, status);
		
    if (acmd41_response == R1_CLEAR_FLAGS)
      break;
    // Card initialization shall be completed within 1 second 
    // from the first ACMD41
    if (HAL_GetTick() - tickstart > 1000)
      return SD_TIMEOUT;
  }

  // Read OCR
  SEND_CMD(hspi, cmd58, cmd58_response, status);
  if (cmd58_response.ocr_register_content[0] & 0x2) // Check CCS
    sd_card_status.capacity = HIGH_OR_EXTENDED;
  else
    sd_card_status.capacity = STANDART;
  sd_card_status.version = 2;
	
  return status;
}

// Implementations -----------------------------------------------------------

sd_error sd_card_reset(SPI_HandleTypeDef *const hspi, const bool crc_enable)
{
  // 2.7-3.6V and check pattern
  sd_command cmd8 = sd_card_get_cmd(8, (1 << 8) | 0x55);
  sd_r7_response cmd8_response = { 0 };

  sd_error status = sd_card_enter_spi_mode(hspi);

  if (status)
    goto end_of_initialization;

  SEND_CMD(hspi, cmd8, cmd8_response, status);

  if (status)
    goto end_of_initialization;
	
  status |= sd_card_crc_on_off(hspi, crc_enable);

  // Illegal command hence version 1.0 sd card
  if (cmd8_response.high_order_part & R1_ILLEGAL_COMMAND)
  {
    status |= sd_card_v1_init_process(hspi);
    goto end_of_initialization;
  }

  // Check pattern or voltage inconsistency
  if (!(cmd8_response.echo_back_of_check_pattern == 0x55 &&
    GET_VOLTAGE_FROM_R7(cmd8_response) == 0x1))
  {
    status |= SD_UNUSABLE_CARD;
    goto end_of_initialization;
  }
	
  status |= sd_card_v2_init_process(hspi);

end_of_initialization:
  sd_card_status.error_in_initialization = (bool)status;
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

  // cmd.crc_block = crc_buffer_calculate_crc_7(
  //   &crc_buffer, (uint8_t*)&cmd, sizeof(cmd) - 1
  // );
  cmd.crc_block = crc_buffer_calculate_crc_7(
    &crc_buffer, (uint8_t*)&cmd, sizeof(cmd) - 1
  );
	
  return cmd;
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

bool is_partial_block_possible(
  SPI_HandleTypeDef *const hspi
)
{
  sd_command cmd9 = sd_card_get_cmd(9, 0); // get CSD
  uint8_t csd_register[16] = { 0 };

  // We request the CSD register to check the ability to set the block size
  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd9, sizeof(cmd9), SD_TRANSMISSION_TIMEOUT
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
  sd_command cmd16 = sd_card_get_cmd(16, length);
  sd_r1_response r1 = { 0 };
  sd_error status = 0x0;

  // The maximum block length is given by 512 Bytes
  if (!is_partial_block_possible(hspi) || length > 512)
    return SD_INCORRECT_ARGUMENT;
  
  SEND_CMD(hspi, cmd16, r1, status);
  if (r1)
    return SD_TRANSMISSION_ERROR;

  return status;
}

sd_error sd_card_read_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  uint8_t* data,
  const uint32_t block_length
)
{
  sd_command cmd17 = sd_card_get_cmd(17, address);
  sd_r1_response r1 = { 0 };

  SELECT_SD();
  HAL_StatusTypeDef status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd17, sizeof(cmd17), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_read;

  status |= sd_card_receive_data_block(
    hspi, data, block_length
  );

end_read:
  DISELECT_SD();
  return status;
}

sd_error sd_card_read_multiple_data(
  SPI_HandleTypeDef *const hspi, 
  const uint32_t address,
  uint8_t* data,
  const uint32_t block_length,
  const uint32_t number_of_blocks
)
{
  sd_command cmd18 = sd_card_get_cmd(18, address);
  sd_command cmd12 = sd_card_get_cmd(12, address);
  sd_r1_response r1 = { 0 };
  uint8_t busy_signal = 0;

  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd18, sizeof(cmd18), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_read;

  for (uint32_t i = 0; i < number_of_blocks; i++)
  {
    status |= sd_card_receive_data_block(
      hspi, data + (i * block_length), block_length
    );
  }

  status |= HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd12, sizeof(cmd12), SD_TRANSMISSION_TIMEOUT
  );

  // Do we always get 0xef in r1?
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);  
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

end_read:
  DISELECT_SD();
  return status;
}

// data_size - user data size!
sd_error sd_card_transmit_data_block(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data,
  const uint16_t data_size,
  const uint8_t start_token
)
{
  crc_buffer_16 crc_buffer = { 0 };
  uint8_t data_response = 0x0;
  uint8_t busy_signal = 0;

  crc_16_result crc_result = crc_buffer_calculate_crc_16(
    &crc_buffer, data, data_size
  );

  sd_error status = sd_card_transmit_byte(hspi, &start_token);
  status |= sd_card_transmit_bytes(hspi, data, data_size);
  status |= sd_card_transmit_bytes(
    hspi, (uint8_t*)&crc_result, sizeof(crc_16_result)
  );

  status |= sd_card_receive_byte(hspi, &data_response);
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

  switch (data_response & 0xf)
  {
    case SD_DATA_RESPONSE_CRC_ERROR:
      status = SD_CRC_ERROR;
      break;
    case SD_DATA_RESPONSE_WRITE_ERROR:
      status = SD_ERROR;
      break;
    case SD_DATA_RESPONSE_ACCEPTED:
      break;
    default:
      status |= SD_TRANSMISSION_ERROR;
  }

  return status;
}

sd_error sd_card_write_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  const uint8_t *const data,
  const uint32_t block_length
)
{
  sd_command cmd24 = sd_card_get_cmd(24, address);
  sd_r1_response r1 = { 0 };

  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd24, sizeof(cmd24), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_write;

  // 0xfe - start token of single block write
  status |= sd_card_transmit_data_block(
    hspi, data, block_length, 0xfe
  );

end_write:
  DISELECT_SD();
  return status;
}

sd_error sd_card_write_multiple_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  const uint8_t *const data,
  const uint32_t block_length,
  const uint32_t number_of_blocks
)
{
  sd_command cmd25 = sd_card_get_cmd(25, address);
  sd_r1_response r1 = { 0 };
  uint8_t stop_token = 0xfd;
  uint8_t busy_signal = 0;

  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd25, sizeof(cmd25), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_write;

  for (uint32_t i = 0; i < number_of_blocks; i++)
  {
    // 0xfc - start token of multiple block write
    status |= sd_card_transmit_data_block(
      hspi, data + (i * block_length), block_length, 0xfc
    );
  }

  if (status)
    goto end_write;

  status |= sd_card_transmit_byte(hspi, &stop_token);
  // The busy signal does not appear immediately. This is not
  // described in the documentation
  status |= sd_card_wait_response(hspi, &busy_signal, 0xff);
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

end_write:
  DISELECT_SD();
  return status;
}
