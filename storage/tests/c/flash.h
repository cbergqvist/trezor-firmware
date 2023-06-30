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

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>
#include <stdlib.h>
#include "secbool.h"

#define STORAGE_AREAS_COUNT 2
#define FLASH_BYTE_ACCESS 1

typedef struct {
  uint16_t first_sector;
  uint16_t num_sectors;
} flash_subarea_t;

typedef struct {
  flash_subarea_t subarea[4];
  uint8_t num_subareas;
} flash_area_t;

extern const flash_area_t STORAGE_AREAS[STORAGE_AREAS_COUNT];

void flash_init(void);

secbool __wur flash_unlock_write(void);
secbool __wur flash_lock_write(void);

const void *flash_get_address(uint16_t sector, uint32_t offset, uint32_t size);
uint32_t flash_sector_size(uint16_t sector);
uint16_t flash_total_sectors(const flash_area_t *area);
int flash_get_sector_num(const flash_area_t *area, uint32_t sector_inner_num);

const void *flash_area_get_address(const flash_area_t *area, uint32_t offset,
                                   uint32_t size);
uint32_t flash_area_get_size(const flash_area_t *area);

secbool __wur flash_area_erase(const flash_area_t *area,
                               void (*progress)(int pos, int len));
secbool __wur flash_area_erase_bulk(const flash_area_t *area, int count,
                                    void (*progress)(int pos, int len));

#if defined FLASH_BYTE_ACCESS
secbool __wur flash_area_write_byte(const flash_area_t *area, uint32_t offset,
                                    uint8_t data);
secbool __wur flash_area_write_word(const flash_area_t *area, uint32_t offset,
                                    uint32_t data);
#endif
secbool __wur flash_area_write_quadword(const flash_area_t *area,
                                        uint32_t offset, const uint32_t *data);

secbool __wur flash_otp_read(uint8_t block, uint8_t offset, uint8_t *data,
                             uint8_t datalen);
secbool __wur flash_otp_write(uint8_t block, uint8_t offset,
                              const uint8_t *data, uint8_t datalen);
secbool __wur flash_otp_lock(uint8_t block);
secbool __wur flash_otp_is_locked(uint8_t block);

#endif
