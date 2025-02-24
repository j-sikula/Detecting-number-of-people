#include "sensor.h"
#include "esp_log.h"
#include "measurement_utils/utils.h"

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

void startContinuousMeasurement(QueueHandle_t measurementQueue)
{
	status = vl53l7cx_start_ranging(&Dev);
	while (is_measuring)
	{
		loop = 0;
		measurement_t* measurement = (measurement_t*)malloc(MEASUREMENT_LOOP_COUNT * sizeof(measurement_t));
		if (measurement == NULL)
		{
			ESP_LOGE("vTaskLoop", "Failed to allocate memory for measurement");
			return;
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
				for (i = 0; i < N_ZONES; i++)
				{
					measurement[loop].distance_mm[i] = Results.distance_mm[i];
				}


				printf("\nData\n%s", measurement[loop].timestamp);

				
				for (i = 0; i < VL53L7CX_RESOLUTION_8X8; i++)
				{
					printf("%4d;%d ", Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i], Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i]);
				}
				loop++;
			}

			/* Wait a few ms to avoid too high polling (function in platform
			 * file, not in API) */
			VL53L7CX_WaitMs(&(Dev.platform), 10);
		}
		free(measurement);
/*
		if (xQueueSend(measurementQueue, &measurement, portMAX_DELAY) != pdPASS)
		{
			ESP_LOGE("vTaskLoop", "Failed to send measurement to queue");
		}*/
	}
	status = vl53l7cx_stop_ranging(&Dev);
}
