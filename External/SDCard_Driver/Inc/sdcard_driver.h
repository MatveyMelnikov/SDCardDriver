#ifndef SDCARD_DRIVER_H
#define SDCARD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h" // If you don't connect it, there will be HAL errors
#include "stm32f1xx_hal_spi.h"

// Defines -------------------------------------------------------------------

#define sd_card_r1_response uint8_t // 0xxx.xxxx (MSB first)

#define sd_card_r1b_response uint8_t // busy token. 0xxx.xxxx (MSB first)

// Macros --------------------------------------------------------------------

#define SELECT_SD() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

#define DISELECT_SD() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

#define GET_VERSION_FROM_R7(r7) \
  (((r7).command_version_plus_reserved & 0xf0) >> 4)

#define GET_VOLTAGE_FROM_R7(r7) \
  ((r7).voltage_accepted_plus_reserved & 0x0f)

#define SEND_CMD(hspi, cmd, response, status) \
  SELECT_SD(); \
	(status) |= HAL_SPI_Transmit((hspi), (uint8_t*)&(cmd), 6, HAL_MAX_DELAY); \
	(status) |= sd_card_receive_cmd_response( \
    (hspi), (uint8_t*)&(response), sizeof(response), HAL_MAX_DELAY \
  ); \
  DISELECT_SD();

// Structs -------------------------------------------------------------------

typedef struct {
  uint8_t start_block; // '0' + '1' + command index (6 bits)
  uint8_t argument[4];
  uint8_t crc_block; // CRC7 + '1'
} sd_card_command;
	
typedef enum {
  CLEAR_FLAGS = 0x0,
	IN_IDLE_STATE = 0x1,
	ERASE_RESET = 0x2,
	ILLEGAL_COMMAND = 0x4,
	COM_CRC_ERROR = 0x8,
	ERASE_SEQUENCE_ERROR = 0x10,
	ADDRESS_ERROR = 0x20,
	PARAMETER_ERROR = 0x40
} r1_error_mask;

typedef struct {
  sd_card_r1_response high_order_part;
	uint8_t card_status;
} sd_card_r2_response;

typedef struct {
  sd_card_r1_response high_order_part;
	uint8_t ocr_register_content[4];
} sd_card_r3_response;

typedef struct {
  sd_card_r1_response high_order_part;
  uint8_t command_version_plus_reserved;
  uint8_t reserved_bits;
  uint8_t voltage_accepted_plus_reserved;
	uint8_t echo_back_of_check_pattern;
} sd_card_r7_response;

typedef enum {
  STANDART = 0x0,
  HIGH_OR_EXTENDED
} sd_card_capacity;

typedef struct {
  uint8_t version;
  sd_card_capacity capacity;
  bool error_in_initialization;
} sd_card_status;

// Variables -----------------------------------------------------------------

extern sd_card_status sd_status;

// Functions -----------------------------------------------------------------

// CMD 0 - Resets the SD Memory Card
HAL_StatusTypeDef sd_card_reset(SPI_HandleTypeDef *hspi);

sd_card_command sd_card_get_cmd(uint8_t cmd_num, uint32_t arg);

HAL_StatusTypeDef sd_card_receive_byte(
  SPI_HandleTypeDef *hspi, 
  uint8_t* data
);

HAL_StatusTypeDef sd_card_receive_bytes(
  SPI_HandleTypeDef *hspi,
  uint8_t* data,
  const uint8_t size
);

HAL_StatusTypeDef sd_card_transmit_bytes(
  SPI_HandleTypeDef *hspi,
  uint8_t* data,
  const uint8_t size
);

//void sd_card_power_on(SPI_HandleTypeDef *hspi);

HAL_StatusTypeDef sd_card_wait_response(
  SPI_HandleTypeDef *hspi,
  uint8_t* data
);

// response - can have size > 1!
HAL_StatusTypeDef sd_card_receive_cmd_response(
	SPI_HandleTypeDef *hspi, 
	uint8_t* response, 
	uint8_t response_size, 
	uint32_t timeout
);

#endif
