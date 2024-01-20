#ifndef SD_DRIVER_WRITE_H
#define SD_DRIVER_WRITE_H

#include "sd_driver_secondary.h"

// Functions -----------------------------------------------------------------

sd_error sd_card_write_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  const uint8_t *const data,
  const uint32_t block_length
);

sd_error sd_card_write_multiple_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  const uint8_t *const data,
  const uint32_t block_length,
  const uint32_t number_of_blocks
);

#endif
