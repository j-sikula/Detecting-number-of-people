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
	char buffer[DATE_TIME_LENGTH-4];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint16_t milliseconds = tv.tv_usec / 1000;

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

	char *current_time = (char *)malloc(DATE_TIME_LENGTH + 10);
	snprintf(current_time, DATE_TIME_LENGTH+2, "%s,%d", buffer, milliseconds);

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

uint8_t obtain_time(void)
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
		return 0;
	}
	else
	{
		ESP_LOGI(TAG, "Time is set");
		return 1;
		// green_led_intensity = 100;
	}
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

	snprintf(buffer, DATE_TIME_LENGTH+2, "%s", measurement->timestamp);
	free(measurement->timestamp);
	char temp[5];

	for (int j = 0; j < N_ZONES; j++)
	{
		snprintf(temp, DATE_TIME_LENGTH, ";%d", measurement->distance_mm[j]);
		strcat(buffer, temp);
	}
	for (int j = 0; j < N_ZONES; j++)
	{
		snprintf(temp, DATE_TIME_LENGTH, ";%d", measurement->status[j]);
		strcat(buffer, temp);
	}

	strcat(buffer, "\n");

	return buffer;
}