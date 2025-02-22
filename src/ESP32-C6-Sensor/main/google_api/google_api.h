#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#include "measurement_utils/utils.h"
#include "measurement_utils/sensor.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define SPREADSHEET_ID "1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM"
                        //header+(braces+timestamp+distance*n_zones)*loop_count 
#define JSON_APPEND_LENGTH (62+(3+26+7*VL53L7CX_RESOLUTION_8X8)*MEASUREMENT_LOOP_COUNT)

// Number of retries for a failed request
#define REQUEST_RETRIES 1   

typedef struct
{
    char *buffer;
    int buffer_len;
    int buffer_size;
} http_response_t;


uint16_t _send_api_request(const char *url, esp_http_client_method_t method, char *data, int tx_buffer_size, const char *access_token);
void get_google_sheets_data(const char *spreadsheet_id, const char *range);
void append_google_sheets_data(const char *spreadsheet_id, measurement_t *data, const char *sheet_name, const char *access_token);
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name, const char *access_token);
void update_google_sheets_data(const char *spreadsheet_id, const char *data, const char *range, const char *access_token);
esp_err_t http_event_handler(esp_http_client_event_t *evt);

#endif // GOOGLE_API_H