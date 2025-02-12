#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "nvs_flash.h" //non volatile storage
#include <esp_sntp.h>

typedef struct
{
    uint16_t distance_mm[64];
    char *timestamp;
} measurement_t;

char *get_current_time(void);

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