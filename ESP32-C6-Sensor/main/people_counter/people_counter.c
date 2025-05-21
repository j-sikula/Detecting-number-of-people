#include "people_counter.h"

#define N_ZONES 4
int16_t current_people_count = 0; // Global variable to track the number of people
uint8_t zone_entered[N_ZONES] = {0};
uint8_t zone_exited[N_ZONES] = {0};
uint8_t position_entered = NOT_ENTERED;

void count_people(measurement_t *data, uint16_t *background, QueueHandle_t data_to_google_sheets_queue)
{
    int16_t heightData[N_PIXELS];
    for (uint8_t j = 0; j < N_PIXELS; j++)
    {
        heightData[j] = background[j] - data->distance_mm[j];
    }

    for (uint8_t i = 0; i < N_ZONES; i++)
    {
        uint8_t nPixelsAboveThreshold = 0;
        for (int j = 0; j < (64 / N_ZONES); j++)
        {
#ifndef TRANSPOSE_MATRIX
            if (heightData[i * 64 / N_ZONES + j] > HEIGHT_THRESHOLD)
            {
                nPixelsAboveThreshold++;
            }
#endif
#ifdef TRANSPOSE_MATRIX
            if (heightData[i * 8 / N_ZONES + j % (8 / N_ZONES) + j / (8 / N_ZONES) * 8] > HEIGHT_THRESHOLD) // matrix transposed
            {
                nPixelsAboveThreshold++;
            }
#endif
        }

        if (nPixelsAboveThreshold > N_PIXELS_TO_ACTIVATE_ZONE)
        {
            zone_entered[i] = true;
            zone_exited[i] = false;
            if (position_entered == NOT_ENTERED && (i == 0 || i == N_ZONES - 1))
            {
                position_entered = i;
            }
        }
        else
        {
            // If the zone is exited, the zone is exited only if the zone was entered
            if (zone_entered[i])
            {
                zone_exited[i] = true;
            }

            // If all the zones are exited, the people count is updated
            if (all_elements(zone_exited, N_ZONES, true))
            {
                if (position_entered == EXIT_POSITION)
                {
                    current_people_count--;
                    ESP_LOGI("PeopleCounter", "Person exited room");
#ifdef ENABLE_BLINK_INDICATOR
                    single_blink(DEFAULT_BLINK_INTENSITY, 0, 0, 5);
#endif
                    upload_people_count(data_to_google_sheets_queue, current_people_count);
                }
                if (position_entered == ENTER_POSITION)
                {
                    current_people_count++;
                    ESP_LOGI("PeopleCounter", "Person entered room");
#ifdef ENABLE_BLINK_INDICATOR
                    single_blink(0, DEFAULT_BLINK_INTENSITY, 0, 5);
#endif
                    upload_people_count(data_to_google_sheets_queue, current_people_count);
                }

                // Reset the position entered and the zones entered and exited
                position_entered = NOT_ENTERED;
                for (uint8_t k = 0; k < N_ZONES; k++)
                {
                    zone_entered[k] = false;
                    zone_exited[k] = false;
                }
            }
            else
            {
                // when person exits in position entered
                if (are_arrays_equal(zone_entered, zone_exited, N_ZONES))
                {
                    position_entered = NOT_ENTERED;
                    for (uint8_t k = 0; k < N_ZONES; k++)
                    {
                        zone_entered[k] = false;
                        zone_exited[k] = false;
                    }
                }
            }
        }
    }
}

uint16_t *compute_background_data(measurement_t *data)
{
    uint16_t *background = (uint16_t *)malloc(N_PIXELS * sizeof(uint16_t));
    for (uint8_t i = 0; i < N_PIXELS; i++)
    {
        uint16_t zone[MEASUREMENT_LOOP_COUNT];
        for (uint8_t j = 0; j < MEASUREMENT_LOOP_COUNT; j++)
        {
            zone[j] = data[j].distance_mm[i];
        }
        // Compute the background data
        background[i] = median(zone, N_PIXELS);

        if (i % 8 == 0)
        {
            printf("\n");
        }
        printf("%d;%d ", background[i], data[5].distance_mm[i]);
    }
    return background;
}

void upload_people_count(QueueHandle_t data_to_google_sheets_queue, int16_t n_people_count)
{
    people_count_t *people_count = (people_count_t *)malloc(sizeof(people_count_t));
    if (people_count == NULL)
    {
        ESP_LOGE("PeopleCounter", "Failed to allocate memory for people count");
        return;
    }

    people_count->timestamp = get_current_time();
    people_count->people_count = n_people_count;

    if (xQueueSend(data_to_google_sheets_queue, &people_count, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGE("PeopleCounter", "Failed to send people count data to the queue");
        free(people_count->timestamp);
        free(people_count);
    }
}

uint8_t all_elements(uint8_t *array, uint8_t size, uint8_t value)
{
    for (uint8_t i = 0; i < size; i++)
    {
        if (!(array[i] == value))
        {
            return false;
        }
    }
    return true;
}

uint8_t are_arrays_equal(uint8_t *array1, uint8_t *array2, uint8_t size)
{
    for (uint8_t i = 0; i < size; i++)
    {
        if (array1[i] != array2[i])
        {
            return false;
        }
    }
    return true;
}

uint16_t median(uint16_t *array, uint8_t size)
{
    uint8_t indexes[size];
    for (uint8_t i = 0; i < size; i++)
    {
        indexes[i] = i;
    }
    // https://builtin.com/machine-learning/fastest-sorting-algorithm
    for (uint8_t i = 1; i < size; i++)
    {
        uint8_t j = i;
        while (j > 0 && array[indexes[j - 1]] > array[indexes[j]])
        {
            uint8_t temp = indexes[j];
            indexes[j] = indexes[j - 1];
            indexes[j - 1] = temp;
            j--;
        }
    }
    if (size % 2 == 0)
    {
        return (array[indexes[size / 2]] + array[indexes[size / 2 - 1]]) / 2;
    }
    return array[indexes[size / 2]];
}
// Median test
//  int main()
//  {
//      uint16_t test[] = {5, 33, 50, 2, 1, 0, 7, 3, 26};
//      uint16_t med = median(test, *(&test + 1) - test);
//      printf("%d", med);

//     return 0;
// }