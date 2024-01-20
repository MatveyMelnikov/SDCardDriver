#include "sd_driver_read.h"
#include "crc-buffer.h"

// Implementations -----------------------------------------------------------

sd_error sd_card_read_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  uint8_t* data,
  const uint32_t block_length
)
{
  sd_command cmd17 = sd_card_get_cmd(17, address);
  sd_r1_response r1 = { 0 };

  SELECT_SD();
  HAL_StatusTypeDef status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd17, sizeof(cmd17), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_read;

  status |= sd_card_receive_data_block(
    hspi, data, block_length
  );

end_read:
  DISELECT_SD();
  return status;
}

sd_error sd_card_read_multiple_data(
  SPI_HandleTypeDef *const hspi, 
  const uint32_t address,
  uint8_t* data,
  const uint32_t block_length,
  const uint32_t number_of_blocks
)
{
  sd_command cmd18 = sd_card_get_cmd(18, address);
  sd_command cmd12 = sd_card_get_cmd(12, address);
  sd_r1_response r1 = { 0 };
  uint8_t busy_signal = 0;

  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd18, sizeof(cmd18), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_read;

  for (uint32_t i = 0; i < number_of_blocks; i++)
  {
    status |= sd_card_receive_data_block(
      hspi, data + (i * block_length), block_length
    );
  }

  status |= HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd12, sizeof(cmd12), SD_TRANSMISSION_TIMEOUT
  );

  // Do we always get 0xef in r1?
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);  
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

end_read:
  DISELECT_SD();
  return status;
}
