import 'dart:async';

import 'package:flutter/material.dart';

import 'control/serial_port_handler.dart';

class SensorDataVisualiser extends StatefulWidget {
  final SerialPortHandler? serialPortHandler;
  const SensorDataVisualiser({super.key, required this.serialPortHandler});

  @override
  SensorDataVisualiserState createState() => SensorDataVisualiserState();
}

class SensorDataVisualiserState extends State<SensorDataVisualiser> {
  List<int> sensorData = List.filled(64, 0);
  bool startListening = false;
  StreamSubscription<List<int>>? decodedDataSubscription;

  /// Enables listening to the received data
  void enableListening() {
    startListening = true;
  }

  /// Stops listening to the received data
  void stopListening() {
    decodedDataSubscription?.cancel();
  }

  @override
  Widget build(BuildContext context) {
    if (widget.serialPortHandler != null && startListening) {
      decodedDataSubscription =
          widget.serialPortHandler!.decodedDataStream!.listen((data) {
        if (data.length == 64) {
          setState(() {
            sensorData = data;
          });
        }
      });
      startListening = false;
    }
    return GridView.builder(
      scrollDirection: Axis.vertical,
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 8,
      ),
      itemCount: 64,
      itemBuilder: (context, index) {
        return Container(
          margin: const EdgeInsets.all(4.0),
          height: 5,
          width: 5,
          color: Color.lerp(const Color.fromARGB(255, 14, 5, 141), const Color.fromARGB(255, 255, 17, 0), sensorData[index] / 4000),
          child: Center(
            child: Text(
              '${sensorData[index]}',
              style: const TextStyle(color: Colors.white),
            ),
          ),
        );
      },
    );
  }
}
