import 'dart:async';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'package:sensor_gui/control/data_decoder.dart';

class SerialPortHandler {
  final int baudRate;
  final String portName;

  bool isPortOpen = false; // Flag to check if the port is open
  bool tryOpenPortOnceAgain =
      true; // unsucessful opening, after closing port, try to open it only once again
  SerialPort? serialPort;
  /// Stream with decoded measurements from serial port
  Stream<Measurement?>? decodedDataStream;
  List<Measurement> measuredData = [];
  // DataDecoder used for saving file and decoding received data
  DataDecoder decoder = DataDecoder();

  // Constructor
  SerialPortHandler(this.baudRate, this.portName);

  /// Closes the serial port
  Future<bool> closePort() async {
    return true;
  }

  /// Opens the serial port,
  /// configuration 8N1
  ///  baud rate according to the baudRate parameter in the constructor

  Future<bool> openPort() async {
    return true;
  }
}
