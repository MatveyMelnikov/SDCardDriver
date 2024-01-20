#include "sd_driver_write.h"
#include "crc-buffer.h"

// Static functions ----------------------------------------------------------

static sd_error sd_card_transmit_data_block(
  SPI_HandleTypeDef *const hspi,
  const uint8_t *const data,
  const uint16_t data_size,
  const uint8_t start_token
)
{
  crc_buffer_16 crc_buffer = { 0 };
  uint8_t data_response = 0x0;
  uint8_t busy_signal = 0;

  crc_16_result crc_result = crc_buffer_calculate_crc_16(
    &crc_buffer, data, data_size
  );

  sd_error status = sd_card_transmit_byte(hspi, &start_token);
  status |= sd_card_transmit_bytes(hspi, data, data_size);
  status |= sd_card_transmit_bytes(
    hspi, (uint8_t*)&crc_result, sizeof(crc_16_result)
  );

  status |= sd_card_receive_byte(hspi, &data_response);
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

  switch (data_response & 0xf)
  {
    case SD_DATA_RESPONSE_CRC_ERROR:
      status = SD_CRC_ERROR;
      break;
    case SD_DATA_RESPONSE_WRITE_ERROR:
      status = SD_ERROR;
      break;
    case SD_DATA_RESPONSE_ACCEPTED:
      break;
    default:
      status |= SD_TRANSMISSION_ERROR;
  }

  return status;
}

// Implementations -----------------------------------------------------------

sd_error sd_card_write_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  const uint8_t *const data,
  const uint32_t block_length
)
{
  sd_command cmd24 = sd_card_get_cmd(24, address);
  sd_r1_response r1 = { 0 };

  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd24, sizeof(cmd24), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_write;

  // 0xfe - start token of single block write
  status |= sd_card_transmit_data_block(
    hspi, data, block_length, 0xfe
  );

end_write:
  DISELECT_SD();
  return status;
}

sd_error sd_card_write_multiple_data(
  SPI_HandleTypeDef *const hspi,
  const uint32_t address,
  const uint8_t *const data,
  const uint32_t block_length,
  const uint32_t number_of_blocks
)
{
  sd_command cmd25 = sd_card_get_cmd(25, address);
  sd_r1_response r1 = { 0 };
  uint8_t stop_token = 0xfd;
  uint8_t busy_signal = 0;

  SELECT_SD();
  sd_error status = HAL_SPI_Transmit(
    hspi, (uint8_t*)&cmd25, sizeof(cmd25), SD_TRANSMISSION_TIMEOUT
  );
  status |= sd_card_receive_cmd_response(hspi, &r1, 1);

  if (r1)
    status = SD_TRANSMISSION_ERROR;
  if (status)
    goto end_write;

  for (uint32_t i = 0; i < number_of_blocks; i++)
  {
    // 0xfc - start token of multiple block write
    status |= sd_card_transmit_data_block(
      hspi, data + (i * block_length), block_length, 0xfc
    );
  }

  if (status)
    goto end_write;

  status |= sd_card_transmit_byte(hspi, &stop_token);
  // The busy signal does not appear immediately. This is not
  // described in the documentation
  status |= sd_card_wait_response(hspi, &busy_signal, 0xff);
  status |= sd_card_wait_response(hspi, &busy_signal, 0x0);

end_write:
  DISELECT_SD();
  return status;
}
