/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include STM32_HAL_H

#include "common.h"
#include "display.h"
#include "flash.h"
#include "mini_printf.h"
#include "random_delays.h"
#include "rng.h"
#include "sbu.h"
#include "sdcard.h"
#include "secbool.h"
#include "touch/touch.h"
#include "usb.h"

#include "memzero.h"

#define BUFSIZE 32768

enum { VCP_IFACE = 0x00 };

// static void vcp_intr(void) {
//   display_clear();
//   ensure(secfalse, "vcp_intr");
// }

static void vcp_readpacket() {
  static uint8_t cmdbuf[4];
  static uint8_t buf[BUFSIZE];
  int r = usb_vcp_read_blocking(VCP_IFACE, cmdbuf, 4, -1);
  if (r != 4) {
    return;
  }
  uint16_t len = *(uint16_t*)(cmdbuf + 2);
  ensure(sectrue * (len <= sizeof(buf)), "vcp_readpacket: len too big");
  bool read = cmdbuf[0];
  uint8_t cmd = cmdbuf[1];

  if (cmd > 0) {
    CMD(cmd);
  }
  if (read) {
    for (int i = 0; i < len; i++) {
      buf[i] = *DISPLAY_DATA_ADDRESS;
    }
    r = usb_vcp_write_blocking(VCP_IFACE, buf, len, -1);
    ensure(sectrue * (r == len), "vcp_readpacket: read data");
  } else {
    size_t idx = 0;
    while (idx < len) {
      r = usb_vcp_read_blocking(VCP_IFACE, buf + idx, len - idx, -1);
      idx += r;
    }
    for (int i = 0; i < len; i++) {
      DATA(buf[i]);
    }
  }
}

static void usb_init_all(void) {
  enum {
    VCP_PACKET_LEN = 64,
    VCP_BUFFER_LEN = BUFSIZE,
  };

  static const usb_dev_info_t dev_info = {
      .device_class = 0xEF,     // Composite Device Class
      .device_subclass = 0x02,  // Common Class
      .device_protocol = 0x01,  // Interface Association Descriptor
      .vendor_id = 0x1209,
      .product_id = 0x53C1,
      .release_num = 0x0400,
      .manufacturer = "SatoshiLabs",
      .product = "TREZOR",
      .serial_number = "000000000000",
      .interface = "TREZOR Interface",
      .usb21_enabled = secfalse,
      .usb21_landing = secfalse,
  };

  static uint8_t tx_packet[VCP_PACKET_LEN];
  static uint8_t tx_buffer[VCP_BUFFER_LEN];
  static uint8_t rx_packet[VCP_PACKET_LEN];
  static uint8_t rx_buffer[VCP_BUFFER_LEN];

  static const usb_vcp_info_t vcp_info = {
      .tx_packet = tx_packet,
      .tx_buffer = tx_buffer,
      .rx_packet = rx_packet,
      .rx_buffer = rx_buffer,
      .tx_buffer_len = VCP_BUFFER_LEN,
      .rx_buffer_len = VCP_BUFFER_LEN,
      // .rx_intr_fn = vcp_intr,
      // .rx_intr_byte = 3,  // Ctrl-C
      .iface_num = VCP_IFACE,
      .data_iface_num = 0x01,
      .ep_cmd = 0x82,
      .ep_in = 0x81,
      .ep_out = 0x01,
      .polling_interval = 10,
      .max_packet_len = VCP_PACKET_LEN,
  };

  usb_init(&dev_info);
  ensure(usb_vcp_add(&vcp_info), "usb_vcp_add");
  usb_start();
}

static void draw_border(int width, int padding) {
  const int W = width, P = padding, RX = DISPLAY_RESX, RY = DISPLAY_RESY;
  display_clear();
  display_bar(P, P, RX - 2 * P, RY - 2 * P, 0xFFFF);
  display_bar(P + W, P + W, RX - 2 * (P + W), RY - 2 * (P + W), 0x0000);
  display_refresh();
}

#define BACKLIGHT_NORMAL 150

int main(void) {
  display_orientation(0);
  random_delays_init();
  sdcard_init();
  touch_init();
  sbu_init();
  usb_init_all();

  display_reinit();
  display_clear();
  draw_border(1, 3);

  char dom[32];
  // format: TREZOR2-YYMMDD
  if (sectrue == flash_otp_read(FLASH_OTP_BLOCK_BATCH, 0, (uint8_t *)dom, 32) &&
      0 == memcmp(dom, "TREZOR2-", 8) && dom[31] == 0) {
    display_qrcode(DISPLAY_RESX / 2, DISPLAY_RESY / 2, dom, 4);
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 30, dom + 8, -1,
                        FONT_BOLD, COLOR_WHITE, COLOR_BLACK);
  }

  display_fade(0, BACKLIGHT_NORMAL, 1000);

  for (;;) { vcp_readpacket(); }

  // for (;;) {
  //   vcp_readline(line, sizeof(line));

  //   if (startswith(line, "PING")) {
  //     vcp_printf("OK");

  //   } else if (startswith(line, "BORDER")) {
  //     test_border();

  //   } else if (startswith(line, "DISP ")) {
  //     test_display(line + 5);

  //   } else if (startswith(line, "TOUCH ")) {
  //     test_touch(line + 6);

  //   } else if (startswith(line, "SENS ")) {
  //     test_sensitivity(line + 5);

  //   } else if (startswith(line, "PWM ")) {
  //     test_pwm(line + 4);

  //   } else if (startswith(line, "SD")) {
  //     test_sd();

  //   } else if (startswith(line, "SBU ")) {
  //     test_sbu(line + 4);

  //   } else if (startswith(line, "OTP READ")) {
  //     test_otp_read();

  //   } else if (startswith(line, "OTP WRITE ")) {
  //     test_otp_write(line + 10);

  //   } else if (startswith(line, "WIPE")) {
  //     test_wipe();

  //   } else {
  //     vcp_printf("UNKNOWN");
  //   }
  // }

  return 0;
}
