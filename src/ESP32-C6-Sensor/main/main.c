/**
 * @file main.c
 * @brief Main file for the ESP32 application
 * @details This file contains the main function, which starts the tasks for sensor control, Wi-Fi, SD card and resetting people count. Main function continues to LED indicator loop.
 * @author Josef Sikula
 * @license MIT
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "led_indicator/led_indicator.h"
#include "measurement_utils/sensor.h"
#include "wifi/wifi_controller.h"

#include "measurement_utils/utils.h"
#include "google_api/keys.h"
#include "google_api/google_api.h"
#include "google_api/authentication.h"
#include "people_counter/people_counter_correlation_matrix.h"

#include "sd_card/sd_card.h"

#define MIN_GOOGLE_SHEETS_UPDATE_PERIOD 10000 // 10 seconds

static char *google_api_access_token = NULL;

static QueueHandle_t data_to_sd_queue;
static QueueHandle_t data_to_google_sheets_queue;
/**
 * @brief Task for sensor control
 * @details Initializes the sensor and starts continuous measurement.
 */
void vTaskLoop();
/**
 * @brief Task for Wi-Fi connection and HTTPS requests
 * @details Connects to Wi-Fi and obtains time. Uploads data to Google Sheets.
 */
void vTaskWifi();
/**
 * @brief Task for SD card
 * @details Initializes the SD card and saves raw data from sensor and logs to it. Refreshes log file every 2 minutes.
 */
void vTaskSDCard();
/**
 * @brief Task for resetting people count
 * @details Resets people count every day between 00:00 and 00:59.
 */
void vTaskResetPeopleCounter();

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

	xTaskCreate(vTaskWifi, "wifi_task", 32 * 1024, NULL, 4, NULL);
	xTaskCreate(vTaskSDCard, "sd_card_task", 32 * 1024, NULL, 5, NULL);
	xTaskCreate(vTaskResetPeopleCounter, "rst_p_c_task", 1024, NULL, 3, NULL);
	led_loop();
}

void vTaskSDCard()
{
	char *date = get_current_date();
	init_sd_card();
	init_log_to_file(date);
	uint8_t log_file_counter = 0;
	while (true)
	{
		if (log_file_counter >= 10) // refresh log file every cca 2 minutes (after 10 times received raw data per 10s)
		{
			log_file_counter = 0;
			date = get_current_date();
			if (refresh_log_file(date) == 0)
			{
				ESP_LOGE("vTaskSDCard", "Failed to refresh log file");
			}
			free(date);
			ESP_LOGI("vTaskSDCard", "Log file refreshed");
		}
		else
		{
			log_file_counter++;
		}

		measurement_t *data;
		if (xQueueReceive(data_to_sd_queue, &data, portMAX_DELAY) == pdPASS)
		{
			ESP_LOGD("vTaskSDCard", "Received data from queue");
			date = get_current_date();
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

void vTaskWifi()
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
				ESP_LOGI("vTaskWifi", "Received data from queue");

				if (checkAccessTokenValidity(&google_api_access_token) == 0)
				{
					ESP_LOGI("vTaskWifi", "acces token before expiration, generated new one, %s", google_api_access_token);
				}
				char *date = get_current_week();
				people_count_t *data[n_data];

				for (uint8_t i = 0; i < n_data; i++)
				{
					if (xQueueReceive(data_to_google_sheets_queue, &data[i], 0) != pdPASS)
					{
						ESP_LOGE("vTaskWifi", "Failed to receive data from queue");
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

void vTaskResetPeopleCounter()
{
	while (1)
	{
		if (is_midnight() == 1)
		{
			reset_people_count();
			vTaskDelay(36000000 / portTICK_PERIOD_MS); // sleep 10 h
		}
		vTaskDelay(3600000 / portTICK_PERIOD_MS); // sleep 1 h
	}
	vTaskDelete(NULL);
}