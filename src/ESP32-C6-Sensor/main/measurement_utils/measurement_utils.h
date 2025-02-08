#ifndef MEASUREMENT_UTILS_H
#define MEASUREMENT_UTILS_H

#include <stdint.h>

typedef struct
{
    uint16_t measured_values[64];
    char *timestamp;
} measurement_t;

char* get_current_time(void);
#endif // MEASUREMENT_UTILS_H