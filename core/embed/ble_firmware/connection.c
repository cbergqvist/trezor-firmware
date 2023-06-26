
#include "ble_gap.h"

#include "connection.h"

static uint16_t m_conn_handle =
    BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

void set_connection_handle(uint16_t val) { m_conn_handle = val; }
uint16_t get_connection_handle(void) { return m_conn_handle; }

void disconnect(void) {
  if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
    sd_ble_gap_disconnect(m_conn_handle,
                          BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
  }
}
