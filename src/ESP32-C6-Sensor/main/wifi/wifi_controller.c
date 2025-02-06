#include "wifi_controller.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "WiFi";
int retry_num = 0;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi lost connection\n");
        if (retry_num < 5)
        {
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Retrying to Connect...\n");
        }
        else
        {

            vTaskDelay(30000 / portTICK_PERIOD_MS);
            esp_wifi_connect();
            ESP_LOGI(TAG, "Retrying to Connect...\n");
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "Wifi got IP...\n\n");
    }
}

void wifi_task(void *pvParameters)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
}
