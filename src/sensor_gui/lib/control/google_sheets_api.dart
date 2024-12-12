import 'dart:developer';
import 'package:googleapis/sheets/v4.dart';
import 'package:googleapis_auth/auth_io.dart';
import 'dart:convert';
import 'package:flutter/services.dart' show rootBundle;

class GoogleSheetsApi {
  SheetsApi? sheetsApi;

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

    // Define the spreadsheet ID and range
    const spreadsheetId = '1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM';
    const range = 'Sheet1!A1';

    // Define the values to upload
    final values = [
      ['Name', 'Age', 'City'],
      ['John Doe', '30', 'New York'],
      ['Jane Smith', '25', 'Los Angeles'],
      ['Tom Brown', '40', 'Chicago'],
    ];

    // Create the value range
    final valueRange = ValueRange.fromJson({
      'range': range,
      'values': values,
    });

    // Upload the data
    await sheetsApi!.spreadsheets.values
        .update(valueRange, spreadsheetId, range, valueInputOption: 'RAW');

    log('Data uploaded successfully');
    // Initialize Google API
  }

  SheetsApi get getSheetsApi => sheetsApi!;
}
