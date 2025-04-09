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
  GoogleSheetsApi apiRawDataCloud =
      GoogleSheetsApi('1TzPddcXQPqZVjk_19nel91hl8BTlgOg8bBRZ543iEuM', true);
  GoogleSheetsApi apiPeopleCounter =
      GoogleSheetsApi('1SMUomRFOupgDCK7eLoi8eb6Y_97LJ3NA8j68mztiyTw', false);
  late PeopleCounter peopleCounter;
  DataDecoder() {
    apiRawDataCloud.initGoogleAPI();
    Timer.periodic(const Duration(seconds: 10), (timer) {
      if (measurements.isNotEmpty) {
        measurements.clear();
        allMeasurements.clear();
        //uploadDataToGoogleSheets();
      }
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

      if (dataLines.length > 2) {
        // to remove lines without information (with less than 10 characters)
        List<String> tempDataLines = dataLines;
        dataLines = [];
        for (String line in tempDataLines) {
          if (line.length > 10) {
            dataLines.add(line);
          }
        }
      }
      if (dataLines.length > 1) {
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
          .map((e) =>
              int.tryParse(e.split(";").first) ??
              0) // Convert the data to integers
          .toList();

      List<int> decodedStatuses = currentData
          .replaceAll(RegExp(r'\s+'),
              ' ') // Replace multiple spaces with a single space
          .split(' ')
          .map((e) =>
              int.tryParse(e.split(";").last) ??
              0) // Convert the data to integers
          .toList();
      if (decodedData.length >= 64) {
        // Check if the data has at least 64 elements

        decodedData = decodedData.take(64).toList();
        Measurement currentMeasurement =
            Measurement(decodedData, timeOfMeasurement, decodedStatuses);
        measurements.add(currentMeasurement);
        allMeasurements.add(currentMeasurement);
        Measurement toReturn = Measurement(
            peopleCounter.processMeasurement(currentMeasurement),
            timeOfMeasurement,
            decodedStatuses);

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
        for (var data in measurement.depthData) {
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
      List<String> parsedMeasurement = []; // one row of data
      parsedMeasurement.add(measurement.time.toIso8601String());
      parsedMeasurement
          .addAll(measurement.depthData.map((e) => e.toString()).toList());
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

// created using prompt to GPT-4o: decode data from csv where one row is like 2025-04-02 07:49:36,572;259;206;758;897;986;1145;1294;0;191;195;214;286;2005;2082;2142;2172;193;207;236;2120;2162;2145;2173;2168;209;238;2108;286;2147;2178;2229;2191;560;2092;2108;2147;2173;2186;2227;2248;366;2130;2144;2185;2187;2228;2226;2272;2170;2168;2162;2183;2250;2244;2288;2211;2252;2199;2239;2236;2242;2120;1884;1700;6;6;6;6;6;6;6;255;6;6;6;255;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;255;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6;6

/// Reads the measurements from a file
List<Measurement> readMeasurementsFromFile(File fileRawData) {
    List<Measurement> measurements = [];
    List<String> lines = fileRawData.readAsLinesSync();
    for (String line in lines) {
      try {
        List<String> parts = line.split(';');
        if (parts.length > 1) {
          DateTime time = DateFormat("yyyy-MM-dd HH:mm:ss,SSS").parse(parts[0], true).toLocal();    //converts UTC time in given format to Local time  
          List<int> depthData = parts.sublist(1, 65).map(int.parse).toList();
          List<int> statuses = parts.sublist(65).map(int.parse).toList();
          measurements.add(Measurement(depthData, time, statuses));
        }
      } catch (e) {
        log('Failed to parse line: $line, error: $e');
      }
    }
    return measurements;
  }


/// Represents a measurement
class Measurement {
  final List<int> depthData; // depth in mm
  final DateTime time;
  final List<int> statuses;
  Measurement(this.depthData, this.time, this.statuses);
}
