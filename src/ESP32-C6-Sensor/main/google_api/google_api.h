#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#include "measurement_utils/utils.h"
#include "measurement_utils/sensor.h"
#include "people_counter/people_counter.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define SPREADSHEET_ID_RAW "1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM"
#define SPREADSHEET_ID "1SMUomRFOupgDCK7eLoi8eb6Y_97LJ3NA8j68mztiyTw"
                        //header+(braces+timestamp+distance*n_zones)*loop_count 
#define JSON_APPEND_LENGTH (62+(3+26+7*VL53L7CX_RESOLUTION_8X8)*MEASUREMENT_LOOP_COUNT)
#define PEOPLE_COUNT_STR_LENGTH 34
#define JSON_UPLOAD_PEOPLE_COUNT_LENGTH (62+45+PEOPLE_COUNT_STR_LENGTH) //header+spreadsheed ID+people_count


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

/**
 * @brief Upload people count to Google Sheets
 * @param spreadsheet_id Google Sheets spreadsheet ID
 * @param data array of pointers to people count data structures
 * @param n_data size of the data array
 * @param sheet_name name of the sheet in the spreadsheet
 * @param access_token Google API access token
 */
void upload_people_count_to_google_sheets(const char *spreadsheet_id, people_count_t **data, uint8_t n_data, const char *sheet_name, const char *access_token);
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name, const char *access_token);
void update_google_sheets_data(const char *spreadsheet_id, const char *data, const char *range, const char *access_token);
esp_err_t http_event_handler(esp_http_client_event_t *evt);

#endif // GOOGLE_API_H