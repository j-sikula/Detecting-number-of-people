#include "utils.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "esp_mac.h"
#include "esp_log.h"

static const char *TAG = "utils";

char *get_current_time(void)
{
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[DATE_TIME_LENGTH - 3];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint16_t milliseconds = tv.tv_usec / 1000;

	strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

	char *current_time = (char *)malloc(DATE_TIME_LENGTH + 4);
	snprintf(current_time, DATE_TIME_LENGTH + 3, "%s,%d", buffer, milliseconds);

	return current_time;
}

char *get_current_date(void)
{
	time_t rawtime;
	struct tm *timeinfo;
	char *buffer = (char *)malloc(11 * sizeof(char));

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 11, "%Y%m%d", timeinfo);

	return buffer;
}

char *get_current_week(void)
{
    time_t rawtime;
	struct tm *timeinfo;
	char *buffer = (char *)malloc(11 * sizeof(char));

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 11, "%G-W%V", timeinfo);

	return buffer;
}

uint8_t obtain_time(void)
{
	initialize_sntp();
	time_t now = 0;
	struct tm timeinfo = {0};
	int retry = 0;
	const int retry_count = 10;

	while (timeinfo.tm_year < (2016 - 1900))	// wait for time to be set
	{

		ESP_LOGI(TAG, "Waiting for system time to be set...");
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
		if (retry++ >= retry_count)
		{
			vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	ESP_LOGI(TAG, "Time is set");
	return 1;
}

void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_init();
}

char *measurement_array_to_string(measurement_t *measurement)
{
	char *buffer = (char *)malloc(MEASUREMENT_IN_CHAR_LENGTH * sizeof(char));
	if (buffer == NULL)
	{
		ESP_LOGE("measurement_array_to_string", "Failed to allocate memory");
		return NULL;
	}

	snprintf(buffer, DATE_TIME_LENGTH + 2, "%s", measurement->timestamp);
	free(measurement->timestamp);
	char temp[5];

	for (int j = 0; j < N_PIXELS; j++)
	{
		snprintf(temp, DATE_TIME_LENGTH, ";%d", measurement->distance_mm[j]);
		strcat(buffer, temp);
	}
	for (int j = 0; j < N_PIXELS; j++)
	{
		snprintf(temp, DATE_TIME_LENGTH, ";%d", measurement->status[j]);
		strcat(buffer, temp);
	}

	strcat(buffer, "\n");

	return buffer;
}

void check_heap_memory()
{
	size_t free_heap_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	ESP_LOGI("Heap", "Free heap size: %d bytes", free_heap_size);
}