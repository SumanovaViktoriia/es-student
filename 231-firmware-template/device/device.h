#pragma once

#include <stdint.h>

#define FIRMWARE_NAME "es-firmware-template"
#define FIRMWARE_VERSION "2.0.0"
#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/mmariaaabdr/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

typedef struct {
    const char *board;
    char serial[17];
    uint32_t chip_manufacturer;
    uint32_t chip_part;
    uint32_t chip_revision;
    const char *sdk_version;
} device_info_t;

void device_get_info(device_info_t *info);