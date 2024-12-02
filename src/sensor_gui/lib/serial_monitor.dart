import 'dart:async';

import 'package:flutter/material.dart';
import 'package:sensor_gui/control/serial_port_handler.dart';

class SerialMonitor extends StatefulWidget {
  final SerialPortHandler? serialPortHandler;

  const SerialMonitor({super.key, required this.serialPortHandler});

  @override
  SerialMonitorState createState() => SerialMonitorState();
}

class SerialMonitorState extends State<SerialMonitor> {
  String displayText = '';
  bool startListening = false;
  StreamSubscription<String>? receivedDataSubscription;
  final ScrollController _scrollController = ScrollController();

  void clearDisplayText() {
    setState(() {
      displayText = '';
    });
  }

  /// Enables listening to the received data
  void enableListening() {
    startListening = true;
  }

  /// Stops listening to the received data
  void stopListening() {
    receivedDataSubscription?.cancel();
  }

  @override
  void dispose() {
    _scrollController.dispose();
    receivedDataSubscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (widget.serialPortHandler != null && startListening) {
      receivedDataSubscription =
          widget.serialPortHandler!.receivedData!.listen((data) {
        setState(() {
          displayText = '$data\n';
        });
      });
      startListening = false;
    }
    return Scrollbar(
      controller: _scrollController,
      child: SingleChildScrollView(
        controller: _scrollController,
        child: Column(
          children: [
            const Text('Serial Monitor'),
            if (widget.serialPortHandler != null)
              Text('Baud Rate: ${widget.serialPortHandler!.baudRate}'),
            const Text('Data:'),
            Center(
              child: Text(displayText),
            ),
          ],
        ),
      ),
    );
  }
}
