#include "device.h"
#include <stdio.h>
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

void device_get_info(device_info_t *info)
{
    info->board = DEVICE_BOARD;
    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));

    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    info->chip_manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->chip_part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    info->chip_revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    info->sdk_version = PICO_SDK_VERSION_STRING;
}