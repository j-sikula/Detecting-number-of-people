#include "google_api.h"
#include "keys.h"    //API_KEY
#include "esp_mac.h" //needed for http
#include "esp_wifi.h" //needed for esp_wifi_is_connected
#include "esp_http_client.h"
#include "lwip/sockets.h" //needed for http
#include "esp_log.h"
#include "cJSON.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");

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

void get_google_sheets_data(const char *spreadsheet_id, const char *range)
{
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s/values/%s?key=%s", spreadsheet_id, range, API_KEY);

    http_response_t response = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
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
void append_google_sheets_data(const char *spreadsheet_id, measurement_t *data, const char *sheet_name, const char *access_token)
{
    static uint8_t n_request_repeated = 0;
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s/values/%s:append?valueInputOption=RAW", spreadsheet_id, sheet_name);
    char *dataJSON = (char *)malloc(JSON_APPEND_LENGTH * sizeof(char));
    snprintf(dataJSON, JSON_APPEND_LENGTH * sizeof(char), "{\"range\":\"%s\",\"majorDimension\":\"ROWS\",\"values\":[", sheet_name);

    for (int i = 0; i < MEASUREMENT_LOOP_COUNT; i++)
    {
        char row_buffer[1024];
        snprintf(row_buffer, sizeof(row_buffer), "[\"%s\"", data[i].timestamp);

        for (int j = 0; j < N_PIXELS; j++)
        {
            char value_buffer[16];
            snprintf(value_buffer, sizeof(value_buffer), ",%d", data[i].distance_mm[j]);
            strncat(row_buffer, value_buffer, sizeof(row_buffer) - strlen(row_buffer) - 1);
        }

        strncat(row_buffer, "]", sizeof(row_buffer) - strlen(row_buffer) - 1);

        if (i < MEASUREMENT_LOOP_COUNT - 1)
        {
            strncat(row_buffer, ",", sizeof(row_buffer) - strlen(row_buffer) - 1);
        }

        strncat(dataJSON, row_buffer, JSON_APPEND_LENGTH * sizeof(char) - strlen(dataJSON) - 1);
        free(data[i].timestamp);
    }

    strncat(dataJSON, "]}", JSON_APPEND_LENGTH * sizeof(char) - strlen(dataJSON) - 1);

    uint8_t status_code = _send_api_request(url, HTTP_METHOD_POST, dataJSON, 30 * 1024, access_token);

    // when sheet does not exist, create a new sheet and retry the request
    if (status_code != 200)
    {
        if (n_request_repeated < REQUEST_RETRIES)
        {
            n_request_repeated++;
            ESP_LOGI(TAG, "Creating a new Sheet and retrying the request");
            create_new_sheet(spreadsheet_id, sheet_name, access_token);
            append_google_sheets_data(spreadsheet_id, data, sheet_name, access_token);
        }
    }
    else
    {
        // reset the counter when the request is successful
        n_request_repeated = 0;
    }
    //free(data); //must be freed in the calling function due to recursion (request retries)
}

void upload_people_count_to_google_sheets(const char *spreadsheet_id, people_count_t *data, const char *sheet_name, const char *access_token)
{
    static uint8_t n_request_repeated = 0;
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s/values/%s:append?valueInputOption=RAW", spreadsheet_id, sheet_name);
    char *dataJSON = (char *)malloc(JSON_UPLOAD_PEOPLE_COUNT_LENGTH * sizeof(char));

    snprintf(dataJSON, JSON_UPLOAD_PEOPLE_COUNT_LENGTH * sizeof(char), "{\"range\":\"%s\",\"majorDimension\":\"ROWS\",\"values\":[[\"%s\",%d]]}", sheet_name, data->timestamp, data->people_count);
    
    uint8_t status_code = _send_api_request(url, HTTP_METHOD_POST, dataJSON, 30 * 1024, access_token);

    // when sheet does not exist, create a new sheet and retry the request
    if (status_code != 200)
    {
        if (n_request_repeated < REQUEST_RETRIES)
        {
            n_request_repeated++;
            ESP_LOGI(TAG, "Creating a new Sheet and retrying the request");
            create_new_sheet(spreadsheet_id, sheet_name, access_token);
            upload_people_count_to_google_sheets(spreadsheet_id, data, sheet_name, access_token);
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

void update_google_sheets_data(const char *spreadsheet_id, const char *data, const char *range, const char *access_token)
{
    char url[512];
    snprintf(url, sizeof(url), "https://sheets.googleapis.com/v4/spreadsheets/%s/values/%s?valueInputOption=USER_ENTERED&key=%s", spreadsheet_id, range, API_KEY);

    cJSON *values = cJSON_CreateArray();
    cJSON *row = cJSON_CreateArray();
    cJSON_AddItemToArray(row, cJSON_CreateString(data));
    cJSON_AddItemToArray(values, row);
    // Create JSON payload
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "range", range);
    cJSON_AddStringToObject(root, "majorDimension", "ROWS");
    cJSON_AddItemToObject(root, "values", values);

    char *put_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    http_response_t response = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .cert_pem = (const char *)server_cert_pem_start,
        .user_data = &response,
        .buffer_size = 4 * 1024,
        .buffer_size_tx = 4 * 1024,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_header[1040];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", access_token);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_method(client, HTTP_METHOD_PUT);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, put_data, strlen(put_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "HTTP PUT Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGI("Google Sheets", "%s", log_msg);

        ESP_LOGI(TAG, "Response: %s\n", response.buffer);

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
        }
        else
        {
            // Handle the response as needed
            cJSON_Delete(json);
        }

        // Free the response buffer
        free(response.buffer);
    }
    else
    {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "HTTP PUT request failed: %s", esp_err_to_name(err));
        ESP_LOGE("Google Sheets", "%s", error_msg);
    }

    free(put_data);
    esp_http_client_cleanup(client);
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