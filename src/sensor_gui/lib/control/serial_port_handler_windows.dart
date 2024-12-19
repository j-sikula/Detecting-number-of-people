import 'dart:developer';

import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'package:sensor_gui/control/serial_port_handler.dart';

class SerialPortHandlerWindows extends SerialPortHandler {
  SerialPortReader? reader;

  SerialPortHandlerWindows(super.baudRate, super.portName){
    serialPort = SerialPort(portName);
  }

  @override
  Future<bool> openPort() async {
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
      reader = SerialPortReader(serialPort!);

      receivedData = reader!.stream.map((data) {
        return String.fromCharCodes(data);
      });

      decodedDataStream = reader!.stream.map((data) {
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

  @override
  Future<bool> closePort() async {
    if (serialPort!.close()) {
      isPortOpen = false;
      log('Port closed successfully');
      return true;
    } else {
      log('Failed to close port');
      return false;
    }
  }
}
