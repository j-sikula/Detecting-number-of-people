import 'dart:async';
import 'dart:developer';

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
        if (previousData.split('Data').length > 3) {
          // Check if there is one complete set of data
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
}
