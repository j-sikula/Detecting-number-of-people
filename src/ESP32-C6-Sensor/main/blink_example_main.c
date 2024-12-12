/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/i2c.h"	 // Inter-Integrated Circuit driver
#include "driver/uart.h" // UART driver
#include "vl53l7cx_uld_api/vl53l7cx_api.h"
#include "platform/platform.h"
#include "wifi/wifi_controller.h"
#include "nvs_flash.h" //non volatile storage
#include <esp_sntp.h>

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

void print_current_time()
{
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	int milliseconds = tv.tv_usec / 1000;

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	printf("%s,%d\n", buffer, milliseconds);
}

static void blink_led(void)
{
	/* If the addressable LED is enabled */
	if (s_led_state)
	{
		/* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
		led_strip_set_pixel(led_strip, 0, 16, 16, 16);
		/* Refresh the strip to send data */
		led_strip_refresh(led_strip);
	}
	else
	{
		/* Set all LED off to clear all pixels */
		led_strip_clear(led_strip);
	}
}

static void configure_led(void)
{
	ESP_LOGI(TAG, "Example configured to blink addressable LED!");
	/* LED strip initialization with the GPIO and pixels number*/
	led_strip_config_t strip_config = {
		.strip_gpio_num = BLINK_GPIO,
		.max_leds = 1, // at least one LED on board
	};
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
	led_strip_rmt_config_t rmt_config = {
		.resolution_hz = 10 * 1000 * 1000, // 10MHz
		.flags.with_dma = false,
	};
	ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
	led_strip_spi_config_t spi_config = {
		.spi_bus = SPI2_HOST,
		.flags.with_dma = true,
	};
	ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
	/* Set all LED off to clear all pixels */
	led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
	/* Set the GPIO level according to the state (LOW or HIGH)*/
	gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
	ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
	gpio_reset_pin(BLINK_GPIO);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

void vTaskLoop();
void vWifiTask();
void app_main(void)
{

	/* Configure the peripheral according to the LED type */
	configure_led();

	print_current_time();

	// Start the i2c scanner task
	xTaskCreate(vTaskLoop, "forever_loop", 40 * 1024, NULL, 5, NULL);
	xTaskCreate(vWifiTask, "wifi_task", 4096, NULL, 5, NULL);

	while (1)
	{
		// ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
		blink_led();
		/* Toggle the LED state */
		s_led_state = !s_led_state;
		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
	}
}

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

				/* As the sensor is set in 8x8 mode by default, we have a total
				 * of 64 zones to print.
				 */

				printf("\nData\n");
				print_current_time();
				for (i = 0; i < VL53L7CX_RESOLUTION_8X8; i++)
				{
					printf("%4d ", Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
				}

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

// Set local time from Wi-Fi
void obtain_time(void);
void initialize_sntp(void);

void obtain_time(void)
{
	initialize_sntp();
	time_t now = 0;
	struct tm timeinfo = {0};
	int retry = 0;
	const int retry_count = 10;

	while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
	{
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
	}

	if (retry == retry_count)
	{
		ESP_LOGE(TAG, "Failed to obtain time");
	}
}

void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_init();
}

void vWifiTask()
{
	nvs_flash_init();
	wifi_task(NULL);
	// Obtain time after connecting to Wi-Fi
	obtain_time();
	vTaskDelete(NULL);
}
