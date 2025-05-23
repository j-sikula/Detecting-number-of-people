#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "nvs_flash.h" //non volatile storage
#include <esp_sntp.h>
#include "esp_vfs_fat.h"
#include <stdio.h>
#include "measurement_utils/sensor.h"

#define LOG_FILE_NAME "/sdcard/log.log"
#define DATE_TIME_LENGTH 23 //for example 2025-02-13 12:23:26,874

/// @brief returns 1 once a day
/// @return 1 only once when called between 0:00AM and 1:00AM, otherwise 0
uint8_t is_midnight();

/**
 * @brief Allocate memory for a string containing the current time
 * @note Need to free the memory after use
 * @return current time in format "YYYY-MM-DD HH:MM:SS,ms"
 */
char *get_current_time(void);

/**
 * @brief Allocate memory for a string containing the current date
 * @note Need to free the memory after use
 * @return current date in format "YYYYMMDD"
 */
char *get_current_date(void);

/**
 * @brief Allocate memory for a string containing the current week
 * @note Need to free the memory after use
 * @return current week in ISO 8601 format "YYYY-WWW"
 */
char *get_current_week(void);

/**
 * @brief Set local time from Wi-Fi
 * @return 0 - failed, 1 - success
 */
uint8_t obtain_time(void);

/**
 * @brief Initialize SNTP used for obtaining time
 *
 */
void initialize_sntp(void);

/**
 * @brief Convert measurement array to string
 * structure of string: date_time;64x 4 digits depth; 64x 3 digits status;\n
 * @param measurement array - size MEASUREMENT_LOOP_COUNT
 * @return string containing all measurements
 * @note Frees memory for timestamp, need to free the *measurement after use
 */
char *measurement_array_to_string(measurement_t *measurement);

/**
 * @brief Prints to log current free heap size
 * @note This function is used for debugging purposes
 */
void check_heap_memory();

/**
 * Initialize log file and set custom log function: int custom_log_to_file
 * @param log_file_name - name of the log file
 */
void init_log_to_file(char *log_file_name);

/**
 * @brief Custom log function that writes logs to a file and console
 */
int custom_log_to_file(const char *fmt, va_list args);

/**
 * @brief Closes and opens log file to ensure that data writen from last close of the file are correctly saved
 * @param log_file_name - name of the log file
 * @return 0 - failed, 1 - success
 */
uint8_t refresh_log_file(char *log_file_name);

/**
 * @brief Closes the log file
 * @return 0 - failed, 1 - success
 */
uint8_t close_log_file(void);

#endif // UTILS_H