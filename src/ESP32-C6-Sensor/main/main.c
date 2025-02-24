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
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/i2c.h"	 // Inter-Integrated Circuit driver
#include "driver/uart.h" // UART driver
#include "measurement_utils/sensor.h"
#include "wifi/wifi_controller.h"

#include "measurement_utils/utils.h"
#include "google_api/keys.h"
#include "google_api/google_api.h"
#include "google_api/authentication.h"

#include "sd_card/sd_card.h"

#include "esp_heap_caps.h"

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static uint8_t green_led_intensity = 16;

static void blink_led(void)
{
	/* If the addressable LED is enabled */
	if (s_led_state)
	{
		/* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
		led_strip_set_pixel(led_strip, 0, 16, green_led_intensity, 16);
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
#endif
	/* Set all LED off to clear all pixels */
	led_strip_clear(led_strip);
}

#endif

static QueueHandle_t measurementQueue;

void vTaskLoop();
void vWifiTask();
void vTaskSDCard();

void check_heap_memory()
{
	size_t free_heap_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	ESP_LOGI("Heap", "Free heap size: %d bytes", free_heap_size);
}

void app_main(void)
{

	/* Configure the peripheral according to the LED type */
	configure_led();

	measurementQueue = xQueueCreate(10, sizeof(measurement_t *));
	if (measurementQueue == NULL)
	{
		ESP_LOGE("app_main", "Failed to create queue");
		return;
	}

	// Start the i2c scanner task

	//xTaskCreate(vTaskLoop, "forever_loop", 40 * 1024, NULL, 5, NULL);

	//xTaskCreate(vWifiTask, "wifi_task", 32 * 1024, NULL, 4, NULL);
	xTaskCreate(vTaskSDCard, "sd_card_task", 32* 1024, NULL, 4, NULL);
	while (1)
	{
		// ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
		blink_led();
		/* Toggle the LED state */
		s_led_state = !s_led_state;
		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
	}
}

void vTaskSDCard()
{
	init_sd_card();
	vTaskDelete(NULL);
}

void vTaskLoop()
{
	// Initialize the sensor.
	initVL53L7CX();
	check_heap_memory();
	startContinuousMeasurement(measurementQueue);
	// Delete this task if it exits from the loop above
	vTaskDelete(NULL);
}

void vWifiTask()
{
	nvs_flash_init();
	wifi_task(NULL);
	// Obtain time after connecting to Wi-Fi
	if (obtain_time())
	{
		get_google_sheets_data(SPREADSHEET_ID, "Sheet1!A1:B2");
		char *access_token = generate_access_token();
		if (access_token != NULL)
		{
			create_new_sheet(SPREADSHEET_ID, "2025_02_18", access_token);
			update_google_sheets_data(SPREADSHEET_ID, get_current_time(), "Sheet2!C1", access_token);

			while (true)
			{
				measurement_t *receivedMeasurement;
				if (xQueueReceive(measurementQueue, &receivedMeasurement, portMAX_DELAY) == pdPASS)
				{
					ESP_LOGI("measurement", "Received measurement %s\n", receivedMeasurement->timestamp);
					char *date = get_current_date();
					append_google_sheets_data(SPREADSHEET_ID, receivedMeasurement, date, access_token);
					free(date);
					check_heap_memory();
				}
				vTaskDelay(500 / portTICK_PERIOD_MS);
			}
		}
	}

	vTaskDelete(NULL);
}
