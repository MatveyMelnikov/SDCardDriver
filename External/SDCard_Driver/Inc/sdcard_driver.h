#ifndef SDCARD_DRIVER_H
#define SDCARD_DRIVER_H

#include <stdint.h>
#include "stm32f1xx.h" // If you don't connect it, there will be HAL errors
#include "stm32f1xx_hal_spi.h"

// Defines -------------------------------------------------------------------

#define sd_card_r1_response uint8_t // 0xxx.xxxx (MSB first)

#define sd_card_r1b_response uint8_t // busy token. 0xxx.xxxx (MSB first)

// Macros --------------------------------------------------------------------

#define SELECT_CHIP() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

#define DISELECT_CHIP() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

// Structs -------------------------------------------------------------------

typedef struct {
	uint8_t crc_block; // CRC7 + '1'
	//uint32_t argument;
  uint8_t argument[4];
	uint8_t start_block; // '0' + '1' + command index (6 bits)
} sd_card_command;
	
typedef enum {
	IN_IDLE_STATE = 0x1,
	ERASE_RESET,
	ILLEGAL_COMMAND,
	COM_CRC_ERROR,
	ERASE_SEQUENCE_ERROR,
	ADDRESS_ERROR,
	PARAMETER_ERROR
} r1_error_mask;

typedef struct {
	uint8_t card_status;
	sd_card_r1_response high_order_part;
} sd_card_r2_response;

typedef struct {
	uint32_t ocr_register_content;
	sd_card_r1_response high_order_part;
} sd_card_r3_response;

typedef struct {
	uint8_t crc_block;
	uint8_t echo_back_of_check_pattern;
	uint8_t voltage_accepted;
	uint8_t reserved_bits;
	uint8_t command_version;
	sd_card_r1_response high_order_part;
} sd_card_r7_response;

// Functions -----------------------------------------------------------------

// CMD 0 - Resets the SD Memory Card
HAL_StatusTypeDef sdcard_reset(SPI_HandleTypeDef *hspi);

// response - can have size > 1!
HAL_StatusTypeDef receive_cmd_response(
	SPI_HandleTypeDef *hspi, 
	uint8_t* response, 
	uint8_t response_size, 
	uint32_t timeout
);

sd_card_command get_cmd(uint8_t cmd_num, uint32_t arg);

#endif
