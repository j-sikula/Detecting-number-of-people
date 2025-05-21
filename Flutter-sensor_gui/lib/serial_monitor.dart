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
  final int maxLines = 30;  // Maximum number of lines to display

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
    return Scrollbar(
      controller: _scrollController,
      child: SingleChildScrollView(
        controller: _scrollController,
        clipBehavior: Clip.antiAlias,
        child: Center(
          child: widget.serialPortHandler != null
              ? StreamBuilder<String>(
                  stream: widget.serialPortHandler!.receivedData!,
                  builder: (context, snapshot) {
                    if (snapshot.connectionState == ConnectionState.waiting) {
                      return const CircularProgressIndicator(); // Display a loading indicator when waiting for data.
                    } else if (snapshot.hasError) {
                      return Text(
                          'Error: ${snapshot.error}'); // Display an error message if an error occurs.
                    } else if (snapshot.hasData) {
                      displayText += '${snapshot.data}'; // Append new data to displayText.
                      List<String> lines = displayText.split('\n');
                      if (lines.length >maxLines) {
                        lines = lines.sublist(lines.length - maxLines);
                      }
                      displayText = lines.join('\n');
                      WidgetsBinding.instance.addPostFrameCallback((_) {
                        _scrollController.animateTo(
                          _scrollController.position.maxScrollExtent,
                          duration: const Duration(milliseconds: 300),
                          curve: Curves.easeOut,
                        );
                      });
                      return Text(
                        displayText,
                        style: const TextStyle(fontSize: 14),
                      ); // Display the accumulated data.
                    } else {
                      return const Text(
                          'No data available'); // Display a message when no data is available.
                    }
                  },
                )
              : const Text("No serial port handler found!"),
        ),
      ),
    );
  }
}
