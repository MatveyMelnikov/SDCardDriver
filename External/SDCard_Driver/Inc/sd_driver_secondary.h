/*
* Auxiliary and common elements
*/

#ifndef SD_DRIVER_SECONDARY_H
#define SD_DRIVER_SECONDARY_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h" // If you don't include it, there will be HAL errors
#include "stm32f1xx_hal_spi.h"

// Defines -------------------------------------------------------------------

#define sd_r1_response uint8_t

#define sd_r1b_response uint8_t

#define SD_TRANSMISSION_TIMEOUT 500U

// Macros --------------------------------------------------------------------

#define SELECT_SD() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET)

#define DISELECT_SD() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET)

#define GET_VERSION_FROM_R7(r7) \
  (((r7).command_version_plus_reserved & 0xf0) >> 4)

#define GET_VOLTAGE_FROM_R7(r7) \
  ((r7).voltage_accepted_plus_reserved & 0x0f)

#define SEND_CMD(p_hspi, cmd, response, status) \
  SELECT_SD(); \
  (status) |= HAL_SPI_Transmit((p_hspi), (uint8_t*)&(cmd), 6, HAL_MAX_DELAY); \
  (status) |= sd_card_receive_cmd_response( \
    (p_hspi), (uint8_t*)&(response), sizeof(response) \
  ); \
  DISELECT_SD()

#define IS_PARTIAL_BLOCK_ALLOWED(csd) \
  (bool)((csd)[6] & 0x80)

// Structs -------------------------------------------------------------------

typedef enum 
{
  SD_OK = 0x00U,
  SD_ERROR = 0x01U,
  SD_BUSY = 0x02U,
  SD_TIMEOUT = 0x03U,
  SD_UNUSABLE_CARD = 0x04U,
  SD_CRC_ERROR = 0x05U,
  SD_INCORRECT_ARGUMENT = 0x06U,
  SD_TRANSMISSION_ERROR = 0X07U // r1 error
} sd_error;

typedef struct 
{
  uint8_t start_block; // '0' + '1' + command index (6 bits)
  uint8_t argument[4];
  uint8_t crc_block; // CRC7 + '1'
} sd_command;

typedef enum
{
  SD_DATA_RESPONSE_ACCEPTED = 0x5U,
  SD_DATA_RESPONSE_CRC_ERROR = 0xBU,
  SD_DATA_RESPONSE_WRITE_ERROR = 0xDU
} sd_data_response_mask;
	
typedef enum 
{
  R1_CLEAR_FLAGS = 0x0U,
  R1_IN_IDLE_STATE = 0x1U,
  R1_ERASE_RESET = 0x2U,
  R1_ILLEGAL_COMMAND = 0x4U,
  R1_COM_CRC_ERROR = 0x8U,
  R1_ERASE_SEQUENCE_ERROR = 0x10U,
  R1_ADDRESS_ERROR = 0x20U,
  R1_PARAMETER_ERROR = 0x40U
} sd_r1_error_mask;

typedef struct 
{
  sd_r1_response high_order_part;
  uint8_t card_status;
} sd_r2_response;

typedef struct 
{
  sd_r1_response high_order_part;
  uint8_t ocr_register_content[4];
} sd_r3_response;

typedef struct 
{
  sd_r1_response high_order_part;
  uint8_t command_version_plus_reserved;
  uint8_t reserved_bits;
  uint8_t voltage_accepted_plus_reserved;
  uint8_t echo_back_of_check_pattern;
} sd_r7_response;

typedef enum 
{
  UNDEFINED = 0x0U,
  STANDART,
  HIGH_OR_EXTENDED
} sd_capacity;

typedef struct 
{
  uint8_t version;
  sd_capacity capacity;
  bool error_in_initialization;
} sd_status;

// Functions -----------------------------------------------------------------

sd_error sd_card_receive_byte(
  SPI_HandleTypeDef *const hspi, 
  uint8_t* data
);

sd_error sd_card_receive_bytes(
  SPI_HandleTypeDef *const hspi,
  uint8_t* data,
  const uint16_t size
);

sd_error sd_card_transmit_byte(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data
);

sd_error sd_card_transmit_bytes(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data,
  const uint16_t size
);

sd_command sd_card_get_cmd_without_crc(
  const uint8_t cmd_num, const uint32_t arg
);

sd_command sd_card_get_cmd(const uint8_t cmd_num, const uint32_t arg);

sd_error sd_card_receive_data_block(
  SPI_HandleTypeDef *const hspi,
  uint8_t* data,
  const uint16_t data_size
);

// Waits for a value other than idle and writes it to received_value
sd_error sd_card_wait_response(
  SPI_HandleTypeDef *const hspi,
  uint8_t* received_value,
  const uint8_t idle_value
);

sd_error sd_card_receive_cmd_response(
	SPI_HandleTypeDef *const hspi, 
	uint8_t* response, 
	const uint8_t response_size
);

//bool is_partial_block_possible(SPI_HandleTypeDef *const hspi);

// CSD takes 16 bytes
sd_error sd_card_get_csd(
  SPI_HandleTypeDef *const hspi,
  uint8_t *const csd
);

sd_error sd_card_set_block_len(
  SPI_HandleTypeDef *const hspi,
  const uint32_t length
);

#endif