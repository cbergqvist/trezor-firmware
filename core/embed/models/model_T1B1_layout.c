#include "flash.h"
#include "model.h"

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

const flash_area_t BOOTLOADER_AREA = {
    .num_subareas = 1,
    .subarea[0] =
        {
            .first_sector = 5,
            .num_sectors = 1,
        },
};

const flash_area_t FIRMWARE_AREA = {
    .num_subareas = 1,
    .subarea[0] =
        {
            .first_sector = 6,
            .num_sectors = 6,
        },
};
