import 'package:flutter/material.dart';
import 'package:sensor_gui/measured_data_visualiser/measured_data_visualiser.dart';

class MeasuredDataVisualiserScreen extends StatelessWidget {
  const MeasuredDataVisualiserScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const SingleChildScrollView(
      child: MeasuredDataVisualiser(),
    );
  }
}
