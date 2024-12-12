#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H


#define WIFI_SSID "WifiSSID"
#define WIFI_PASS "wifipassword"
#define CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM 10

void wifi_task(void *pvParameters);


#endif // WIFI_CONTROLLER_H