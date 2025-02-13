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
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    int milliseconds = tv.tv_usec / 1000;

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    
	char *current_time = (char *)malloc(89 * sizeof(char));
    snprintf(current_time, 89*sizeof(char), "%s,%d", buffer, milliseconds);
	
    return current_time;
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
	} else {
		ESP_LOGI(TAG, "Time is set");
		return 1;
		//green_led_intensity = 100;
	}
}

void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_init();
}