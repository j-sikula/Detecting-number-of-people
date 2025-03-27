#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "nvs_flash.h" //non volatile storage
#include <esp_sntp.h>
#include "measurement_utils/sensor.h"

#define DATE_TIME_LENGTH 23 //for example 2025-02-13 12:23:26,874

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
 * @note Need to free the memory after use
 */
char *measurement_array_to_string(measurement_t *measurement);

void check_heap_memory();

#endif // UTILS_H