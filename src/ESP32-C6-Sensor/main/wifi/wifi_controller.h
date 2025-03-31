#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "google_api/keys.h" // WIFI_SSID, WIFI_PASS

#define CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM 10

void wifi_task(void *pvParameters);


#endif // WIFI_CONTROLLER_H