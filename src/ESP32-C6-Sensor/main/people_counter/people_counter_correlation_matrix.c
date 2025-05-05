#include "people_counter_correlation_matrix.h"
#include "people_counter/person_movement.h"
#define BACKGROUND_HEIGHT 2100

uint16_t correlation_matrix[N_PIXELS];

#define DETECTION_GRID_SIZE 9
uint8_t detection_grid[DETECTION_GRID_SIZE] = {1, 1, 1, 1, 2, 1, 1, 1, 1};
#define MAX_MOVEMENT_LENGTH 4.3
#define MIN_LOCAL_MINIMUMS_DISTANCE 4.0
#define DEPTH_THRESHOLD 1390
#define TAG "PeopleCounter"

int16_t people_count = 0; // Number of people detected
char *timestamp = NULL; // Timestamp of the measurement
uint8_t movement_not_detected_yet = 0; // Flag to check if the movement is detected

person_movement_t *person_movement_list = NULL; // Pointer to the head of the list

void process_frame(measurement_t *data, QueueHandle_t data_to_google_sheets_queue)
{
    timestamp = data->timestamp; // Get the timestamp from the measurement data
    uint16_t depth_data[N_PIXELS];
    for (uint8_t i = 0; i < N_PIXELS; i++)
    {
        // Eliminate failed measurements
        if (data->status[i] == 255)
        {
            depth_data[i] = BACKGROUND_HEIGHT;
        }
        else
        {
            depth_data[i] = data->distance_mm[i];
        }

        // eliminate the door
        if ((i % 8 == 7 &&
             depth_data[i] <= 147 &&
             depth_data[i] >= 134 &&
             data->distance_mm[(i + 32) % 64] <= 147 &&
             data->distance_mm[(i + 32) % 64] >= 134) ||
            (i % 8 == 6 &&
             depth_data[i] <= 215 &&
             depth_data[i] >= 190 &&
             data->distance_mm[(i + 32) % 64] <= 215 &&
             data->distance_mm[(i + 32) % 64] >= 190) ||
            (i % 8 == 5 &&
             depth_data[i] <= 340 &&
             depth_data[i] >= 280 &&
             data->distance_mm[(i + 32) % 64] <= 340 &&
             data->distance_mm[(i + 32) % 64] >= 280))
        {
            depth_data[i] = 2100; // Set the depth data to background (2100 mm)
        }

        for (uint8_t i = 0; i < N_PIXELS; i++)
        {
            uint8_t nWeightedCells = 0; // Number of weighted cells
            for (uint8_t j = 0; j < DETECTION_GRID_SIZE; j++)
            {
                int index = i +
                            (8 * (j / DETECTION_GRID_SIZE) +
                             j % DETECTION_GRID_SIZE -
                             (DETECTION_GRID_SIZE - 1) / 2 * 9); // Calculate the index for the depth data grid
                // Check if the index is within bounds and in the same row (overflow is handled)
                if (index >= 0 &&
                    index < 64 &&
                    index / 8 ==
                        i / 8 -
                            (DETECTION_GRID_SIZE - 1) / 2 +
                            j / DETECTION_GRID_SIZE)
                {
                    correlation_matrix[i] += depth_data[index] * detection_grid[j];
                    nWeightedCells++;
                }
            }
            correlation_matrix[i] = (correlation_matrix[i] / nWeightedCells);
        }
    }

    local_minimum_list_t *local_minimums = find_all_local_minimums(); // Find all local minimums in the correlation matrix
    movement_not_detected_yet = 1; // Reset the flag for movement detection
    count_people_correlation_matrix(local_minimums, data_to_google_sheets_queue); // Count people based on the local minimums found
    clear_local_minimum_list(&local_minimums);                 // Clear the list of local minimums
}
/// Finds the index of the local minimum in the correlation_matrix array concerned as a 8x8 grid
/// Algorithm: gradient descent
uint8_t find_local_minimum(uint8_t start_index)
{

    bool local_minimum_found = false; // Flag to check if local minimum is found
    while (!local_minimum_found)
    {
        uint8_t index_x = start_index % 8;   // Column index
        uint8_t index_y = start_index / 8;   // Row index
        local_minimum_found = true;          // Set the flag to true
        uint8_t local_minimum = start_index; // Local minimum value
        for (int8_t i = -1; i <= 1; i++)
        {
            for (int8_t j = -1; j <= 1; j++)
            {
                if (i == 0 && j == 0)
                    continue; // Skip the center element
                int neighborX = index_x + j;
                int neighbor_y = index_y + i;
                if (neighborX >= 0 &&
                    neighborX < 8 &&
                    neighbor_y >= 0 &&
                    neighbor_y < 8)
                {
                    int neighborIndex = neighbor_y * 8 + neighborX;
                    if (correlation_matrix[neighborIndex] < correlation_matrix[local_minimum])
                    {
                        local_minimum =
                            neighborIndex; // Update the index of the probable local minimum
                        local_minimum_found =
                            false; // Set the flag to false, found lower value
                    }
                }
            }
        }
        if (!local_minimum_found)
        {
            start_index =
                local_minimum; // Update the start index to the new local minimum
        }
    }
    return start_index; // Return the index of the local minimum
}

