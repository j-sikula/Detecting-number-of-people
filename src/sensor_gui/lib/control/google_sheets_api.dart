import 'dart:developer';
import 'package:googleapis/sheets/v4.dart';
import 'package:googleapis_auth/auth_io.dart';
import 'dart:convert';
import 'package:flutter/services.dart' show rootBundle;

class GoogleSheetsApi {
  SheetsApi? sheetsApi;
  final String spreadsheetId;
  bool
      isSheetForRawData; // sheet for raw data has different header (65 columns)
  GoogleSheetsApi(this.spreadsheetId, this.isSheetForRawData);

  void initGoogleAPI() async {
    final credentialsJson =
        await rootBundle.loadString('assets/credentials/credentials.json');
    final credentials = json.decode(credentialsJson);

    // Define the scopes required
    final scopes = [SheetsApi.spreadsheetsScope];
    AutoRefreshingAuthClient? client;
    try {
      // Authenticate
      client = await clientViaServiceAccount(
          ServiceAccountCredentials.fromJson(credentials), scopes);
    } catch (e) {
      log('Failed to authenticate: $e');
      return;
    }
    // Create Sheets API instance
    sheetsApi = SheetsApi(client);
  }

  SheetsApi get getSheetsApi => sheetsApi!;

  Future<void> appendData(List<List<String>> newData) async {
    if (sheetsApi == null) {
      log('Google Sheets API is not initialized');
      return;
    }

    // Prepare the sheet for the day if it does not exist
    bool isSheetPrepared = await createSheetIfNotExists(getCurrentDate()); 

    if (!isSheetPrepared) {
      log('Failed to prepare the sheet');
      return;
    }

    // Update last upload time
    String range = '${getCurrentDate()}!C1';

    // Define the values to upload
    final values = [
      [DateTime.now().toIso8601String()],
    ];

    // Create the value range
    ValueRange valueRange = ValueRange.fromJson({
      'range': range,
      'values': values,
    });
    try {
      // Upload the data
      await sheetsApi!.spreadsheets.values
          .update(valueRange, spreadsheetId, range, valueInputOption: 'RAW');

      log('Last upload time updated successfully');

      range = getCurrentDate(); // sheet name

      valueRange = ValueRange.fromJson({
        'range': range,
        'values': newData,
      });

      await sheetsApi!.spreadsheets.values.append(
        valueRange,
        spreadsheetId,
        range,
        valueInputOption: 'RAW',
      );

      log('Data appended successfully');
    } catch (e) {
      log('Failed to append data: $e');
    }
  }

  Future<void> appendDataRow(List<String> data) async {
    return appendData([data]);
  }

  Future<bool> createSheetIfNotExists(String sheetTitle) async {
    if (sheetsApi == null) {
      log('Google Sheets API is not initialized');
      return false;
    }

    try {
      // Check if the sheet exists
      final spreadsheet = await sheetsApi!.spreadsheets.get(spreadsheetId);
      final sheetExists = spreadsheet.sheets!
          .any((sheet) => sheet.properties!.title == sheetTitle);

      if (!sheetExists) {
        // Create the sheet
        final request = {
          'addSheet': {
            'properties': {
              'title': sheetTitle,
            },
          },
        };

        final batchUpdateRequest = BatchUpdateSpreadsheetRequest.fromJson({
          'requests': [request],
        });

        await sheetsApi!.spreadsheets
            .batchUpdate(batchUpdateRequest, spreadsheetId);
        log('Sheet "$sheetTitle" created successfully');

        await prepareHeader(sheetTitle);
      } else {
        return true;
      }
    } catch (e) {
      log('Failed to create sheet: $e');
      return false;
    }
    return true;
  }

  /// inserts the header to the sheet
  /// throws an error if the header is not prepared
  Future<void> prepareHeader(String sheetName) {
    if (sheetsApi == null) {
      log('Google Sheets API is not initialized');
      return Future.value();
    }

    String range = '$sheetName!A1';

    // Define the values to upload
    final List<List<String>> values;
    if (isSheetForRawData){
      values = [
      [
        'Data measured on VL53L7CX',
        'Last Upload'
      ],
    ];
    } else {
      values = [
      [
        'Processed data',
        'Last Upload'
      ],
    ];
    }
    
    // Create the value range
    ValueRange valueRange = ValueRange.fromJson({
      'range': range,
      'values': values,
    });

    // Upload the data
    sheetsApi!.spreadsheets.values
        .update(valueRange, spreadsheetId, range, valueInputOption: 'RAW');

    log('Last upload time updated successfully');

    final headerRangeSecond = '$sheetName!A2';
    List<String> zonesHeaders;
    if (isSheetForRawData) {
      zonesHeaders = List.generate(64, (index) => 'Zone $index');
    } else {
      zonesHeaders = ["People count"];
    }
    final headerValuesSecond = [
      ['UTC time', ...zonesHeaders], // UTC time and zone headers or label people count
    ];

    final headerValueRangeSecond = ValueRange.fromJson({
      'range': headerRangeSecond,
      'values': headerValuesSecond,
    });

    return sheetsApi!.spreadsheets.values.update(
      headerValueRangeSecond,
      spreadsheetId,
      headerRangeSecond,
      valueInputOption: 'RAW',
    );
  }

  /// Returns the current date in the format 'year_month_day'
  /// used for sheet names
  String getCurrentDate() {
    String year = DateTime.now().year.toString();
    String month = DateTime.now().month.toString();
    String day = DateTime.now().day.toString();
    return '${year}_${month}_$day';
  }
}
