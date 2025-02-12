#ifndef SENSOR_H
#define SENSOR_H

#include "vl53l7cx_uld_api/vl53l7cx_api.h"
#include "platform/platform.h"


#define I2C_ADDRESS             VL53L7CX_DEFAULT_I2C_ADDRESS
#define RESOLUTION              VL53L7CX_RESOLUTION_8X8 
#define RANGING_FREQUENCY_HZ    10
#define TARGET_ORDER            VL53L7CX_TARGET_ORDER_CLOSEST
#define RANGING_MODE            VL53L7CX_RANGING_MODE_CONTINUOUS

#define MEASUREMENT_LOOP_COUNT 100

/**
 * @brief Initialize the sensor.
 * Starts I2C communication and sets defined parameters.
 */
void initVL53L7CX();

/**
 * @brief Start continuous measurement.
 * Starts the ranging and loops until the measurement is stopped.
 */
void startContinuousMeasurement();

#endif // SENSOR_H