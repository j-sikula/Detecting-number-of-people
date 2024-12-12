/*
   The I2C bus scanner tests all addresses and detects chips and
   modules that are connected to the SDA and SCL signals.
   Xtensa dual-core 32-bit LX6 (ESP32-CAM), 240 MHz
   PlatformIO, ESP-IDF framework

   Copyright (c) 2022 Tomas Fryza, 2024 Josef Sikula
   Dept. of Radio Electronics, Brno University of Technology, Czechia
   This work is licensed under the terms of the GNU GENERAL PUBLIC LICENSE.
 */

/*-----------------------------------------------------------*/
#include <freertos/FreeRTOS.h> // FreeRTOS
#include <freertos/task.h>
#include <esp_log.h>	// ESP_LOG/E/W/I functions
#include <driver/i2c.h> // Inter-Integrated Circuit driver
#include <driver/uart.h> // UART driver
#include "platform.h"
#include "vl53l7cx_api.h"

/*-----------------------------------------------------------*/

#define BUILT_IN_LED 2

/*-----------------------------------------------------------*/
// Used function(s)

void vTaskLoop();

/*-----------------------------------------------------------*/
/* In ESP-IDF instead of "main", we use "app_main" function
   where the program execution begins */
void app_main(void)
{
	uart_set_baudrate(UART_NUM_0, 460800);
	// Set GPIO
	gpio_reset_pin(BUILT_IN_LED);
	gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);

	// Start the i2c scanner task
	xTaskCreate(vTaskLoop, "forever_loop", 40 * 1024, NULL, 5, NULL);
}

/*-----------------------------------------------------------*/
void vTaskLoop()
{

	/*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	uint8_t status, loop, isAlive, isReady, i;
	uint32_t integration_time_ms;
	VL53L7CX_Configuration Dev;	  /* Sensor configuration */
	VL53L7CX_ResultsData Results; /* Results data from VL53L7CX */

	/*********************************/
	/*      Customer platform        */
	/*********************************/

	/* Fill the platform structure with customer's implementation. For this
	 * example, only the I2C address is used.
	 */
	Dev.platform.address = VL53L7CX_DEFAULT_I2C_ADDRESS;

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
	status = vl53l7cx_set_resolution(&Dev, VL53L7CX_RESOLUTION_8X8);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_resolution failed, status %u\n", status);
		return;
	}

	/* Set ranging frequency to 10Hz.
	 * Using 4x4, min frequency is 1Hz and max is 60Hz
	 * Using 8x8, min frequency is 1Hz and max is 15Hz
	 */
	status = vl53l7cx_set_ranging_frequency_hz(&Dev, 10);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_ranging_frequency_hz failed, status %u\n", status);
		return;
	}

	/* Set target order to closest */
	status = vl53l7cx_set_target_order(&Dev, VL53L7CX_TARGET_ORDER_CLOSEST);
	if (status)
	{
		ESP_LOGE("sensor", "vl53l7cx_set_target_order failed, status %u\n", status);
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

	/*********************************/
	/*         Ranging loop          */
	/*********************************/

	status = vl53l7cx_start_ranging(&Dev);
	while (1)
	{
		loop = 0;
		while (loop < 255)
		{
			/* Use polling function to know when a new measurement is ready.
			 * Another way can be to wait for HW interrupt raised on PIN A3
			 * (GPIO 1) when a new measurement is ready */

			status = vl53l7cx_check_data_ready(&Dev, &isReady);

			if (isReady)
			{
				vl53l7cx_get_ranging_data(&Dev, &Results);

				/* As the sensor is set in 4x4 mode by default, we have a total
				 * of 16 zones to print. For this example, only the data of first zone are
				 * print */
				printf("Print data no : %3u\n", Dev.streamcount);
				for (i = 0; i < 16; i++)
				{
					printf("Zone : %3d, Status : %3u, Distance : %4d mm\n",
						   i,
						   Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i],
						   Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
				}
				printf("\033[17A");
				loop++;
			}

			/* Wait a few ms to avoid too high polling (function in platform
			 * file, not in API) */
			VL53L7CX_WaitMs(&(Dev.platform), 1);
		}
	}
	printf("End of ULD demo\n");
	status = vl53l7cx_stop_ranging(&Dev);
	// Delete this task if it exits from the loop above
	vTaskDelete(NULL);
}