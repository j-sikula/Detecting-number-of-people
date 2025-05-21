import 'package:flutter/material.dart';
import 'package:sensor_gui/sensor_data_visualiser/serial_port_selector.dart';

class SensorDataVisualiserScreen extends StatelessWidget {
  const SensorDataVisualiserScreen({super.key});

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
