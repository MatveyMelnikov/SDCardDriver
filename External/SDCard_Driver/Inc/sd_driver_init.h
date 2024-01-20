/*
Initializing SD cards of different versions and sizes
*/

#ifndef SD_DRIVER_INIT_H
#define SD_DRIVER_INIT_H

#include "sd_driver_secondary.h"

// Variables -----------------------------------------------------------------

// Displaying the status of the SD card: its version and size
extern sd_status sd_card_status;

// Structs -------------------------------------------------------------------

typedef struct 
{
  float max_transfer_speed; // MBit/s (MHz). Max data transfer rate 
  // per one data line
  uint16_t command_classes; // The bit number corresponds to the supported class
  uint16_t max_data_block_size; // In bytes
  bool partial_blocks_allowed;
  uint32_t size; // For version 2 - KBytes, for version 1 - bytes
} sd_info;

// Functions -----------------------------------------------------------------

sd_error sd_card_reset(SPI_HandleTypeDef *const hspi, const bool crc_enable);

sd_error sd_card_get_common_info(
  SPI_HandleTypeDef *const hspi, sd_info *const info
);

#endif
