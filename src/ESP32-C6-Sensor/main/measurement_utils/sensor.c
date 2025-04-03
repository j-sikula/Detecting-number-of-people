#include "sensor.h"
#include "esp_log.h"
#include "measurement_utils/utils.h"
#include "string.h"
#include "people_counter/people_counter.h"
#include "led_indicator/led_indicator.h"

uint8_t is_measuring = 1;
uint8_t status, loop, isAlive, isReady, i;
uint32_t integration_time_ms;
VL53L7CX_Configuration Dev;	  /* Sensor configuration */
VL53L7CX_ResultsData Results; /* Results data from VL53L7CX */

void initVL53L7CX()
{
	/*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	/*********************************/
	/*      Customer platform        */
	/*********************************/

	/* Fill the platform structure with customer's implementation. For this
	 * example, only the I2C address is used.
	 */
	Dev.platform.address = I2C_ADDRESS;

	VL53L7CX_InitCommunication(&(Dev.platform));

	/* (Optional) Reset sensor toggling PINs (see platform, not in API) */
	// VL53L7CX_Reset_Sensor(&(Dev.platform));

	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	ESP_LOGI("setup", "Memory allocated");

	/* (Optional) Check if there is a VL53L7CX sensor connected */
	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if (!isAlive || status)
	{
		ESP_LOGI("sensor", "VL53L7CX not detected at requested address\n");
	}

	status = vl53l7cx_init(&Dev);
	if (status)
	{
		ESP_LOGI("sensor", "VL53L7CX ULD Loading failed\n");
	}
	else
	{
		ESP_LOGI("sensor", "VL53L7CX ULD ready ! (Version : %s)\n",
				 VL53L7CX_API_REVISION);
	}

	/*********************************/
	/*        Set some params        */
	/*********************************/

	/* Set resolution in 8x8. WARNING : As others settings depend to this
	 * one, it must be the first to use.
	 */
	status = vl53l7cx_set_resolution(&Dev, RESOLUTION);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_resolution failed, status %u\n", status);
		return;
	}

	/* Set ranging frequency to 10Hz.
	 * Using 4x4, min frequency is 1Hz and max is 60Hz
	 * Using 8x8, min frequency is 1Hz and max is 15Hz
	 */
	status = vl53l7cx_set_ranging_frequency_hz(&Dev, RANGING_FREQUENCY_HZ);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_ranging_frequency_hz failed, status %u\n", status);
		return;
	}

	/* Set target order to closest */
	status = vl53l7cx_set_target_order(&Dev, TARGET_ORDER);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_target_order failed, status %u\n", status);
		return;
	}

	status = vl53l7cx_set_ranging_mode(&Dev, RANGING_MODE);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_ranging_mode failed, status %u\n", status);
		return;
	}
	/* Get current integration time */
	status = vl53l7cx_get_integration_time_ms(&Dev, &integration_time_ms);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_get_integration_time_ms failed, status %u\n", status);
		return;
	}
	ESP_LOGI("sensor", "Current integration time is : %d ms", (int)integration_time_ms);
}

void startContinuousMeasurement(QueueHandle_t data_to_sd_queue, QueueHandle_t data_to_google_sheets_queue)
{
	status = vl53l7cx_start_ranging(&Dev);
	uint16_t *background = NULL;
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	single_blink(0, 0, 14, 2);
	while (is_measuring)
	{
		loop = 0;
		check_heap_memory();
		measurement_t *measurement = (measurement_t *)malloc(MEASUREMENT_LOOP_COUNT * sizeof(measurement_t));
		if (measurement == NULL)
		{
			while(measurement == NULL)	// try to allocate memory until success
			{
				ESP_LOGE("vTaskLoop", "Failed to allocate memory for measurement");
				vTaskDelay(10 / portTICK_PERIOD_MS);
				measurement = (measurement_t *)malloc(MEASUREMENT_LOOP_COUNT * sizeof(measurement_t));
			}
		}

		while (loop < MEASUREMENT_LOOP_COUNT)
		{
			/* Use polling function to know when a new measurement is ready.
			 * Another way can be to wait for HW interrupt raised on PIN A3
			 * (GPIO 1) when a new measurement is ready */

			status = vl53l7cx_check_data_ready(&Dev, &isReady);

			if (isReady)
			{
				vl53l7cx_get_ranging_data(&Dev, &Results);

				/* As the sensor is set in 8x8 mode by default, we have a total
				 * of 64 zones to print.
				 */

				measurement[loop].timestamp = get_current_time();
				for (i = 0; i < N_PIXELS; i++)
				{
					measurement[loop].distance_mm[i] = Results.distance_mm[i];
					measurement[loop].status[i] = Results.target_status[i];
				}

				if (background != NULL)
				{
					count_people(&measurement[loop], background, data_to_google_sheets_queue);
				}

#ifdef PRINT_DATA_TO_UART
				printf("\nData\n%s\n", measurement[loop].timestamp);

				for (i = 0; i < VL53L7CX_RESOLUTION_8X8; i++)
				{
					printf("%d;%d ", Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i], Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i]);
				}
#endif

				loop++;
			}

			/* Wait a few ms to avoid too high polling (function in platform
			 * file, not in API) */
			VL53L7CX_WaitMs(&(Dev.platform), 10);
		}

		if (background == NULL)
		{
			background = compute_background_data(measurement);
			single_blink(0, 25, 0, 10);
			for (i = 0; i < MEASUREMENT_LOOP_COUNT; i++)
			{
				printf("%d, ", measurement[i].distance_mm[20]);
			}
		}
		

		if (xQueueSend(data_to_sd_queue, &measurement, portMAX_DELAY) != pdPASS)
		{
			ESP_LOGE("sensor", "Failed to send measurement data to queue");
		}
	}
	status = vl53l7cx_stop_ranging(&Dev);
	free(background);
}
