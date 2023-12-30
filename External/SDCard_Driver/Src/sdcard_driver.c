#include "sdcard_driver.h"
#include <string.h>
#include "crc-buffer.h"

sd_card_status sd_status = { 0 };

// To IDLE state
void sd_card_power_on(SPI_HandleTypeDef *hspi)
{
  uint8_t dummy_data = 0xff;

  DISELECT_SD();

  // Clock occurs only during transmission
  for (uint8_t i = 0; i < 10; i++) // Need at least 74 ticks
    HAL_SPI_Transmit(hspi, &dummy_data, 1, 500);

  sd_card_receive_byte(hspi, &dummy_data); // Just in case, we get the result
}

HAL_StatusTypeDef sd_card_enter_spi_mode(SPI_HandleTypeDef *hspi)
{
  static bool sd_card_is_spi_mode = false;
  sd_card_command cmd0 = sd_card_get_cmd(0, 0x0);
  sd_card_r1_response cmd0_response = 0;

  if (sd_card_is_spi_mode)
    return HAL_OK;

  sd_card_power_on(hspi);

  SELECT_SD();
	HAL_StatusTypeDef status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd0, 6, HAL_MAX_DELAY
  );
	status |= sd_card_receive_byte(hspi, &cmd0_response);

  uint32_t tickstart = HAL_GetTick();
  do
  {
    status |= sd_card_receive_byte(hspi, &cmd0_response);
    if (HAL_GetTick() - tickstart > 500)
      return HAL_TIMEOUT;
  } while (cmd0_response != IN_IDLE_STATE);

  DISELECT_SD();

  if (status == HAL_OK)
    sd_card_is_spi_mode = true;

  return status;
}

HAL_StatusTypeDef sd_card_v1_init_process(SPI_HandleTypeDef *hspi)
{
  // Reads OCR to get supported voltage
  sd_card_command cmd58 = sd_card_get_cmd(58, 0x0);
  sd_card_command cmd55 = sd_card_get_cmd(55, 0x0);
  sd_card_command acmd41 = sd_card_get_cmd(41, 0x0);
  sd_card_r3_response cmd58_response = { 0 };
  sd_card_r1_response cmd55_response = { 0 };
	sd_card_r1_response acmd41_response = { 0 };
  HAL_StatusTypeDef status = HAL_OK;

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
		
		if (acmd41_response == 0x0)
			break;
    if (acmd41_response & ILLEGAL_COMMAND)
			return HAL_ERROR;
    // Card initialization shall be completed within 1 second 
    // from the first ACMD41
    if (HAL_GetTick() - tickstart > 1000)
      return HAL_TIMEOUT;
	}

  sd_status.version = 1;
  sd_status.capacity = STANDART;

  return HAL_OK;
}

HAL_StatusTypeDef sd_card_v2_init_process(
  SPI_HandleTypeDef* hspi
)
{
  // Reads OCR to get supported voltage
	sd_card_command cmd58 = sd_card_get_cmd(58, 0x0);
	sd_card_command cmd55 = sd_card_get_cmd(55, 0x0);
	sd_card_command acmd41 = sd_card_get_cmd(41, 0x0);
	sd_card_r3_response cmd58_response = { 0 };
	sd_card_r1_response cmd55_response = { 0 };
	sd_card_r1_response acmd41_response = { 0 };
  HAL_StatusTypeDef status = HAL_OK;

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
		
		if (acmd41_response == 0x0)
			break;
    // Card initialization shall be completed within 1 second 
    // from the first ACMD41
    if (HAL_GetTick() - tickstart > 1000)
      return HAL_TIMEOUT;
	}

  // Read OCR
  SELECT_SD();
	status |= HAL_SPI_Transmit(hspi, (uint8_t*)&cmd58, 6, HAL_MAX_DELAY);
	status |= sd_card_receive_cmd_response(
    hspi, (uint8_t*)&cmd58_response, 5, HAL_MAX_DELAY
  );
  DISELECT_SD();

  if (cmd58_response.ocr_register_content[0] & 0x40) // Check CCS
    sd_status.capacity = STANDART;
  else
    sd_status.capacity = HIGH_OR_EXTENDED;
  sd_status.version = 2;
	
	return status;
}

