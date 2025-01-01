import 'dart:async';
import 'dart:developer';
import 'dart:io';
import 'dart:typed_data';

import 'package:intl/intl.dart';
import 'package:sensor_gui/control/google_sheets_api.dart';
import 'package:sensor_gui/control/people_counter.dart';

/// Decodes the received data
/// stores the data in a list of [Measurement]
/// used in the [SerialPortHandler] class
class DataDecoder {
  String previousData = '';
  List<Measurement> measurements = []; // List of non backed-up measurements
  List<Measurement> allMeasurements = []; // List of all measurements
  GoogleSheetsApi apiRawDataCloud = GoogleSheetsApi('1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM', true);
  GoogleSheetsApi apiPeopleCounter = GoogleSheetsApi('1SMUomRFOupgDCK7eLoi8eb6Y_97LJ3NA8j68mztiyTw', false);
  late PeopleCounter peopleCounter;
  DataDecoder() {

    apiRawDataCloud.initGoogleAPI();
    Timer.periodic(const Duration(seconds: 10), (timer) {
      uploadDataToGoogleSheets();
    });
    apiPeopleCounter.initGoogleAPI();
    peopleCounter = PeopleCounter(apiPeopleCounter);
  }

  Measurement? decode(Uint8List data) {
    previousData += String.fromCharCodes(
        data); // Append the received data to the previous data
    String currentData = '';
    if (previousData.split('Data').length > 2) {
      // Checks if there is at least one complete set of data
      currentData = previousData.split('Data')[
          1]; // Extract the current data from the previous data between 'Data' strings
      previousData =
          'Data${previousData.split('Data').last}'; // Update the previous data with the remaining data, some data can be lost
    }
    if (currentData.isNotEmpty) {
      List<String> dataLines = currentData.split('\n');
      DateTime timeOfMeasurement = DateTime.now();
      if (dataLines.length > 1) {
        if (dataLines.length > 2) { // to remove lines without information (with less than 10 characters)
          List<String> tempDataLines = dataLines;
          dataLines = [];
          for (String line in tempDataLines) {
            if (line.length > 10) {
              dataLines.add(line);
            }
          }
        }
        // Check if there is 2 complete line of data
        currentData =
            dataLines[1]; // Extract the current data from the previous data
        String time = dataLines[0]; // Extract the time data
        try {
          timeOfMeasurement = DateFormat("yyyy-MM-dd HH:mm:ss,SSS")
              .parse(time); // Parse the time data
        } catch (e) {
          log('Failed to parse time data: $time');
        }
      }
      List<int> decodedData = currentData
          .replaceAll(RegExp(r'\s+'),
              ' ') // Replace multiple spaces with a single space
          .split(' ')
          .skip(1)
          .map((e) => int.tryParse(e) ?? 0) // Convert the data to integers
          .toList();
      if (decodedData.length >= 64) {
        // Check if the data has at least 64 elements
        decodedData = decodedData.take(64).toList();
        Measurement currentMeasurement =
            Measurement(decodedData, timeOfMeasurement);
        measurements.add(currentMeasurement);
        allMeasurements.add(currentMeasurement);
        Measurement toReturn = Measurement(peopleCounter.processMeasurement(currentMeasurement), timeOfMeasurement);
        
        return toReturn;
      }
    }
    return null;
  }

  /// Saves the measured data to a file at [path]
  /// The first row contains the header
  /// After saving the data, the measured data is cleared

  void saveDataToFile(String path) async {
    if (path.contains('.') == false) {
      // Checks if the path contains the file extension
      path = '$path.csv'; // Adds the file extension csv if not present
    }

    final file = File(path);
    try {
      final sink = file.openWrite();
      sink.write('Data measured on VL53L7CX;Saved;${DateTime.now()}\n');
      // Write the header to the file
      sink.write('Time;');
      for (var i = 0; i < 64; i++) {
        sink.write('Zone $i;');
      }
      sink.write('\n');
      // Write the data to the file
      for (var measurement in allMeasurements) {
        sink.write('${measurement.time.toIso8601String()};');
        for (var data in measurement.data) {
          sink.write('$data;');
        }
        sink.write('\n');
      }
      await sink.flush();
      await sink.close();
      
      log('Data saved to file successfully');
    } catch (e) {
      log('Failed to save data to file: $e');
    }
  }

  void uploadDataToGoogleSheets() {
    List<List<String>> parsedMeasurements = []; // all data to be uploaded
    for (Measurement measurement in measurements) {
      List<String> parsedMeasurement = [];  // one row of data
      parsedMeasurement.add(measurement.time.toIso8601String());
      parsedMeasurement
          .addAll(measurement.data.map((e) => e.toString()).toList());
      parsedMeasurements.add(parsedMeasurement);
    }
   
    apiRawDataCloud.appendData(parsedMeasurements);
    measurements.clear();
  }

  Measurement? getLatestMeasurement() {
    if (allMeasurements.isNotEmpty) {
      return allMeasurements.last;
    }
    return null;
  }
}

/// Represents a measurement
class Measurement {
  final List<int> data;
  final DateTime time;

  Measurement(this.data, this.time);
}
