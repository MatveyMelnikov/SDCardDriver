#include "sdcard_driver.h"
#include <string.h>
#include "crc-buffer.h"

HAL_StatusTypeDef activate_sd_card_init_process(SPI_HandleTypeDef *hspi)
{
	sd_card_command cmd58 = get_cmd(58, 0x0); // Reads OCR to get supported voltage
	sd_card_command cmd55 = get_cmd(55, 0x0);
	sd_card_command acmd41 = get_cmd(41, 0x0);
	
	sd_card_r3_response cmd58_response = { 0 };
	sd_card_r1_response cmd55_response = { 0 };
	sd_card_r1_response acmd41_response = { 0 };
	
	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd58, 6, HAL_MAX_DELAY);
	receive_cmd_response(hspi, (uint8_t*)&cmd58_response, 5, HAL_MAX_DELAY);
	
	// TODO: check response r3 - voltage
	
	uint32_t tickstart = HAL_GetTick();
	
	// Тут идет цикл. Мы дожидаемся, пока карта проинициализируется
	do
	{
		HAL_SPI_Transmit(hspi, (uint8_t*)&cmd55, 6, HAL_MAX_DELAY);
		receive_cmd_response(hspi, (uint8_t*)&cmd58_response, 5, HAL_MAX_DELAY);
		
		HAL_SPI_Transmit(hspi, (uint8_t*)&acmd41, 6, HAL_MAX_DELAY);
		receive_cmd_response(hspi, (uint8_t*)&acmd41_response, 5, HAL_MAX_DELAY);
		
		if (acmd41_response)
			break;
		if (acmd41_response & ILLEGAL_COMMAND)
			return HAL_ERROR;
	} while (HAL_GetTick() - tickstart < 10000); // example timeout value (TODO)
	
	return HAL_OK;
}

HAL_StatusTypeDef sdcard_reset(SPI_HandleTypeDef *hspi)
{
	sd_card_command cmd0 = get_cmd(0, 0x0);
	// 2.7-3.6V and check pattern
	sd_card_command cmd8 = get_cmd(8, (1 << 8) | 0x55);
	sd_card_command cmd58 = get_cmd(58, 0x0);
	
	sd_card_r1_response cmd0_response = 0;
	sd_card_r7_response cmd8_response = { 0 };
	sd_card_r3_response cmd58_response = { 0 };
	
	// TODO: + CS pin Asserted("0")
	// Как-то нужно проверки нужно упаковать

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_StatusTypeDef status = HAL_SPI_Transmit(hspi, (uint8_t*)&cmd0, 6, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	if (status)
		return status;
	HAL_SPI_Receive(hspi, &cmd0_response, 1, HAL_MAX_DELAY);
	if (status)
		return status;
	
	// TODO: check r1 errors
	
	//cmd_list[1].argument |= (1 << 8);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd8, 6, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	//HAL_SPI_Receive(hspi, (uint8_t*)&cmd8_response, 5, HAL_MAX_DELAY);
	status = receive_cmd_response(hspi, (uint8_t*)&cmd8_response, 5, HAL_MAX_DELAY);
	if (status)
		return status;
	// TODO: check r7. if illegal command - cmd 58
	
	// Разделение на ветки по версиям sd карты. Всего их - 3
	
	// illegal command => version 2.0 or later sd card
	if (cmd8_response.high_order_part & ILLEGAL_COMMAND)
		return activate_sd_card_init_process(hspi); // plus goto?
	
	// check pattern or voltage inconsistency
	if (!(cmd8_response.echo_back_of_check_pattern == 0x55 &&
		cmd8_response.voltage_accepted & 0x1))
		return HAL_ERROR;
	
	activate_sd_card_init_process(hspi); // TODO - check
	
	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd58, 6, HAL_MAX_DELAY); // Read OCR
	receive_cmd_response(hspi, (uint8_t*)&cmd58_response, 5, HAL_MAX_DELAY);
	
	//if (cmd58_response.ocr_register_content & (1 << 30)) // CCS - card capacity status
		// version 2.0 or later, high capacity or extended capacity
	//else
		// version 2.0 or later, standart capacity
	
	return HAL_OK;
}


HAL_StatusTypeDef receive_cmd_response(
	SPI_HandleTypeDef *hspi, 
	uint8_t* response, 
	uint8_t response_size, 
	uint32_t timeout
)
{
	sd_card_r1_response r1 = 0;
	uint8_t buffer[6] = {0}; // Max length of cmd response - 7 bytes
	
	// First we receive the first byte - r1
	HAL_StatusTypeDef status = HAL_SPI_Receive(hspi, &r1, 1, timeout);
	*response = r1;
	
	if (r1 || status)
		return HAL_ERROR;
	
	// If there are no errors, then we can continue to receive data
	if (response_size > 1)
	{
		status = HAL_SPI_Receive(
			hspi, (uint8_t*)buffer, response_size - 1, timeout
		);
	
		memcpy(response, buffer + 1, response_size - 1);
	}
	
	return status;
}

sd_card_command get_cmd(uint8_t cmd_num, uint32_t arg)
{
	crc_buffer_7 crc_buffer;
	sd_card_command cmd = (sd_card_command) {
		.start_block = 0x40 | cmd_num,
	};
  memcpy((void*)&cmd.argument, (void*)&arg, 4);
	cmd.crc_block = crc_buffer_calculate_crc_7(&crc_buffer, (uint8_t*)&cmd + 1, 5);
	
	return cmd;
}
