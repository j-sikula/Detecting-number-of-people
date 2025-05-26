import 'dart:developer';
import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:sensor_gui/control/data_decoder.dart';
import 'package:sensor_gui/control/people_count_handler.dart';
import 'package:sensor_gui/control/people_counter.dart';
import 'package:sensor_gui/control/people_detector.dart';
import 'package:sensor_gui/measured_data_visualiser/grid_algorithm_data_widget.dart';
import 'package:sensor_gui/measured_data_visualiser/grid_data_widget.dart';
import 'package:sensor_gui/measured_data_visualiser/target_status_table.dart';

Measurement defaultMeasurement = Measurement(List<int>.filled(64, 0),
    DateTime.now(), List<int>.filled(64, 0)); // Default measurement

class MeasuredDataVisualiser extends StatefulWidget {
  const MeasuredDataVisualiser({super.key});

  @override
  MeasuredDataVisualiserState createState() => MeasuredDataVisualiserState();
}

class MeasuredDataVisualiserState extends State<MeasuredDataVisualiser> {
  String lblFileName = "No file selected";
  List<Measurement> measurement = List.filled(1, defaultMeasurement);
  int indexOfMeasurement = 0; // Index of the measurement to be displayed
  bool showTargetStatuses = false; // Show target statuses
  bool isFileLoading = false;
  int algorithm =
      0; // Algorithm index 0 - xcorrelation algorithm, 1 - zones of matrix
  PeopleDetector peopleDetector =
      PeopleDetector(); // Local minimums of correlation matrix algorithm
  PeopleCounter peopleCounter = PeopleCounter(null); // Zone of Matrix algorithm
  AlgorithmData dataAlgorithmGrid = AlgorithmData(
    dataGrid: List<int>.filled(64, 0),
    textToDisplay: "No data",
  );
  AlgorithmData heightDataAlgorithmGrid = AlgorithmData(
    dataGrid: List<int>.filled(64, 0),
    textToDisplay: "No data",
  );

  @override
  void initState() {
    super.initState();
    peopleCounter.setBackgroundFromList(List<int>.filled(64, 2100));
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: <Widget>[
        const SizedBox(height: 20),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            DropdownMenu<int>(
              initialSelection: algorithm,
              label: const Text("Algorithm"),
              dropdownMenuEntries: const [
                DropdownMenuEntry(value: 0, label: "X-Correlation"),
                DropdownMenuEntry(value: 1, label: "Zones of Matrix"),
              ],
              onSelected: (int? newValue) {
                setState(() {
                  algorithm = newValue ?? 0;
                });
              },
            ),
          ],
        ),
        const SizedBox(height: 10),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(lblFileName),
            ElevatedButton(
              onPressed: onBtnOpenFilePressed,
              child: isFileLoading
                  ? const SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(
                        strokeWidth: 3,
                      ),
                    )
                  : const Text("Open File"),
            ),
          ],
        ),
        const SizedBox(height: 10),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Text("Show depth"),
            const SizedBox(width: 10),
            Switch(
              value: showTargetStatuses,
              onChanged: (value) => {
                setState(() {
                  showTargetStatuses = value;
                })
              },
            ),
            const SizedBox(width: 10),
            const Text("Show target statuses"),
          ],
        ),
        Column(
          children: [
            GridDataWidget(
              measurement: measurement[indexOfMeasurement],
              showTargetStatuses: showTargetStatuses,
            ),
            showTargetStatuses ? const TargetStatusTable() : const SizedBox(),
          ],
        ),
        SelectableText(measurement[indexOfMeasurement].time.toString()),
        Slider(
          value: indexOfMeasurement.toDouble(),
          onChanged: onSliderChanged,
          min: 0,
          max: measurement.length.toDouble() - 1,
          divisions: measurement.length > 1 ? measurement.length - 1 : null,
          //label: "${indexOfMeasurement + 1} / ${measurement.length}",
        ),
        SingleChildScrollView(
          scrollDirection: Axis.horizontal,
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ButtonChangeTime(
                  onPressed: () {
                    changeIndexOfMeasurement(-600);
                  },
                  text: "- 1 min"),
              ButtonChangeTime(
                  onPressed: () {
                    changeIndexOfMeasurement(-10);
                  },
                  text: "- 1 s"),
              ButtonChangeTime(
                  onPressed: () {
                    changeIndexOfMeasurement(-1);
                  },
                  text: "-"),
              ButtonChangeTime(
                  onPressed: () {
                    changeIndexOfMeasurement(1);
                  },
                  text: "+"),
              ButtonChangeTime(
                  onPressed: () {
                    changeIndexOfMeasurement(10);
                  },
                  text: "+ 1 s"),
              ButtonChangeTime(
                  onPressed: () {
                    changeIndexOfMeasurement(600);
                  },
                  text: "+ 1 min"),
            ],
          ),
        ),
        GridAlgorithmDataWidget(
            data: algorithm == 0 ? dataAlgorithmGrid : heightDataAlgorithmGrid),
      ],
    );
  }

  /// Callback function to open file with raw data
  void onBtnOpenFilePressed() async {
    setState(() {
      isFileLoading = true;
    });
    FilePickerResult? result = await FilePicker.platform.pickFiles();

    if (result != null) {
      String path = result.files.single.path ?? "No path available";
      File rawDataFile = File(path);

      log("Loading file started");
      List<Measurement> measurements =
          await compute(readMeasurementsFromFile, rawDataFile);

      if (measurements.isEmpty) {
        log("No measurements found in the file");
        return;
      }
      setState(() {
        lblFileName = path;
        indexOfMeasurement = 0;
        measurement = measurements;
      });
      peopleDetector.resetPeopleCounter();
      for (var mes in measurement) {
        if (algorithm == 0) {
          peopleDetector.processFrame(mes); // Process all measurements
        } else if (algorithm == 1) {
          peopleCounter.processMeasurement(mes);
        }
      }

      String? pathProcessedData = await FilePicker.platform.saveFile();
      if (pathProcessedData != null) {
        if (algorithm == 0) {
          savePeopleCountHistory(
              pathProcessedData, peopleDetector.peopleCountHistory);
        } else if (algorithm == 1) {
          savePeopleCountHistory(
              pathProcessedData, peopleCounter.peopleCountHistory);
        }
      }
    }

    setState(() {
      isFileLoading = false;
    });
  }

  void onSliderChanged(double value) {
    setState(() {
      indexOfMeasurement = value.toInt();
    });
  }

  void changeIndexOfMeasurement(int incrementVal) {
    setState(() {
      indexOfMeasurement += incrementVal;
      if (indexOfMeasurement < 0) {
        indexOfMeasurement = 0;
      } else if (indexOfMeasurement >= measurement.length) {
        indexOfMeasurement = measurement.length - 1;
      }
      if (algorithm == 0) {
        dataAlgorithmGrid = peopleDetector.processFrame(
            measurement[indexOfMeasurement]); // Process the current measurement
      } else if (algorithm == 1) {
        heightDataAlgorithmGrid = AlgorithmData(
            dataGrid: peopleCounter
                .processMeasurement(measurement[indexOfMeasurement]),
            textToDisplay: "Height data");
      }
    });
  }
}

class ButtonChangeTime extends StatelessWidget {
  const ButtonChangeTime({
    super.key,
    required this.onPressed,
    required this.text,
  });

  final VoidCallback onPressed;
  final String text;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(5),
      child: ElevatedButton(
        onPressed: onPressed,
        child: Text(text),
      ),
    );
  }
}
