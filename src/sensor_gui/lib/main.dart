/*
MIT License

Copyright (c) 2020 J-P Nurmi <jpnurmi@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
//
import 'dart:developer';

import 'package:flutter/material.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'package:sensor_gui/serial_port_selector.dart';
import 'package:window_size/window_size.dart';
import 'dart:convert';
import 'package:flutter/services.dart' show rootBundle;
import 'package:googleapis/sheets/v4.dart';
import 'package:googleapis_auth/auth_io.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
 // setWindowTitle('VL553L7 data visualiser');
  initGoogleAPI();
  runApp(const ExampleApp());
}

void initGoogleAPI() async {
  final credentialsJson = await rootBundle.loadString('assets/credentials/credentials.json');
    final credentials = json.decode(credentialsJson);

  // Define the scopes required
  final scopes = [SheetsApi.spreadsheetsScope];

  // Authenticate
  final client = await clientViaServiceAccount(
      ServiceAccountCredentials.fromJson(credentials), scopes);

  // Create Sheets API instance
  final sheetsApi = SheetsApi(client);

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
  await sheetsApi.spreadsheets.values
      .update(valueRange, spreadsheetId, range, valueInputOption: 'RAW');

  log('Data uploaded successfully');
  // Initialize Google API
}

class ExampleApp extends StatefulWidget {
  const ExampleApp({super.key});

  @override
  ExampleAppState createState() => ExampleAppState();
}

extension IntToString on int {
  String toHex() => '0x${toRadixString(16)}';
  String toPadded([int width = 3]) => toString().padLeft(width, '0');
  String toTransport() {
    switch (this) {
      case SerialPortTransport.usb:
        return 'USB';
      case SerialPortTransport.bluetooth:
        return 'Bluetooth';
      case SerialPortTransport.native:
        return 'Native';
      default:
        return 'Unknown';
    }
  }
}

class ExampleAppState extends State<ExampleApp> {
  var availablePorts = [];
  final ScrollController _scrollController = ScrollController();

  @override
  void initState() {
    super.initState();
    //initPorts();
  }

  void initPorts() {
    setState(() => availablePorts = SerialPort.availablePorts);
  }

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      theme: ThemeData.light(), // Light theme
      darkTheme: ThemeData.dark(), // Dark theme
      themeMode: ThemeMode.dark, // Use system theme mode

      home: Scaffold(
          appBar: AppBar(
            title: const Text('Flutter Serial Port example'),
          ),
          body: const SizedBox(
            width: 400,
            child: Text("data")//SerialPortSelector(),
          )),
    );
  }
}
