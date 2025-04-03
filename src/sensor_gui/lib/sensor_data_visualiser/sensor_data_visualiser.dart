import 'dart:async';

import 'package:flutter/material.dart';
import 'package:sensor_gui/control/data_decoder.dart';

import '../control/serial_port_handler.dart';

class SensorDataVisualiser extends StatefulWidget {
  final SerialPortHandler? serialPortHandler;
  const SensorDataVisualiser({super.key, required this.serialPortHandler});

  @override
  SensorDataVisualiserState createState() => SensorDataVisualiserState();
}

class SensorDataVisualiserState extends State<SensorDataVisualiser> {
  List<int> sensorData = List.filled(64, 0);
  List<int> sensorStatuses = List.filled(64, 5);
  bool startListening = false;
  StreamSubscription<Measurement?>? decodedDataSubscription;

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
    if (widget.serialPortHandler != null &&
        widget.serialPortHandler!.decodedDataStream != null &&
        startListening) {
      decodedDataSubscription =
          widget.serialPortHandler!.decodedDataStream!.listen((data) {
        if (data != null) {
          if (data.data.length == 64) {
            setState(() {
              sensorData = data.data;
              sensorStatuses = data.statuses;
            });
          }
        }
      });
      startListening = false;
    }
    return Column(
      children: [
        Container(
          margin: const EdgeInsets.all(10),
          child: Text(
            "Depth [mm]",
            style: Theme.of(context).textTheme.bodyLarge,
          ),
        ),
        SizedBox(
          width: 400,
          height: 400,
          child: GridView.builder(
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
                color: sensorStatuses[index] == 5
                    ? Color.lerp(
                        const Color.fromARGB(255, 14, 5, 141),
                        const Color.fromARGB(255, 255, 17, 0),
                        sensorData[index] / 4000)!
                    : Colors.black,
                child: Center(
                  child: Text(
                    '${sensorData[index]}',
                    style: const TextStyle(color: Colors.white),
                  ),
                ),
              );
            },
          ),
        ),
        Container(
          margin: const EdgeInsets.all(10),
          child: Text(
            "Target statuses",
            style: Theme.of(context).textTheme.bodyLarge,
          ),
        ),
        SizedBox(
          width: 400,
          height: 400,
          child: GridView.builder(
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
                color: sensorStatuses[index] == 5
                    ? Color.lerp(
                        const Color.fromARGB(255, 14, 5, 141),
                        const Color.fromARGB(255, 255, 17, 0),
                        sensorData[index] / 4000)!
                    : Colors.black,
                child: Center(
                  child: Text(
                    '${sensorStatuses[index]}',
                    style: const TextStyle(color: Colors.white),
                  ),
                ),
              );
            },
          ),
        ),
      ],
    );
  }
}
