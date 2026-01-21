#ifndef PEOPLE_COUNTER_H
#define PEOPLE_COUNTER_H

#include <stdint.h>
//#include "freertos/queue.h" //cannoot be included once again
#include "measurement_utils/sensor.h"
#include "measurement_utils/utils.h"
#include "esp_log.h"
#include "led_indicator/led_indicator.h"

#ifdef CONFIG_BLINK_LED_STRIP
#define ENABLE_BLINK_INDICATOR
#endif

#define N_PIXELS_TO_ACTIVATE_ZONE 5
#define HEIGHT_THRESHOLD 1000
#define NOT_ENTERED 255
#define TRANSPOSE_MATRIX // transpose matrix if the sensor is rotated 90 degrees

#define ENTER_POSITION N_ZONES - 1
#define EXIT_POSITION 0


typedef struct
{
    int16_t people_count;    
    char *timestamp;
} people_count_t;

/**
 * @brief Count the number of people in the room
 * @param data The measurement data
 * @param background The background data
 * @param data_to_google_sheets_queue The queue to send the people count data to
 */
void count_people(measurement_t *data, uint16_t *background, QueueHandle_t data_to_google_sheets_queue);

/**
 * @brief Compute the background data as avarage of the measurement data
 * @param data The measurement data
 * @return The background data
 */
uint16_t *compute_background_data(measurement_t *data);

/**
 * @brief Upload the current_people_count to the Google Sheets
 * @param data_to_google_sheets_queue The queue to send the people count data to
 */
void upload_people_count(QueueHandle_t data_to_google_sheets_queue, int16_t n_people_count);


/**
 * @brief Check if all elements in the array are equal to a value
 * @param array The array to check
 * @param size The size of the array
 * @param value The value to check
 * @return 1 if all elements are equal to the value, 0 otherwise
 */
uint8_t all_elements(uint8_t *array, uint8_t size, uint8_t value);

/**
 * @brief Check if two arrays are equal
 * @param array1 The first array
 * @param array2 The second array
 * @param size The size of the arrays
 */
uint8_t are_arrays_equal(uint8_t *array1, uint8_t *array2, uint8_t size);

uint16_t median(uint16_t *array, uint8_t size);

#endif // PEOPLE_COUNTER_H