HAL_StatusTypeDef sd_card_reset(SPI_HandleTypeDef *hspi)
{
	// 2.7-3.6V and check pattern
	sd_card_command cmd8 = sd_card_get_cmd(8, (1 << 8) | 0x55);
	sd_card_r7_response cmd8_response = { 0 };

  HAL_StatusTypeDef status = sd_card_enter_spi_mode(hspi);
  sd_card_enter_spi_mode(hspi);

  SEND_CMD(hspi, cmd8, cmd8_response, status);

	if (status)
		goto end_of_initialization;
	
	// Illegal command hence version 1.0 sd card
	if (cmd8_response.high_order_part & ILLEGAL_COMMAND)
  {
		status |= sd_card_v1_init_process(hspi);
    goto end_of_initialization;
  }

  // Check pattern or voltage inconsistency
	if (!(cmd8_response.echo_back_of_check_pattern == 0x55 &&
    GET_VOLTAGE_FROM_R7(cmd8_response) == 0x1))
		return HAL_ERROR;
	
	status |= sd_card_v2_init_process(hspi);

end_of_initialization:
  sd_status.error_in_initialization = (bool)status;
	return status;
}

sd_card_command sd_card_get_cmd(uint8_t cmd_num, uint32_t arg)
{
	crc_buffer_7 crc_buffer;
	sd_card_command cmd = (sd_card_command) {
		.start_block = 0x40 | cmd_num,
	};

  for (uint8_t i = 0; i < 4; i++) // Reverse memcpy
  {
    uint8_t shift = i << 3;
    cmd.argument[3 - i] = (arg & (0xff << shift)) >> shift;
  }

	cmd.crc_block = crc_buffer_calculate_crc_7(&crc_buffer, (uint8_t*)&cmd, 5);
	
	return cmd;
}

HAL_StatusTypeDef sd_card_receive_byte(
  SPI_HandleTypeDef *hspi, 
  uint8_t* data
)
{
  uint8_t dummy_data = 0xff;

  return HAL_SPI_TransmitReceive(hspi, &dummy_data, data, 1, 1000);
}

HAL_StatusTypeDef sd_card_receive_bytes(
  SPI_HandleTypeDef *hspi,
  uint8_t* data,
  const uint8_t size
)
{
  HAL_StatusTypeDef status = 0;

  for (uint8_t i = 0; i < size; i++)
    status += sd_card_receive_byte(hspi, data + i);
  
  return status;
}

HAL_StatusTypeDef sd_card_transmit_bytes(
  SPI_HandleTypeDef *hspi,
  uint8_t* data,
  const uint8_t size
)
{
  HAL_StatusTypeDef status = 0;

  for (int16_t i = size - 1; i >= 0; i--)
    status += HAL_SPI_Transmit(hspi, data + i, 1, 1000);
  
  return status;
}

// We are trying to get a non-zero byte.
// The received byte is written to the argument
HAL_StatusTypeDef sd_card_wait_response(
  SPI_HandleTypeDef *hspi,
  uint8_t* data
)
{
  HAL_StatusTypeDef status = 0;
  uint32_t captured_tick = HAL_GetTick();

  do
  {
    if ((HAL_GetTick() - captured_tick) > 500)
      return HAL_TIMEOUT;

    status |= sd_card_receive_byte(hspi, data);
  } while (data == NULL);

  return status;
}

HAL_StatusTypeDef sd_card_receive_cmd_response(
	SPI_HandleTypeDef *hspi, 
	uint8_t* response, 
	uint8_t response_size, 
	uint32_t timeout
)
{
	sd_card_r1_response r1 = 0;
  // Max length of cmd response - 5 bytes 
  // (excluding starting r1)
	uint8_t buffer[5] = { 0 };
	
	// First we receive the first byte - r1
  HAL_StatusTypeDef status = sd_card_wait_response(hspi, buffer);
	status |= sd_card_receive_byte(hspi, &r1);
	*response = r1;
	
	// if (r1 || status)
	// 	return HAL_ERROR;

  if (status)
		return status;
	
	// If there are no errors, then we can continue to receive data
	if (response_size > 1)
	{
		// status = HAL_SPI_Receive(
		// 	hspi, (uint8_t*)buffer, response_size - 1, timeout
		// );
    status = sd_card_receive_bytes(
			hspi, (uint8_t*)buffer, response_size - 1
		);
	
		memcpy(response + 1, buffer, response_size - 1);
	}
	
	return status;
}
