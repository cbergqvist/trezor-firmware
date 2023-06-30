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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "flash.h"
#include "norcow_config.h"

#define FLASH_SECTOR_COUNT 24

const flash_area_t STORAGE_AREAS[STORAGE_AREAS_COUNT] = {
    {
        .num_subareas = 1,
        .subarea[0] =
            {
                .first_sector = 4,
                .num_sectors = 1,
            },
    },
    {
        .num_subareas = 1,
        .subarea[0] =
            {
                .first_sector = 16,
                .num_sectors = 1,
            },
    },
};

static const uint32_t FLASH_START = 0x08000000;
static const uint32_t FLASH_END = 0x08200000;
static const uint32_t FLASH_SECTOR_TABLE[FLASH_SECTOR_COUNT + 1] = {
    [0] = FLASH_START,  // - 0x08003FFF |  16 KiB
    [1] = 0x08004000,   // - 0x08007FFF |  16 KiB
    [2] = 0x08008000,   // - 0x0800BFFF |  16 KiB
    [3] = 0x0800C000,   // - 0x0800FFFF |  16 KiB
    [4] = 0x08010000,   // - 0x0801FFFF |  64 KiB
    [5] = 0x08020000,   // - 0x0803FFFF | 128 KiB
    [6] = 0x08040000,   // - 0x0805FFFF | 128 KiB
    [7] = 0x08060000,   // - 0x0807FFFF | 128 KiB
    [8] = 0x08080000,   // - 0x0809FFFF | 128 KiB
    [9] = 0x080A0000,   // - 0x080BFFFF | 128 KiB
    [10] = 0x080C0000,  // - 0x080DFFFF | 128 KiB
    [11] = 0x080E0000,  // - 0x080FFFFF | 128 KiB
    [12] = 0x08100000,  // - 0x08103FFF |  16 KiB
    [13] = 0x08104000,  // - 0x08107FFF |  16 KiB
    [14] = 0x08108000,  // - 0x0810BFFF |  16 KiB
    [15] = 0x0810C000,  // - 0x0810FFFF |  16 KiB
    [16] = 0x08110000,  // - 0x0811FFFF |  64 KiB
    [17] = 0x08120000,  // - 0x0813FFFF | 128 KiB
    [18] = 0x08140000,  // - 0x0815FFFF | 128 KiB
    [19] = 0x08160000,  // - 0x0817FFFF | 128 KiB
    [20] = 0x08180000,  // - 0x0819FFFF | 128 KiB
    [21] = 0x081A0000,  // - 0x081BFFFF | 128 KiB
    [22] = 0x081C0000,  // - 0x081DFFFF | 128 KiB
    [23] = 0x081E0000,  // - 0x081FFFFF | 128 KiB
    [24] = FLASH_END,   // last element - not a valid sector
};
const uint32_t FLASH_SIZE = FLASH_END - FLASH_START;
uint8_t *FLASH_BUFFER = NULL;

secbool flash_unlock_write(void) { return sectrue; }

secbool flash_lock_write(void) { return sectrue; }

uint32_t flash_sector_size(uint16_t sector) {
  if (sector >= FLASH_SECTOR_COUNT) {
    return 0;
  }
  return FLASH_SECTOR_TABLE[sector + 1] - FLASH_SECTOR_TABLE[sector];
}

const void *flash_get_address(uint16_t sector, uint32_t offset, uint32_t size) {
  if (sector >= FLASH_SECTOR_COUNT) {
    return NULL;
  }
  const uint32_t addr = FLASH_SECTOR_TABLE[sector] + offset;
  const uint32_t next = FLASH_SECTOR_TABLE[sector + 1];
  if (addr + size > next) {
    return NULL;
  }
  return FLASH_BUFFER + addr - FLASH_SECTOR_TABLE[0];
}

uint32_t flash_subarea_get_size(const flash_subarea_t *subarea) {
  uint32_t size = 0;
  for (int s = 0; s < subarea->num_sectors; s++) {
    size += flash_sector_size(subarea->first_sector + s);
  }
  return size;
}

uint32_t flash_area_get_size(const flash_area_t *area) {
  uint32_t size = 0;
  for (int i = 0; i < area->num_subareas; i++) {
    size += flash_subarea_get_size(&area->subarea[i]);
  }
  return size;
}

const void *flash_area_get_address(const flash_area_t *area, uint32_t offset,
                                   uint32_t size) {
  uint32_t tmp_offset = offset;

  for (int i = 0; i < area->num_subareas; i++) {
    uint16_t sector = area->subarea[i].first_sector;

    uint32_t sub_size = flash_subarea_get_size(&area->subarea[i]);
    if (tmp_offset >= sub_size) {
      tmp_offset -= sub_size;
      continue;
    }

    // in correct subarea

    for (int s = 0; s < area->subarea[i].num_sectors; s++) {
      const uint32_t sector_size = flash_sector_size(sector);
      if (tmp_offset >= sector_size) {
        tmp_offset -= sector_size;
        sector++;

        if (s == area->subarea[i].num_sectors - 1) {
          return NULL;
        }
        continue;
      }
      // in correct sector
      break;
    }

    const void *addr = flash_get_address(sector, tmp_offset, size);
    const void *area_end = flash_get_address(sector + 1, 0, 0);
    if ((size_t)addr + size > (size_t)area_end) {
      return NULL;
    }
    return addr;
  }
  return NULL;
}

