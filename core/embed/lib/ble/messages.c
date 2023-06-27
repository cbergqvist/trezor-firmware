
#include <stdint.h>

#include "ble.h"
#include "common.h"
#include "int_comm_defs.h"
#include "messages.h"

void send_state_request(void) {
  uint8_t cmd = INTERNAL_CMD_SEND_STATE;
  ble_int_comm_send(&cmd, sizeof(cmd), INTERNAL_EVENT);
}

void send_advertising_on(bool whitelist) {
  uint8_t data[2];
  data[0] = INTERNAL_CMD_ADVERTISING_ON;
  data[1] = whitelist ? 1 : 0;
  ble_int_comm_send(data, sizeof(data), INTERNAL_EVENT);
}

void send_advertising_off(void) {
  uint8_t cmd = INTERNAL_CMD_ADVERTISING_OFF;
  ble_int_comm_send(&cmd, sizeof(cmd), INTERNAL_EVENT);
}

bool send_erase_bonds(void) {
  if (!ble_firmware_running()) {
    return false;
  }
  uint8_t cmd = INTERNAL_CMD_ERASE_BONDS;
  ble_int_comm_send(&cmd, sizeof(cmd), INTERNAL_EVENT);

  uint8_t buf[64] = {0};

  uint32_t ticks_start = hal_ticks_ms();
  int len = 0;

  while (len == 0) {
    len = ble_int_event_receive(buf, sizeof(buf));

    if (hal_ticks_ms() - ticks_start > 1000) {
      // timeout
      return false;
    }
  }

  if (buf[0] == INTERNAL_EVENT_SUCCESS) {
    return true;
  }

  return false;
}

bool send_disconnect(void) {
  if (!ble_firmware_running()) {
    return false;
  }
  uint8_t cmd = INTERNAL_CMD_DISCONNECT;
  ble_int_comm_send(&cmd, sizeof(cmd), INTERNAL_EVENT);

  uint8_t buf[64] = {0};

  uint32_t ticks_start = hal_ticks_ms();
  int len = 0;

  while (len == 0) {
    len = ble_int_event_receive(buf, sizeof(buf));

    if (hal_ticks_ms() - ticks_start > 1000) {
      // timeout
      return false;
    }
  }

  if (buf[0] == INTERNAL_EVENT_SUCCESS) {
    return true;
  }

  return false;
}
