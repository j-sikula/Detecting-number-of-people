#include "google_api.h"
#include "keys.h"    //API_KEY
#include "esp_mac.h" //needed for http
#include "esp_http_client.h"
#include "lwip/sockets.h" //needed for http
#include "esp_log.h"
#include "cJSON.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");

static const char *TAG = "google_sheets";

typedef struct
{
    char *buffer;
    int buffer_len;
    int buffer_size;
} http_response_t;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *response = (http_response_t *)evt->user_data;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE("http", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI("http", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI("http", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        char header_log[256];
        snprintf(header_log, sizeof(header_log), "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        ESP_LOGI("http", "%s", header_log);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI("http", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (response->buffer == NULL)
        {
            response->buffer = malloc(evt->data_len + 1);
            response->buffer_size = evt->data_len + 1;
            response->buffer_len = 0;
        }
        else if (response->buffer_len + evt->data_len + 1 > response->buffer_size)
        {
            response->buffer_size += evt->data_len;
            response->buffer = realloc(response->buffer, response->buffer_size);
        }

        memcpy(response->buffer + response->buffer_len, evt->data, evt->data_len);
        response->buffer_len += evt->data_len;
        response->buffer[response->buffer_len] = '\0';
        /*
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            char data_log[evt->data_len + 1];
            snprintf(data_log, sizeof(data_log), "%.*s", evt->data_len, (char *)evt->data);
            ESP_LOGI("http", "%s", data_log);
        }*/
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI("http", "HTTP_EVENT_ON_FINISH\n");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI("http", "HTTP_EVENT_DISCONNECTED\n");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI("http", "HTTP_EVENT_REDIRECT\n");
        break;
    }
    return ESP_OK;
}

void send_data(const char *data)
{
    esp_http_client_config_t config = {
        .url = "https://app.izidoor.cz/getEvangelium.ashx",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        printf("HTTP GET Status = %d, content_length = %lld\n",
               esp_http_client_get_status_code(client),
               esp_http_client_get_content_length(client));
    }
    else
    {
        printf("HTTP GET request failed: %s\n", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void get_google_sheets_data(const char *spreadsheet_id, const char *range)
{
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s/values/%s?key=%s", spreadsheet_id, range, API_KEY);

    http_response_t response = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .cert_pem = (const char *)server_cert_pem_start,
        .user_data = &response,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGI(TAG, "%s", log_msg);

        printf("Response: %s\n", response.buffer);
        cJSON *json = cJSON_Parse(response.buffer);
        if (json == NULL)
        {
            ESP_LOGE(TAG, "Failed to parse JSON response");
        }
        else
        {
            cJSON *values = cJSON_GetObjectItem(json, "values");
            if (values != NULL && cJSON_IsArray(values))
            {
                int rows = cJSON_GetArraySize(values);
                for (int i = 0; i < rows; i++)
                {
                    cJSON *row = cJSON_GetArrayItem(values, i);
                    if (cJSON_IsArray(row))
                    {
                        int cols = cJSON_GetArraySize(row);
                        for (int j = 0; j < cols; j++)
                        {
                            cJSON *cell = cJSON_GetArrayItem(row, j);
                            if (cJSON_IsString(cell))
                            {
                                printf("%s\t", cell->valuestring);
                            }
                        }
                        printf("\n");
                    }
                }
            }
            cJSON_Delete(json);
        }

        // Free the response buffer
        free(response.buffer);
       
    }
    else
    {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "HTTP GET request failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", error_msg);
    }

    esp_http_client_cleanup(client);
}

/**
 * @brief Append data to a Google Sheets spreadsheet
 * https://developers.google.com/sheets/api/samples/writing#append_values
 * https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
 * 
 */
void append_google_sheets_data(const char *spreadsheet_id, const char *data, const char *sheet_name)
{
}

/**
 * @brief Create a new sheet in a Google Sheets spreadsheet
 * POST request
 * https://developers.google.com/sheets/api/samples/sheet#add_a_sheet
 * 
 * 
 */
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name)
{
}