secbool flash_erase_sectors(const uint8_t *sectors, int len,
                            void (*progress)(int pos, int len)) {
  if (progress) {
    progress(0, len);
  }
  for (int i = 0; i < len; i++) {
    const uint8_t sector = sectors[i];
    const uint32_t offset = FLASH_SECTOR_TABLE[sector] - FLASH_SECTOR_TABLE[0];
    const uint32_t size =
        FLASH_SECTOR_TABLE[sector + 1] - FLASH_SECTOR_TABLE[sector];
    memset(FLASH_BUFFER + offset, 0xFF, size);
    if (progress) {
      progress(i + 1, len);
    }
  }
  return sectrue;
}

secbool flash_area_erase(const flash_area_t *area,
                         void (*progress)(int pos, int len)) {
  ensure(flash_unlock_write(), NULL);

  int total_sectors = 0;
  int done_sectors = 0;
  for (int i = 0; i < area->num_subareas; i++) {
    total_sectors += area->subarea[i].num_sectors;
  }

  if (progress) {
    progress(0, total_sectors);
  }

  for (int s = 0; s < area->num_subareas; s++) {
    for (int i = 0; i < area->subarea[s].num_sectors; i++) {
      int sector = area->subarea[s].first_sector + i;

      const uint32_t offset =
          FLASH_SECTOR_TABLE[sector] - FLASH_SECTOR_TABLE[0];
      const uint32_t size =
          FLASH_SECTOR_TABLE[sector + 1] - FLASH_SECTOR_TABLE[sector];
      memset(FLASH_BUFFER + offset, 0xFF, size);

      done_sectors++;
      if (progress) {
        progress(done_sectors, total_sectors);
      }
    }
  }
  ensure(flash_lock_write(), NULL);
  return sectrue;
}

secbool flash_write_byte(uint8_t sector, uint32_t offset, uint8_t data) {
  uint8_t *flash = (uint8_t *)flash_get_address(sector, offset, 1);
  if (!flash) {
    return secfalse;
  }
  if ((flash[0] & data) != data) {
    return secfalse;  // we cannot change zeroes to ones
  }
  flash[0] = data;
  return sectrue;
}

secbool flash_write_word(uint8_t sector, uint32_t offset, uint32_t data) {
  if (offset % 4) {  // we write only at 4-byte boundary
    return secfalse;
  }
  uint32_t *flash = (uint32_t *)flash_get_address(sector, offset, sizeof(data));
  if (!flash) {
    return secfalse;
  }
  if ((flash[0] & data) != data) {
    return secfalse;  // we cannot change zeroes to ones
  }
  flash[0] = data;
  return sectrue;
}

secbool flash_area_write_byte(const flash_area_t *area, uint32_t offset,
                              uint8_t data) {
  uint32_t tmp_offset = offset;
  for (int i = 0; i < area->num_subareas; i++) {
    uint16_t sector = area->subarea[i].first_sector;

    uint32_t sub_size = flash_subarea_get_size(&area->subarea[i]);
    if (tmp_offset >= sub_size) {
      tmp_offset -= sub_size;
      continue;
    }

    // in correct subarea
    for (int s = 0; s < area->subarea[i].num_sectors; s++) {
      const uint32_t sector_size = flash_sector_size(sector);
      if (tmp_offset >= sector_size) {
        tmp_offset -= sector_size;
        sector++;

        if (s == area->subarea[i].num_sectors - 1) {
          return secfalse;
        }
        continue;
      }
      // in correct sector
      return flash_write_byte(sector, tmp_offset, data);
    }
  }
  return secfalse;
}

secbool flash_area_write_word(const flash_area_t *area, uint32_t offset,
                              uint32_t data) {
  uint32_t tmp_offset = offset;
  for (int i = 0; i < area->num_subareas; i++) {
    uint16_t sector = area->subarea[i].first_sector;

    uint32_t sub_size = flash_subarea_get_size(&area->subarea[i]);
    if (tmp_offset >= sub_size) {
      tmp_offset -= sub_size;
      continue;
    }

    // in correct subarea
    for (int s = 0; s < area->subarea[i].num_sectors; s++) {
      const uint32_t sector_size = flash_sector_size(sector);
      if (tmp_offset >= sector_size) {
        tmp_offset -= sector_size;
        sector++;

        if (s == area->subarea[i].num_sectors - 1) {
          return secfalse;
        }
        continue;
      }
      // in correct sector
      return flash_write_word(sector, tmp_offset, data);
    }
  }
  return secfalse;
}
