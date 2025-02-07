#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#define SPREADSHEET_ID "1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM"

#include <stdint.h>

typedef struct
{
    uint16_t *measured_values;
    char *timestamp;
} measurement_t;

void send_data(const char *data);
void get_google_sheets_data(const char *spreadsheet_id, const char *range);
void append_google_sheets_data(const char *spreadsheet_id, const char *data, const char *sheet_name);
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name);

#endif // GOOGLE_API_H