import 'dart:async';
import 'dart:developer';
import 'dart:io';
import 'package:flutter_libserialport/flutter_libserialport.dart';

class SerialPortHandler {
  final int baudRate;
  final String portName;

  bool isPortOpen = false; // Flag to check if the port is open
  bool tryOpenPortOnceAgain =
      true; // after closing port, try to open it only once again
  SerialPort? serialPort;
  Stream<String>? receivedData;
  Stream<List<int>>? decodedDataStream;
  List<List<int>> measuredData = [];

  // Constructor
  SerialPortHandler(this.baudRate, this.portName) {
    // Initialize the serial port
    serialPort = SerialPort(portName);
  }

  /// Closes the serial port
  bool closePort() {
    if (serialPort!.close()) {
      isPortOpen = false;
      log('Port closed successfully');
      return true;
    } else {
      log('Failed to close port');
      return false;
    }
  }

  /// Opens the serial port,
  /// configuration 8N1
  ///  baud rate according to the baudRate parameter in the constructor

  bool openPort() {
    final portConfig = SerialPortConfig();
    portConfig.baudRate = baudRate;
    portConfig.parity = SerialPortParity.none;
    portConfig.bits = 8;
    portConfig.stopBits = 1;
    if (serialPort!.openReadWrite()) {
      serialPort!.config =
          portConfig; // Set the port configuration immedietly after opening the port
      isPortOpen = true;
      log('Port opened successfully');
      SerialPortReader reader = SerialPortReader(serialPort!);
      receivedData = reader.stream.map((data) {
        return String.fromCharCodes(data);
      });

      String previousData = '';
      decodedDataStream = reader.stream.map((data) {
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
          List<int> decodedData = currentData
              .replaceAll(RegExp(r'\s+'),
                  ' ') // Replace multiple spaces with a single space
              .split(' ')
              .skip(1)
              .map((e) => int.tryParse(e) ?? 0) // Convert the data to integers
              .toList();
          if (decodedData.length >= 64) {
            // Check if the data has at least 64 elements
            measuredData.add(decodedData.take(64).toList());
            return decodedData
                .take(64)
                .toList(); // Return the first 64 elements
          }
        }
        return [];
      });

      return true;
    } else {
      closePort();
      if (tryOpenPortOnceAgain) {
        tryOpenPortOnceAgain = false;
        return openPort();
      }
      log('Failed to open port');
      return false;
    }
  }

  /// Saves the measured data to a file at [path]
  /// The first row contains the header
  /// After saving the data, the measured data is cleared
 
  void saveDataToFile(String path) async {
    if(path.contains('.') == false){  // Checks if the path contains the file extension
      path = '$path.csv'; // Adds the file extension csv if not present
    }

    final file = File(path);
    try {
      final sink = file.openWrite();
      sink.write('Data measured on VL53L7CX;Time;${DateTime.now()}\n');
      for (var i = 0; i < 64; i++) {  // Write the header to the file
        sink.write('Zone $i;');
      }
      sink.write('\n');
      for (var measurement in measuredData) { // Write the data to the file
        for (var data in measurement) {
          sink.write('$data;');
        }
        sink.write('\n'); 
      }
      await sink.flush();
      await sink.close();
      measuredData.clear(); // Clear the measured data after saving it to the file
      log('Data saved to file successfully');
    } catch (e) {
      log('Failed to save data to file: $e');
    }
  }
}
