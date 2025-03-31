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


#define MIN_GOOGLE_SHEETS_UPDATE_PERIOD 10000 // 10 seconds

static char *google_api_access_token = NULL;

static QueueHandle_t data_to_sd_queue;
static QueueHandle_t data_to_google_sheets_queue;

void vTaskLoop();
void vWifiTask();
void vTaskSDCard();

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
	led_loop();
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

			if (uxQueueMessagesWaiting(data_to_google_sheets_queue) >= 1)
			{
				vTaskDelay(MIN_GOOGLE_SHEETS_UPDATE_PERIOD / portTICK_PERIOD_MS); // wait for other possible data

				uint8_t n_data = uxQueueMessagesWaiting(data_to_google_sheets_queue);
				ESP_LOGI("vWifiTask", "Received data from queue");
				
				if(checkAccessTokenValidity(&google_api_access_token) == 0)
				{
					ESP_LOGI("vWifiTask", "acces token before expiration, generated new one, %s", google_api_access_token);
				}
				char *date = get_current_week();
				people_count_t *data[n_data];

				for (uint8_t i = 0; i < n_data; i++)
				{
					if (xQueueReceive(data_to_google_sheets_queue, &data[i], 0) != pdPASS)
					{
						ESP_LOGE("vWifiTask", "Failed to receive data from queue");
					}
				}

				upload_people_count_to_google_sheets(SPREADSHEET_ID, data, n_data, date, google_api_access_token);
				free(date);
				for (uint8_t i = 0; i < n_data; i++)
				{
					free(data[i]->timestamp);
					free(data[i]);
				}
				check_heap_memory();
			}
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
	}

	vTaskDelete(NULL);
}
