/*
SD card erase functions
*/

#ifndef SD_DRIVER_READ_H
#define SD_DRIVER_READ_H

#include "sd_driver_secondary.h"

// Functions -----------------------------------------------------------------

// The address field in the address setting commands is 
// a write block address in byte units
sd_error sd_card_set_erasable_area(
  SPI_HandleTypeDef *const hspi,
  const uint32_t start_address,
  const uint32_t end_address
);

// The data at the card after an erase operation is either '0' or '1', 
// depends on the card vendor
sd_error sd_card_erase(SPI_HandleTypeDef *const hspi);

#endif
