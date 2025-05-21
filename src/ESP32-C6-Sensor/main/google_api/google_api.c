/**
 * @file google_api.c
 * @brief Google Sheets API functions for uploading People Count data and creating new sheets
 * @copyright (c) 2025 Brno University of Technology
 * @author Josef Sikula
 * @license MIT
 */

#include "google_api.h"
#include "keys.h"     //API_KEY
#include "esp_mac.h"  //needed for http
#include "esp_http_client.h"
#include "lwip/sockets.h" //needed for http
#include "esp_log.h"
#include "cJSON.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_server_api_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_api_cert_pem_end");

static const char *TAG = "google_sheets";

esp_err_t http_event_handler(esp_http_client_event_t *evt)
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

        if (response->buffer == NULL)
        {
            ESP_LOGE("http", "Failed to allocate memory for response buffer");
            return ESP_FAIL;
        }

        memcpy(response->buffer + response->buffer_len, evt->data, evt->data_len);
        response->buffer_len += evt->data_len;
        response->buffer[response->buffer_len] = '\0';
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



void upload_people_count_to_google_sheets(const char *spreadsheet_id, people_count_t **data, uint8_t n_data, const char *sheet_name, const char *access_token)
{
    static uint8_t n_request_repeated = 0;
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s/values/%s:append?valueInputOption=RAW", spreadsheet_id, sheet_name);
    char *dataJSON = (char *)malloc((JSON_UPLOAD_PEOPLE_COUNT_LENGTH + (n_data - 1) * PEOPLE_COUNT_STR_LENGTH) * sizeof(char));

    snprintf(dataJSON, JSON_UPLOAD_PEOPLE_COUNT_LENGTH * sizeof(char), "{\"range\":\"%s\",\"majorDimension\":\"ROWS\",\"values\":[[\"%s\",%d]", sheet_name, data[0]->timestamp, data[0]->people_count);

    for (uint8_t i = 1; i < n_data; i++)
    {
        char row_buffer[PEOPLE_COUNT_STR_LENGTH]; // example: ,["2025-03-28 10:09:44,283",12]
        snprintf(row_buffer, sizeof(row_buffer), ",[\"%s\",%d]", data[i]->timestamp, data[i]->people_count);
        strncat(dataJSON, row_buffer, (JSON_UPLOAD_PEOPLE_COUNT_LENGTH + (n_data - 1) * PEOPLE_COUNT_STR_LENGTH) * sizeof(char) - strlen(dataJSON) - 1);
    }
    strncat(dataJSON, "]}", (JSON_UPLOAD_PEOPLE_COUNT_LENGTH + (n_data - 1) * PEOPLE_COUNT_STR_LENGTH) * sizeof(char) - strlen(dataJSON) - 1);
    ESP_LOGI(TAG, "Data JSON: %s", dataJSON);
    uint8_t status_code = _send_api_request(url, HTTP_METHOD_POST, dataJSON, 3 * 1024, access_token);

    // when sheet does not exist, create a new sheet and retry the request
    if (status_code != 200)
    {
        if (n_request_repeated < REQUEST_RETRIES)
        {
            n_request_repeated++;
            ESP_LOGI(TAG, "Creating a new Sheet and retrying the request");
            create_new_sheet(spreadsheet_id, sheet_name, access_token);
            upload_people_count_to_google_sheets(spreadsheet_id, data, n_data, sheet_name, access_token);
        }
    }
    else
    {
        // reset the counter when the request is successful
        n_request_repeated = 0;
    }
}

/**
 * @brief Create a new sheet in a Google Sheets spreadsheet
 * POST request
 * https://developers.google.com/sheets/api/samples/sheet#add_a_sheet
 *
 *
 */
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name, const char *access_token)
{
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s:batchUpdate", spreadsheet_id);

    cJSON *root = cJSON_CreateObject();
    cJSON *requests = cJSON_CreateArray();
    cJSON *addSheetRequest = cJSON_CreateObject();
    cJSON *addSheet = cJSON_CreateObject();
    cJSON *properties = cJSON_CreateObject();

    cJSON_AddStringToObject(properties, "title", sheet_name);
    cJSON_AddItemToObject(addSheet, "properties", properties);
    cJSON_AddItemToObject(addSheetRequest, "addSheet", addSheet);
    cJSON_AddItemToArray(requests, addSheetRequest);
    cJSON_AddItemToObject(root, "requests", requests);
    char *data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    _send_api_request(url, HTTP_METHOD_POST, data, 4 * 1024, access_token);
}


uint16_t _send_api_request(const char *url, esp_http_client_method_t method, char *data, int tx_buffer_size, const char *access_token)
{

    if (data == NULL)
    {
        ESP_LOGE("API", "Failed to load JSON payload");
        return 400;
    }

    http_response_t response = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .cert_pem = (const char *)server_cert_pem_start,
        .user_data = &response,
        .buffer_size = 4 * 1024,
        .buffer_size_tx = tx_buffer_size,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_header[1040];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", access_token);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_method(client, method);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, data, strlen(data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        if (esp_http_client_get_status_code(client) != 200)
        {
            char log_msg[100];
            snprintf(log_msg, sizeof(log_msg), "HTTP Status = %d, content_length = %lld",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));
            ESP_LOGI("Google Sheets", "%s", log_msg);
            ESP_LOGI(TAG, "Response: %s\n", response.buffer);
        }
        else
        {
            ESP_LOGI("API", "Request successful - 200");
        }

        // Parse the JSON response
        cJSON *json = cJSON_Parse(response.buffer);
        if (json == NULL)
        {
            ESP_LOGE("Google Sheets", "Failed to parse JSON response");
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL)
            {
                ESP_LOGE("Google Sheets", "Error before: %s", error_ptr);
            }
            cJSON_Delete(json);
        }
        else
        {
            // Handle the response as needed
            cJSON_Delete(json);
        }
    }
    else
    {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "HTTP PUT request failed: %s", esp_err_to_name(err));
        ESP_LOGE("Google Sheets", "%s", error_msg);
    }
    // Free the response buffer
    free(response.buffer);
    free(data);
    esp_http_client_cleanup(client);
    return esp_http_client_get_status_code(client);
}