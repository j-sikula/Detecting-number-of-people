#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#define API_KEY "AIzaSyBRSImiIIZhZtTvLrQC2wREWae408JVIJc"
#define SPREADSHEET_ID "1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM"

void send_data(const char *data);
void get_google_sheets_data(const char *spreadsheet_id, const char *range);

#endif // GOOGLE_API_H