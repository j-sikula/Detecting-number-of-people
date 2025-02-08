#include "measurement_utils.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "esp_mac.h"

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

    static char current_time[89];
    snprintf(current_time, sizeof(current_time), "%s,%d", buffer, milliseconds);
    return current_time;
}