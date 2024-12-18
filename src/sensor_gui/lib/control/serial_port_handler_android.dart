import 'dart:async';
import 'dart:developer';
import 'dart:typed_data';

import 'package:sensor_gui/control/serial_port_handler.dart';
import 'package:usb_serial/usb_serial.dart';

/// Gets the available ports
/// static asynchronous method
Future<List<String>> getAvailablePortsAndroid() async {
  List<String> ports = [];

  List<UsbDevice> devices = await UsbSerial.listDevices();
  log("Get available ports");
  log(devices.toString());
  UsbSerial.listDevices().then((devices) {
    for (var device in devices) {
      ports.add(device.deviceName);
    }
  });
  return ports;
}

/// Serial port handler for Android,
/// Extends the SerialPortHandler class,
/// used for handling serial port operations on Android,
/// parameters: [baudRate], [portName]
class SerialPortHandlerAndroid extends SerialPortHandler {
  /// Constructor, [isCreatingSerialPort] used for creating SerialPort for windows
  SerialPortHandlerAndroid(super.baudRate, super.portName);

  /// Serial port object
  UsbPort? usbPort;
  

  /// Closes the serial port

  @override
  Future<bool> closePort() async {    
    bool closeResult = await usbPort!.close();
    if (closeResult) {
      isPortOpen = false;
      log('Port closed successfully');
      return true;
    } else {
      log('Failed to close port');
      return false;
    }
  }

  /// Opens the serial port,
  /// returns true if port is opened successfully.
  /// Baud rate according to constructor, type 8N1
  /// streams filled with received data
  @override
  Future<bool> openPort() async {
    List<UsbDevice> devices = await UsbSerial.listDevices();

    for (var device in devices) {
      if (device.deviceName == portName) {
        usbPort = await device.create();
        break;
      }
    }

    if (usbPort == null) {
      log('Device not found');
      return false;
    }

    bool openResult = await usbPort!.open();
    if (!openResult) {
      log('Failed to open port');
      return false;
    }

    await usbPort!.setDTR(true);
    await usbPort!.setRTS(true);
    await usbPort!.setPortParameters(
        baudRate, UsbPort.DATABITS_8, UsbPort.STOPBITS_1, UsbPort.PARITY_NONE);

    isPortOpen = true;
    decodedDataStream = usbPort!.inputStream!.map((Uint8List data) {
      return decoder.decode(data)?.data ??
          []; // Decode the received data, return the data if it is not null
    });

    receivedData = usbPort!.inputStream!.map((data) {
      return String.fromCharCodes(data);
    });

    log('Port opened successfully');
    isPortOpen = true;
    return true;
  }
}
