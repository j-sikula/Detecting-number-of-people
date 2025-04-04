#ifndef SD_CARD_H
#define SD_CARD_H
// https://github.com/i400s/tmp-sdcard/blob/main/main/sdcard_main.c

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

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
// #define PIN_NUM_MISO  20
// #define PIN_NUM_MOSI  21
// #define PIN_NUM_CLK   19
// #define PIN_NUM_CS    22

#define PIN_NUM_MISO GPIO_NUM_23
#define PIN_NUM_MOSI GPIO_NUM_22
#define PIN_NUM_CLK GPIO_NUM_21
#define PIN_NUM_CS GPIO_NUM_18

#define MOUNT_POINT "/sdcard"

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
 */
void save_raw_data(const char *filename, measurement_t *measurement);

/**
 * https://www.geeksforgeeks.org/c-program-to-read-contents-of-whole-file/
 */
char *read_data(const char *filename, int size);

/**
 * @brief returns content of file
 */
char *read_file(const char *filename);

#endif // SD_CARD_H