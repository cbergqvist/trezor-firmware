#include "optiga_trust_m.h"
#include <string.h>

#include "common.h"
#include "i2c.h"
#include TREZOR_BOARD

#define OPTIGA_ADDRESS \
  (0x30U << 1)  // the HAL requires the 7-bit address to be shifted by one bit

void optiga_init(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructure.Alternate = 0;
  GPIO_InitStructure.Pin = GPIO_PIN_9;

  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
}

secbool check_i2c_status(uint8_t* expected_status) {
  // HAL_StatusTypeDef res;
  uint8_t tx_data[1] = {0x82};
  uint8_t rx_buffer[64] = {0};

  while (1) {
    i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 1, 10);
    i2c_receive(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, rx_buffer, 4, 10);

    //    if (res != HAL_OK) {
    //      return secfalse;
    //    }

    if (rx_buffer[0] == expected_status[0] &&
        rx_buffer[1] == expected_status[1] &&
        rx_buffer[2] == expected_status[2] &&
        rx_buffer[3] == expected_status[3]) {
      return sectrue;
    }
  }

  return secfalse;
}

secbool open_application(void) {
  HAL_StatusTypeDef res = HAL_ERROR;

  uint8_t rx_buffer[64] = {0};
  uint8_t tx_data[] = {0x80, 0x03, 0x00, 0x15, 0x00, 0x70, 0x00, 0x00, 0x10,
                       0xD2, 0x76, 0x00, 0x00, 0x04, 0x47, 0x65, 0x6E, 0x41,
                       0x75, 0x74, 0x68, 0x41, 0x70, 0x70, 0x6C, 0x04, 0x1A};

  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 27, 10);
  }

  uint8_t exp[4] = {0xC8, 0x80, 0x00, 0x05};

  if (sectrue != check_i2c_status(exp)) {
    return secfalse;
  }

  tx_data[0] = 0x80;

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 1, 10);
  }

  hal_delay(1);

  i2c_receive(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, rx_buffer, 5, 10);
  // todo check received data

  uint8_t exp1[4] = {0x48, 0x80, 0x00, 0x0A};
  if (sectrue != check_i2c_status(exp1)) {
    return secfalse;
  }

  tx_data[0] = 0x80;

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 1, 10);
  }

  i2c_receive(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, rx_buffer, 10, 10);

  tx_data[0] = 0x80;
  tx_data[1] = 0x80;
  tx_data[2] = 0x00;
  tx_data[3] = 0x00;
  tx_data[4] = 0x0C;
  tx_data[5] = 0xEC;

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 6, 10);
  }

  return sectrue;
}

int test_optiga_read_UID(uint8_t* buffer) {
  HAL_StatusTypeDef res = HAL_ERROR;

  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);
  hal_delay(10);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);
  hal_delay(10);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);
  hal_delay(10);

  i2c_cycle(OPTIGA_I2C_INSTANCE);

  uint8_t exp0[4] = {0x08, 0x80, 0x00, 0x00};

  check_i2c_status(exp0);

  open_application();

  uint8_t rx_buffer[64] = {0};
  uint8_t tx_data[] = {0x80, 0x04, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x00, 0x06,
                       0xE0, 0xC2, 0x00, 0x00, 0x00, 0x64, 0xF0, 0x9F};

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data,
                       sizeof(tx_data), 10);
  }

  uint8_t exp1[4] = {0xC8, 0x80, 0x00, 0x05};
  if (sectrue != check_i2c_status(exp1)) {
    return secfalse;
  }

  tx_data[0] = 0x80;

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 1, 10);
  }
  hal_delay(1);

  i2c_receive(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, rx_buffer, 0x05, 10);

  hal_delay(10);

  uint8_t exp[4] = {0x48, 0x80, 0x00, 0x25};

  check_i2c_status(exp);

  tx_data[0] = 0x80;

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 1, 10);
  }

  hal_delay(1);
  i2c_receive(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, rx_buffer, 0x25, 10);

  hal_delay(1);

  memcpy(buffer, &rx_buffer[8], 27);

  tx_data[0] = 0x80;
  tx_data[1] = 0x81;
  tx_data[2] = 0x00;
  tx_data[3] = 0x00;
  tx_data[4] = 0x56;
  tx_data[5] = 0x30;

  res = HAL_ERROR;
  while (res != HAL_OK) {
    res = i2c_transmit(OPTIGA_I2C_INSTANCE, OPTIGA_ADDRESS, tx_data, 6, 10);
  }

  return 0;
}
