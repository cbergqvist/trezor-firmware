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

#include <libopencm3/stm32/flash.h>
#include <string.h>

#include "common.h"
#include "flash.h"
#include "memory.h"
#include "supervise.h"

#define STORAGE_AREAS_COUNT 2

static const uint32_t FLASH_SECTOR_TABLE[FLASH_SECTOR_COUNT + 1] = {
    [0] = 0x08000000,   // - 0x08003FFF |  16 KiB
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
    [12] = 0x08100000,  // last element - not a valid sector
};

const flash_area_t STORAGE_AREAS[STORAGE_AREAS_COUNT] = {
    {
        .num_subareas = 1,
        .subarea[0] =
            {
                .first_sector = 2,
                .num_sectors = 1,
            },
    },
    {
        .num_subareas = 1,
        .subarea[0] =
            {
                .first_sector = 3,
                .num_sectors = 1,
            },
    },
};

static secbool flash_check_success(uint32_t status) {
  return (status & (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR |
                    FLASH_SR_WRPERR))
             ? secfalse
             : sectrue;
}

secbool flash_unlock_write(void) {
  svc_flash_unlock();
  return sectrue;
}

secbool flash_lock_write(void) { return flash_check_success(svc_flash_lock()); }

const void *flash_get_address(uint8_t sector, uint32_t offset, uint32_t size) {
  if (sector >= FLASH_SECTOR_COUNT) {
    return NULL;
  }
  const uint32_t addr = FLASH_SECTOR_TABLE[sector] + offset;
  const uint32_t next = FLASH_SECTOR_TABLE[sector + 1];
  if (addr + size > next) {
    return NULL;
  }
  return (const void *)FLASH_PTR(addr);
}

uint32_t flash_sector_size(uint8_t sector) {
  if (sector >= FLASH_SECTOR_COUNT) {
    return 0;
  }
  return FLASH_SECTOR_TABLE[sector + 1] - FLASH_SECTOR_TABLE[sector];
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

secbool flash_erase(uint8_t sector) {
  ensure(flash_unlock_write(), NULL);
  svc_flash_erase_sector(sector);
  ensure(flash_lock_write(), NULL);

  // Check whether the sector was really deleted (contains only 0xFF).
  const uint32_t addr_start = FLASH_SECTOR_TABLE[sector],
                 addr_end = FLASH_SECTOR_TABLE[sector + 1];
  for (uint32_t addr = addr_start; addr < addr_end; addr += 4) {
    if (*((const uint32_t *)FLASH_PTR(addr)) != 0xFFFFFFFF) {
      return secfalse;
    }
  }
  return sectrue;
}

secbool flash_write_byte(uint8_t sector, uint32_t offset, uint8_t data) {
  uint8_t *address = (uint8_t *)flash_get_address(sector, offset, 1);
  if (address == NULL) {
    return secfalse;
  }

  if ((*address & data) != data) {
    return secfalse;
  }

  svc_flash_program(FLASH_CR_PROGRAM_X8);
  *(volatile uint8_t *)address = data;

  if (*address != data) {
    return secfalse;
  }

  return sectrue;
}

secbool flash_write_word(uint8_t sector, uint32_t offset, uint32_t data) {
  uint32_t *address = (uint32_t *)flash_get_address(sector, offset, 4);
  if (address == NULL) {
    return secfalse;
  }

  if (offset % 4 != 0) {
    return secfalse;
  }

  if ((*address & data) != data) {
    return secfalse;
  }

  svc_flash_program(FLASH_CR_PROGRAM_X32);
  *(volatile uint32_t *)address = data;

  if (*address != data) {
    return secfalse;
  }

  return sectrue;
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

    uint32_t addr = FLASH_SECTOR_TABLE[sector] + offset;
    uint32_t area_end = FLASH_SECTOR_TABLE[sector + 1];
    if ((uint32_t)addr + size > (uint32_t)area_end) {
      return NULL;
    }
    return (const void *)FLASH_PTR(addr);
  }
  return NULL;
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
      svc_flash_erase_sector(sector);
      // check whether the sector was really deleted (contains only 0xFF)
      const uint32_t addr_start = FLASH_SECTOR_TABLE[sector],
                     addr_end = FLASH_SECTOR_TABLE[sector + 1];
      for (uint32_t addr = addr_start; addr < addr_end; addr += 4) {
        if (*((const uint32_t *)FLASH_PTR(addr)) != 0xFFFFFFFF) {
          ensure(flash_lock_write(), NULL);
          return secfalse;
        }
      }
      done_sectors++;
      if (progress) {
        progress(done_sectors, total_sectors);
      }
    }
  }
  ensure(flash_lock_write(), NULL);
  return sectrue;
}

secbool flash_area_erase_bulk(const flash_area_t *area, int count,
                              void (*progress)(int pos, int len)) {
  ensure(flash_unlock_write(), NULL);

  int total_sectors = 0;
  int done_sectors = 0;
  for (int a = 0; a < count; a++) {
    for (int i = 0; i < area[a].num_subareas; i++) {
      total_sectors += area[a].subarea[i].num_sectors;
    }
  }
  if (progress) {
    progress(0, total_sectors);
  }

  for (int a = 0; a < count; a++) {
    for (int s = 0; s < area[a].num_subareas; s++) {
      for (int i = 0; i < area[a].subarea[s].num_sectors; i++) {
        int sector = area[a].subarea[s].first_sector + i;
        svc_flash_erase_sector(sector);
        // check whether the sector was really deleted (contains only 0xFF)
        const uint32_t addr_start = FLASH_SECTOR_TABLE[sector],
                       addr_end = FLASH_SECTOR_TABLE[sector + 1];
        for (uint32_t addr = addr_start; addr < addr_end; addr += 4) {
          if (*((const uint32_t *)FLASH_PTR(addr)) != 0xFFFFFFFF) {
            ensure(flash_lock_write(), NULL);
            return secfalse;
          }
        }
        done_sectors++;
        if (progress) {
          progress(done_sectors, total_sectors);
        }
      }
    }
  }
  ensure(flash_lock_write(), NULL);
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
