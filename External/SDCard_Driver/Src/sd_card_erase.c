/*
SD card erase functions
*/

#include "sd_driver_erase.h"

// Implementations -----------------------------------------------------------

sd_error sd_card_set_erasable_area(
  SPI_HandleTypeDef *const hspi,
  const uint32_t start_address,
  const uint32_t end_address
)
{
  sd_command cmd_erase_start_addr = sd_card_get_cmd(32, start_address);
  sd_command cmd_erase_end_addr = sd_card_get_cmd(33, start_address);
  sd_r1_response r1 = { 0 };
  sd_error status = { 0 };

  SEND_CMD(hspi, cmd_erase_start_addr, r1, status);
  SEND_CMD(hspi, cmd_erase_end_addr, r1, status);

  return status;
}

sd_error sd_card_erase(SPI_HandleTypeDef *const hspi)
{
  sd_command cmd_erase = sd_card_get_cmd(38, 0x0);
  sd_r1_response r1b = { 0 };
  sd_error status = { 0 };
  uint8_t busy_signal = 0;

  //SEND_CMD(hspi, cmd_erase_start_addr, r1b, status);
  //status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

  SELECT_SD();
  status |= HAL_SPI_Transmit(
    hspi,
    (uint8_t*)&cmd_erase,
    sizeof(cmd_erase),
    SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1b, 1);  
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);
  SELECT_SD();

  return status;
}
