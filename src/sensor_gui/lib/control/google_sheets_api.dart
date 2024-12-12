import 'dart:developer';
import 'package:googleapis/sheets/v4.dart';
import 'package:googleapis_auth/auth_io.dart';
import 'dart:convert';
import 'package:flutter/services.dart' show rootBundle;

class GoogleSheetsApi {
  SheetsApi? sheetsApi;
  final spreadsheetId = '1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM';

  void initGoogleAPI() async {
    final credentialsJson =
        await rootBundle.loadString('assets/credentials/credentials.json');
    final credentials = json.decode(credentialsJson);

    // Define the scopes required
    final scopes = [SheetsApi.spreadsheetsScope];

    // Authenticate
    final client = await clientViaServiceAccount(
        ServiceAccountCredentials.fromJson(credentials), scopes);

    // Create Sheets API instance
    sheetsApi = SheetsApi(client);
  }

  SheetsApi get getSheetsApi => sheetsApi!;

  Future<void> appendData(List<List<String>> newData) async {
    if (sheetsApi == null) {
      log('Google Sheets API is not initialized');
      return;
    }

    /// Update last upload time

    String range = 'Sheet1!C1';

    // Define the values to upload
    final values = [
      [DateTime.now().toIso8601String()],
    ];

    // Create the value range
    ValueRange valueRange = ValueRange.fromJson({
      'range': range,
      'values': values,
    });

    // Upload the data
    await sheetsApi!.spreadsheets.values
        .update(valueRange, spreadsheetId, range, valueInputOption: 'RAW');

    log('Last upload time updated successfully');

    range = 'Sheet1';

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
  }
}
