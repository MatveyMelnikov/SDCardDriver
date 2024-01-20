#ifndef SD_DRIVER_INIT_H
#define SD_DRIVER_INIT_H

#include "sd_driver_secondary.h"

// Variables -----------------------------------------------------------------

extern sd_status sd_card_status;

// Functions -----------------------------------------------------------------

sd_error sd_card_reset(SPI_HandleTypeDef *const hspi, const bool crc_enable);

#endif
