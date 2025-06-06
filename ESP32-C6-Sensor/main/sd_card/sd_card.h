#ifndef SD_CARD_H
#define SD_CARD_H

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_check.h"
#include "driver/sdspi_host.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
// #include "driver/gpio.h"
#include <time.h>
#include <sys/time.h>
#include "malloc.h"
#include "measurement_utils/utils.h" // measurement_array_to_string

#define SDCARD_MAX_CHAR_SIZE 64
#define SDCARD_MOUNT_POINT "/sdcard"
#define SDCARD_FF_DRV_NOT_USED 0xFF
#define SDCARD_FORMAT_ON_MOUNT_FAIL 1
#define SDCARD_CHECK_DISK_STATUS 1
#define SDCARD_MAX_OPEN_FILES 5
#define SDCARD_ALLOCATION_UNIT_SIZE 16 * 1024


#define PIN_NUM_MISO GPIO_NUM_23
#define PIN_NUM_MOSI GPIO_NUM_22
#define PIN_NUM_CLK GPIO_NUM_21
#define PIN_NUM_CS GPIO_NUM_18

#define MOUNT_POINT "/sdcard"

/**
 * @brief Initialize the SD card connected using SPI and SD card module HW-125.
 * source: https://github.com/espressif/esp-idf/blob/master/examples/storage/sd_card/sdmmc/main/sd_card_example_main.c, https://github.com/i400s/tmp-sdcard/blob/main/main/sdcard_main.c
 * @author Espressif under Apache license, modified by (c) 2025 Josef Sikula
 */
void init_sd_card();

/**
 * @brief Write data to the SD card
 * @param filename - if no extension, adds .csv
 * @param data char array
 */
void write_to_sd_card(const char *filename, const char *data);

/**
 * @brief Save raw data to the SD card, line by line
 * @param filename - if no extension, adds .csv
 * @param measurement array of measurement_t
 * @details After writing measurement, frees the memory (even it fails to open the file)
 */
void save_raw_data(const char *filename, measurement_t *measurement);



#endif // SD_CARD_H