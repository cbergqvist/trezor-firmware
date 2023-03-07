
#include "int_comm.h"
#include "app_error.h"
#include "app_uart.h"
#include "ble_nus.h"
#include "nrf_log.h"
#include "stdint.h"
#include "trezorhal/ble/int_comm_defs.h"

static uint8_t m_uart_rx_data[BLE_NUS_MAX_DATA_LEN];
static bool m_uart_rx_data_ready_internal = false;
static uint16_t *m_p_conn_handle = NULL;

BLE_NUS_DEF(m_nus,
            NRF_SDH_BLE_TOTAL_LINK_COUNT); /**< BLE NUS service instance. */

void nus_init(uint16_t *p_conn_handle) {
  m_p_conn_handle = p_conn_handle;
  uint32_t err_code;

  ble_nus_init_t nus_init;

  memset(&nus_init, 0, sizeof(nus_init));

  nus_init.data_handler = nus_data_handler;

  err_code = ble_nus_init(&m_nus, &nus_init);
  APP_ERROR_CHECK(err_code);

  *p_conn_handle = BLE_CONN_HANDLE_INVALID;
}

void process_command(uint8_t *data, uint16_t len) {
  uint8_t cmd = data[0];
  switch (cmd) {
    case INTERNAL_CMD_SEND_STATE:
      if (*m_p_conn_handle != BLE_CONN_HANDLE_INVALID) {
        send_connected_event();
      } else {
        send_disconnected_event();
      }
      break;
    default:
      break;
  }
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart
 * module and append it to a string. The string will be be sent over BLE when
 * the last character received was a 'new line' '\n' (hex 0x0A) or if the string
 * has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t *p_event) {
  static uint8_t index = 0;
  static uint8_t message_type = 0;
  static uint16_t len = 0;
  uint32_t err_code;
  uint8_t rx_byte = 0;

  switch (p_event->evt_type) {
    case APP_UART_DATA_READY:
      while (app_uart_get(&rx_byte) == NRF_SUCCESS) {
        if (index == 0) {
          if (rx_byte == INTERNAL_MESSAGE || rx_byte == INTERNAL_EVENT ||
              rx_byte == EXTERNAL_MESSAGE) {
            message_type = rx_byte;
            index += 1;
            continue;
          } else {
            // unknown message
            continue;
          }
        }

        if (index == 1) {
          // len HI
          len = rx_byte << 8;
          index += 1;
          continue;
        }

        if (index == 2) {
          // len LO
          len |= rx_byte;
          index += 1;
          if (len > sizeof(m_uart_rx_data) + OVERHEAD_SIZE) {
            // message too long
            index = 0;
            continue;
          }
          continue;
        }

        if (index < (len - 1)) {
          // command
          m_uart_rx_data[index - COMM_HEADER_SIZE] = rx_byte;
          index += 1;
          continue;
        }

        if (index >= (len - 1)) {
          if (rx_byte == EOM) {
            if (message_type == EXTERNAL_MESSAGE) {
              NRF_LOG_DEBUG("Ready to send data over BLE NUS");
              NRF_LOG_HEXDUMP_DEBUG(m_uart_rx_data, index);

              do {
                uint16_t length = (uint16_t)len - OVERHEAD_SIZE;
                err_code = ble_nus_data_send(&m_nus, m_uart_rx_data, &length,
                                             *m_p_conn_handle);
                if ((err_code != NRF_ERROR_INVALID_STATE) &&
                    (err_code != NRF_ERROR_RESOURCES) &&
                    (err_code != NRF_ERROR_NOT_FOUND)) {
                  APP_ERROR_CHECK(err_code);
                }
              } while (err_code == NRF_ERROR_RESOURCES);
            } else if (message_type == INTERNAL_MESSAGE) {
              m_uart_rx_data_ready_internal = true;
            } else if (message_type == INTERNAL_EVENT) {
              process_command(m_uart_rx_data, len - OVERHEAD_SIZE);
            }
          }
          index = 0;
        }
      }
      break;
    default:
      break;
  }
}
/**@snippet [Handling the data received over UART] */

void send_byte(uint8_t byte) {
  uint32_t err_code;

  do {
    err_code = app_uart_put(byte);
    if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY)) {
      NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
    }
  } while (err_code == NRF_ERROR_BUSY);
}

void send_packet(uint8_t message_type, const uint8_t *tx_data, uint16_t len) {
  uint16_t total_len = len + OVERHEAD_SIZE;
  send_byte(message_type);
  send_byte((total_len >> 8) & 0xFF);
  send_byte(total_len & 0xFF);
  for (uint32_t i = 0; i < len; i++) {
    send_byte(tx_data[i]);
  }
  send_byte(EOM);
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART
 * BLE Service and send it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
void nus_data_handler(ble_nus_evt_t *p_evt) {
  if (p_evt->type == BLE_NUS_EVT_RX_DATA) {
    NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
    NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data,
                          p_evt->params.rx_data.length);

    if (p_evt->params.rx_data.length != 64) {
      return;
    }

    send_packet(EXTERNAL_MESSAGE, p_evt->params.rx_data.p_data,
                p_evt->params.rx_data.length);
  }
}
/**@snippet [Handling the data received over BLE] */

void send_connected_event(void) {
  uint8_t tx_data[] = {
      INTERNAL_EVENT_CONNECTED,
  };
  send_packet(INTERNAL_EVENT, tx_data, sizeof(tx_data));
}

void send_disconnected_event(void) {
  uint8_t tx_data[] = {
      INTERNAL_EVENT_DISCONNECTED,
  };
  send_packet(INTERNAL_EVENT, tx_data, sizeof(tx_data));
}

uint16_t get_message_type(const uint8_t *rx_data) {
  return (rx_data[3] << 8) | rx_data[4];
}

bool send_auth_key_request(uint8_t *p_key, uint8_t p_key_len) {
  uint8_t tx_data[] = {
      0x3F, 0x23, 0x23, 0x1F, 0x43, 0x00, 0x00, 0x00, 0x00,
  };
  send_packet(INTERNAL_MESSAGE, tx_data, sizeof(tx_data));

  while (!m_uart_rx_data_ready_internal)
    ;

  if (get_message_type(m_uart_rx_data) != 8004) {
    m_uart_rx_data_ready_internal = false;
    return false;
  }

  for (int i = 0; i < 6; i++) {
    p_key[i] = m_uart_rx_data[i + 11];
  }
  m_uart_rx_data_ready_internal = false;

  return true;
}

bool send_repair_request(void) {
  uint8_t tx_data[] = {
      0x3F, 0x23, 0x23, 0x1F, 0x45, 0x00, 0x00, 0x00, 0x00,
  };
  send_packet(INTERNAL_MESSAGE, tx_data, sizeof(tx_data));

  while (!m_uart_rx_data_ready_internal)
    ;

  m_uart_rx_data_ready_internal = false;

  if (get_message_type(m_uart_rx_data) != 2) {
    return false;
  }

  return true;
}

void send_initialized(void) {
  uint8_t tx_data[] = {
      INTERNAL_EVENT_INITIALIZED,
  };
  send_packet(INTERNAL_EVENT, tx_data, sizeof(tx_data));
}
