#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#include "measurement_utils/measurement_utils.h"

#define SPREADSHEET_ID "1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM"


void send_data(const char *data);
void get_google_sheets_data(const char *spreadsheet_id, const char *range);
void append_google_sheets_data(const char *spreadsheet_id, measurement_t *data, const char *sheet_name);
void create_new_sheet(const char *spreadsheet_id, const char *sheet_name);
void update_google_sheets_data(const char *spreadsheet_id, const char *data, const char *range, const char *access_token);
#endif // GOOGLE_API_H