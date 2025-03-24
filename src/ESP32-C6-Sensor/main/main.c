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
#include "led_indicator/led_indicator.h"
#include "driver/i2c.h"	 // Inter-Integrated Circuit driver
#include "driver/uart.h" // UART driver
#include "measurement_utils/sensor.h"
#include "wifi/wifi_controller.h"

#include "measurement_utils/utils.h"
#include "google_api/keys.h"
#include "google_api/google_api.h"
#include "google_api/authentication.h"
#include "people_counter/people_counter.h"

#include "sd_card/sd_card.h"

#include "esp_heap_caps.h"

static char *google_api_access_token = NULL;

static QueueHandle_t data_to_sd_queue;
static QueueHandle_t data_to_google_sheets_queue;

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

	data_to_sd_queue = xQueueCreate(10, sizeof(measurement_t *));
	if (data_to_sd_queue == NULL)
	{
		ESP_LOGE("app_main", "Failed to create queue for SD card");
		return;
	}

	data_to_google_sheets_queue = xQueueCreate(10, sizeof(people_count_t *));
	if (data_to_google_sheets_queue == NULL)
	{
		ESP_LOGE("app_main", "Failed to create queue for Google Sheets");
		return;
	}

	xTaskCreate(vTaskLoop, "forever_loop", 40 * 1024, NULL, 6, NULL);

	xTaskCreate(vWifiTask, "wifi_task", 32 * 1024, NULL, 4, NULL);
	xTaskCreate(vTaskSDCard, "sd_card_task", 32 * 1024, NULL, 5, NULL);
	
}

void vTaskSDCard()
{
	init_sd_card();

	while (true)
	{
		measurement_t *data;
		if (xQueueReceive(data_to_sd_queue, &data, portMAX_DELAY) == pdPASS)
		{
			ESP_LOGI("vTaskSDCard", "Received data from queue");
			char *date = get_current_date();
			save_raw_data(date, data);
			free(date);
			check_heap_memory();
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}

void vTaskLoop()
{
	// Initialize the sensor.
	initVL53L7CX();
	check_heap_memory();
	startContinuousMeasurement(data_to_sd_queue, data_to_google_sheets_queue);
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
		google_api_access_token = generate_access_token();
		stop_led();
		while (true)
		{
			people_count_t *data;
			if (xQueueReceive(data_to_google_sheets_queue, &data, portMAX_DELAY) == pdPASS)
			{
				ESP_LOGI("vWifiTask", "Received data from queue");
				checkAccessTokenValidity(google_api_access_token);

				char *date = get_current_week();
				upload_people_count_to_google_sheets(SPREADSHEET_ID, data, date, google_api_access_token);
				free(date);
				free(data->timestamp);
    			free(data);
				check_heap_memory();
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
	}

	vTaskDelete(NULL);
}
