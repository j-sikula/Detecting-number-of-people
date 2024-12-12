import 'dart:async';
import 'dart:developer';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'package:sensor_gui/control/data_decoder.dart';

class SerialPortHandler {
  final int baudRate;
  final String portName;

  bool isPortOpen = false; // Flag to check if the port is open
  bool tryOpenPortOnceAgain =
      true; // after closing port, try to open it only once again
  SerialPort? serialPort;
  Stream<String>? receivedData;
  Stream<List<int>>? decodedDataStream;
  List<Measurement> measuredData = [];
  // DataDecoder used for saving file and decoding received data
  DataDecoder decoder = DataDecoder();

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

      decodedDataStream = reader.stream.map((data) {
        return decoder.decode(data)?.data ??
            []; // Decode the received data, return the data if it is not null
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
