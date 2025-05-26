import 'package:flutter/material.dart';
import 'package:sensor_gui/usb_serial_data_visualiser/serial_port_selector.dart';

class USBSerialDataVisualiserScreen extends StatelessWidget {
  const USBSerialDataVisualiserScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const SingleChildScrollView(
        scrollDirection: Axis.vertical,
        child: SizedBox(
          width: 400,
          child: SerialPortSelector(),
        ));
  }
}
