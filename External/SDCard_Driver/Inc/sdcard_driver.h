#ifndef SDCARD_DRIVER_H
#define SDCARD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h" // If you don't connect it, there will be HAL errors
#include "stm32f1xx_hal_spi.h"

// Defines -------------------------------------------------------------------

#define sd_card_r1_response uint8_t // 0xxx.xxxx (MSB first)

#define sd_card_r1b_response uint8_t // busy token. 0xxx.xxxx (MSB first)

#define SD_TRANSMISSION_TIMEOUT 500U

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
    (hspi), (uint8_t*)&(response), sizeof(response) \
  ); \
  DISELECT_SD();

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
} sd_card_error;

typedef struct 
{
  uint8_t start_block; // '0' + '1' + command index (6 bits)
  uint8_t argument[4];
  uint8_t crc_block; // CRC7 + '1'
} sd_card_command;
	
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
} r1_error_mask;

typedef struct 
{
  sd_card_r1_response high_order_part;
  uint8_t card_status;
} sd_card_r2_response;

typedef struct 
{
  sd_card_r1_response high_order_part;
  uint8_t ocr_register_content[4];
} sd_card_r3_response;

typedef struct 
{
  sd_card_r1_response high_order_part;
  uint8_t command_version_plus_reserved;
  uint8_t reserved_bits;
  uint8_t voltage_accepted_plus_reserved;
  uint8_t echo_back_of_check_pattern;
} sd_card_r7_response;

typedef enum 
{
  UNDEFINED = 0X0,
  STANDART,
  HIGH_OR_EXTENDED
} sd_card_capacity;

typedef struct 
{
  uint8_t version;
  sd_card_capacity capacity;
  bool error_in_initialization;
} sd_card_status;

// Variables -----------------------------------------------------------------

extern sd_card_status sd_status;

// Functions -----------------------------------------------------------------

sd_card_error sd_card_reset(SPI_HandleTypeDef *hspi, bool crc_enable);

sd_card_command sd_card_get_cmd_without_crc(uint8_t cmd_num, uint32_t arg);

sd_card_command sd_card_get_cmd(uint8_t cmd_num, uint32_t arg);

// Waits for a value other than idle and writes it to received_value
sd_card_error sd_card_wait_response(
  SPI_HandleTypeDef *hspi,
  uint8_t* received_value,
  const uint8_t idle_value
);

sd_card_error sd_card_receive_cmd_response(
  SPI_HandleTypeDef *hspi, 
  uint8_t* response, 
  uint8_t response_size
);

// Received_size - user data size!
sd_card_error sd_card_receive_data_block(
  SPI_HandleTypeDef *hspi,
  uint8_t* data,
  const uint16_t data_size
);

sd_card_error sd_card_set_block_len(
  SPI_HandleTypeDef *hspi,
  uint32_t length
);

// SDSC uses byte unit address and SDHC and SDXC Cards use
// block unit address (512 bytes unit)
sd_card_error sd_card_read_data(
  SPI_HandleTypeDef *hspi,
  const uint32_t address,
  uint8_t* data,
  const uint32_t block_length
);

sd_card_error sd_card_read_multiple_data(
  SPI_HandleTypeDef *hspi, 
  const uint32_t address,
  uint8_t* data,
  const uint32_t block_length,
  const uint32_t number_of_blocks
);

sd_card_error sd_card_read_write(SPI_HandleTypeDef *hspi);

#endif
