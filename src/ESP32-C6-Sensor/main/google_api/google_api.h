#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#include "measurement_utils/measurement_utils.h"
#include "esp_http_client.h"

#define SPREADSHEET_ID "1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM"

typedef struct
{
    char *buffer;
    int buffer_len;
    int buffer_size;
} http_response_t;

void send_data(const char *data);
void get_google_sheets_data(const char *spreadsheet_id, const char *range);
void append_google_sheets_data(const char *spreadsheet_id, measurement_t *data, const char *sheet_name);
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name);
void update_google_sheets_data(const char *spreadsheet_id, const char *data, const char *range, const char *access_token);
esp_err_t http_event_handler(esp_http_client_event_t *evt);

#endif // GOOGLE_API_H