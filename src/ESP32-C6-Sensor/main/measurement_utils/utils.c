#include "utils.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "esp_mac.h"
#include "esp_log.h"

static const char *TAG = "utils";
static uint8_t is_midnight_not_called = 1;
uint8_t is_midnight()
{

	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if (timeinfo->tm_hour == 0)
	{
		if (is_midnight_not_called)
		{
			is_midnight_not_called = 0;
			return 1;
		}
	}
	else
	{
		is_midnight_not_called = 1;
	}

	return 0;
}

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

	while (timeinfo.tm_year < (2016 - 1900)) // wait for time to be set
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

static FILE *log_file = NULL;

void init_log_to_file(char *log_file_name)
{
	char filepath[256];
	snprintf(filepath, sizeof(filepath), "/sdcard/%s", log_file_name);

	char *extension = strchr(log_file_name, '.');
	if (extension == NULL) // extension is not present
	{
		strcat(filepath, ".log");
	}
	log_file = fopen(filepath, "a");
	if (log_file == NULL)
	{
		ESP_LOGE("Log", "Failed to open log file");
	}
	else
	{
		ESP_LOGI("Log", "Log file opened successfully");
		esp_log_set_vprintf(custom_log_to_file);
	}
}

int custom_log_to_file(const char *fmt, va_list args)
{
	// Write log to the console (optional)
	vprintf(fmt, args);

	// Write log to the file
	if (log_file != NULL)
	{
		vfprintf(log_file, fmt, args);
		fflush(log_file); // Ensure the log is written immediately
	}

	return 0;
}

uint8_t refresh_log_file(char *log_file_name)
{
	if (log_file != NULL)
	{
		char filepath[256];
		snprintf(filepath, sizeof(filepath), "/sdcard/%s", log_file_name);

		char *extension = strchr(log_file_name, '.');
		if (extension == NULL) // extension is not present
		{
			strcat(filepath, ".log");
		}
		fclose(log_file);
		log_file = fopen(filepath, "a");
		if (log_file == NULL)
		{
			ESP_LOGE("Log", "Failed to open log file, logging set to console");
			esp_log_set_vprintf(vprintf);
		}
		else
		{
			esp_log_set_vprintf(custom_log_to_file);
			return 1;
		}

		ESP_LOGI("Log", "Log file closed");
	}
	return 0;
}

uint8_t close_log_file(void)
{
	if (log_file != NULL)
	{
		fclose(log_file);
		log_file = NULL;
		ESP_LOGI("Log", "Log file closed");
	}
	return 0;
}
