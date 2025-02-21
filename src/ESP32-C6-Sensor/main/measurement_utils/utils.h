#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "nvs_flash.h" //non volatile storage
#include <esp_sntp.h>


/**
 * @brief Allocate memory for a string containing the current time
 * @note Need to free the memory after use
 * @return current time in format "YYYY-MM-DD HH:MM:SS,ms"
 */
char *get_current_time(void);

/**
 * @brief Allocate memory for a string containing the current date
 * @note Need to free the memory after use
 * @return current date in format "YYYY_MM_DD"
 */
char *get_current_date(void);

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

#endif // UTILS_H