local_minimum_list_t *find_all_local_minimums()
{
    local_minimum_list_t *local_minimums; // Pointer to the head of the list

    for (uint8_t i = 0; i < 8; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            uint8_t is_local_minimum = true;
            for (uint8_t k = -1; k <= 1; k++)
            {
                for (uint8_t l = -1; l <= 1; l++)
                {
                    if (k == 0 && l == 0)
                        continue; // Skip the center element
                    int8_t neighbor_x = j + l;
                    int8_t neighbor_y = i + k;
                    if (neighbor_x >= 0 &&
                        neighbor_x < 8 &&
                        neighbor_y >= 0 &&
                        neighbor_y < 8)
                    {
                        uint8_t neighborIndex = neighbor_y * 8 + neighbor_x;
                        if (correlation_matrix[neighborIndex] < correlation_matrix[i * 8 + j])
                        {
                            is_local_minimum = false;
                            break;
                        }
                    }
                }
                if (!is_local_minimum)
                    break;
            }
            if (is_local_minimum)
            {
                add_local_minimum(&local_minimums, i * 8 + j); // Add the index of the local minimum
            }
        }
    }

    return local_minimums;
}

void count_people_correlation_matrix(local_minimum_list_t *local_minimums, QueueHandle_t data_to_google_sheets_queue)
{
    if (local_minimums == NULL && person_movement_list != NULL && person_movement_list->delete_movement)
    {
        // No people present, clear the list
        clear_list(&person_movement_list);
        return;
    }
    person_movement_t *person_movement_i = person_movement_list; // Pointer to the head of the list
    // Remove multiple people movements in the same position
    while (person_movement_i != NULL)
    {                                                                                   // for (int i = 0; i < person_movement_list.length; i++) {
        person_movement_t *person_movement_j = person_movement_i->next_element_pointer; // Pointer to the next element
        while (person_movement_j != NULL)
        { // for (int j = i + 1; j < person_movement_list.length; j++) {
            if (person_movement_i->current_position ==
                    person_movement_j->current_position &&
                person_movement_j->started_exiting ==
                    person_movement_i->started_exiting)
            {
                person_movement_t *tmp = person_movement_j->next_element_pointer; // Store the element to be removed in a temporary variable
                remove_from_list(&person_movement_list, person_movement_j);       // Remove the element from the list
                person_movement_j = tmp;                                          // Update the pointer to the next element
            }
        }
        person_movement_i = person_movement_i->next_element_pointer; // Move to the next element
    }

    person_movement_i = person_movement_list; // Reset the pointer to the head of the list

    while (person_movement_i != NULL)
    {
        uint8_t destination_index = find_local_minimum(person_movement_i->current_position);
        if (get_distance(destination_index, person_movement_i->current_position) >
            MAX_MOVEMENT_LENGTH)
        {
            // If the distance is too big, remove the person from the list
            destination_index = person_movement_i->current_position;
            //ESP_LOGI(TAG,"Overflowing distance, banned");
        }
        if (correlation_matrix[destination_index] < DEPTH_THRESHOLD)
        {
            if (is_border_index(destination_index))
            {
                if (is_exit_border_index(destination_index) ==
                    is_exit_border_index(person_movement_i->start_position))
                {
                    // Person exits where entered
                    update_position(person_movement_i, destination_index); // Update the current position
                    person_movement_i->start_position =
                        destination_index; // Update the start position
                }
                else
                {
                    // Check if there is not multiple detection of the same person
                    if (movement_not_detected_yet)
                    {
                        movement_not_detected_yet = 0; // Reset the flag for movement detection
                        if (person_movement_i->started_exiting)
                        {
                            ESP_LOGI(TAG,"Person exited %s", timestamp);
                            people_count--;
                            upload_people_count(data_to_google_sheets_queue, people_count);
                        }
                        else
                        {
                            ESP_LOGI(TAG,"Person entered %s", timestamp);
                            people_count++;
                            upload_people_count(data_to_google_sheets_queue, people_count);
                        }
                    }
                    reset_person_movement(person_movement_i,
                                         destination_index);  // After finishing the movement, create a new movement for case it returns back immediately
                }
            }
            else
            {
                update_position(person_movement_i, destination_index); // Update the current position
            }

            
        }
    }

    local_minimum_list_t *local_minimum_i = local_minimums; // Pointer to the head of the list
    while (local_minimum_i != NULL)
    {
        // person on the edge of the grid
        if (is_border_index(local_minimum_i->index))
        {
            if (person_movement_list == NULL)
            {
                // If no people are present, add the first person
                add_to_list(&person_movement_list,
                            local_minimum_i->index); // Add the person to the list
                                
            }
            else
            {
                if (is_current_index_in_list(person_movement_list, local_minimum_i->index))//indexesOfPresentPeople.contains(indexesOfLocalMinimums[i]))
                {
                    // If the person is already present, skip
                    continue;
                }
                else
                {
                    // If the person is not present in PersonMovement, add the new person
                    double min_distance = 8;
                    person_movement_t *person_movement_j = person_movement_list; // Pointer to the head of the list
                    while (person_movement_j != NULL)
                    {
                        min_distance = get_distance(person_movement_j->current_position,
                                                  local_minimum_i->index);
                        person_movement_j = person_movement_j->next_element_pointer; // Move to the next element
                    }
                    if (min_distance >= MIN_LOCAL_MINIMUMS_DISTANCE)
                    {
                        add_to_list(&person_movement_list,
                                    local_minimum_i->index); // Add the person to the list
                       
                        
                    }
                }
            }
        }
    }

    remove_unactive_from_list(&person_movement_list); // Remove unactive elements from the list

    return;
}

uint8_t is_border_index(uint8_t index)
{
    uint8_t x = index % 8; // Column index
    uint8_t y = index / 8; // Row index

    return x < 2 ||
           x > 5 ||
           ((x == 2 || x == 5) && (y == 0 || y == 7));
}

uint8_t is_exit_border_index(uint8_t index)
{
    return index % 8 EXIT_BORDER_INDEX_CONDITION;
}

double get_distance(uint8_t index1, uint8_t index2)
{
    uint8_t x1 = index1 % 8; // Column index of the first point
    uint8_t y1 = index1 / 8; // Row index of the first point
    uint8_t x2 = index2 % 8; // Column index of the second point
    uint8_t y2 = index2 / 8; // Row index of the second point
    double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)); // Calculate the Euclidean distance
    return distance;
}
