#ifndef PEOPLE_COUNTER_CORRELATION_MATRIX_H
#define PEOPLE_COUNTER_CORRELATION_MATRIX_H

#include "measurement_utils/sensor.h"
#include "esp_log.h"
#include "people_counter/person_movement.h"
#include "people_counter/people_counter.h" // for upload_people_count
#include <math.h>



void process_frame(measurement_t *data, QueueHandle_t data_to_google_sheets_queue);

uint8_t find_local_minimum(uint8_t start_index);


/// @brief  Finds all local minimums in the correlation matrix
/// @return pointer to the head of the linked list of local minimums
local_minimum_list_t *find_all_local_minimums();

void count_people_correlation_matrix(local_minimum_list_t *local_minimums, QueueHandle_t data_to_google_sheets_queue);

/// Checks if the index is on the border of the grid, where the person can enter or exit.
uint8_t is_border_index(uint8_t index);

/// @brief uses EXIT_BORDER_INDEX_CONDITION to check if the index is on the exit border of the grid, where the person can exit.
/// @param index (range 0-63) index of the grid
/// @return if the index is on the exit border of the grid, where the person can exit.
uint8_t is_exit_border_index(uint8_t index);

/// @brief Calculates the distance between two points in the grid: distance = sqrt(deltaX^2 + deltaY^2)
/// @param index1 
/// @param index2 
/// @return 
double get_distance(uint8_t index1, uint8_t index2);

#endif // PEOPLE_COUNTER_CORRELATION_MATRIX_H