import 'dart:developer';

import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:sensor_gui/control/people_count_handler.dart';
import 'package:sensor_gui/control/people_counter.dart';
import 'package:sensor_gui/resullts_visualiser/people_count_graph.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => HomeScreenState();
}

class HomeScreenState extends State<HomeScreen> {
  PeopleCountHandler? peopleCountHandler;
  int? _selectedIndex = 0;
  Future<List<FlSpot>>? _dataFuture;

  @override
  initState() {
    super.initState();
    peopleCountHandler = PeopleCountHandler();
    if (peopleCountHandler == null) {
      log("Google Sheets API is not initialized");
      return;
    }
    _dataFuture = _fetchData(_selectedIndex);
  }

  List<FlSpot> dataPoints = [];

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
        builder: (BuildContext context, BoxConstraints constraints) {
      double? width = constraints.maxWidth;
      if (width > constraints.maxHeight) {
        width = null;
      }
      return SingleChildScrollView(
        child: Column(
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  margin: const EdgeInsets.all(10),
                  child: DropdownMenu<int>(
                      initialSelection: _selectedIndex,
                      label: const Text('Data source'),
                      width: 180,
                      onSelected: (int? newValue) async {
                        setState(() {
                          _selectedIndex = newValue;
                          _dataFuture = _fetchData(newValue);
                        });
                      },
                      dropdownMenuEntries: const [
                        DropdownMenuEntry<int>(
                          value: 0,
                          label: "Last hour",
                          enabled: true,
                        ),
                        DropdownMenuEntry<int>(
                          value: 1,
                          label: "Last day",
                          enabled: true,
                        ),
                        DropdownMenuEntry<int>(
                          value: 2,
                          label: "Last week",
                          enabled: true,
                        ),
                        DropdownMenuEntry<int>(
                          value: 3,
                          label: "Custom date",
                          enabled: true,
                        ),
                      ]),
                ),
                ElevatedButton(
                    onPressed: onPressed, child: const Icon(Icons.refresh)),
              ],
            ),
            // GitHub Copilot on request: change it to future builder to suite that future function, when loading, show progress indicator
            FutureBuilder<List<FlSpot>>(
              future: _dataFuture,
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.waiting) {
                  return const Padding(
                    padding: EdgeInsets.all(32.0),
                    child: CircularProgressIndicator(),
                  );
                } else if (snapshot.hasError) {
                  log("Error fetching data: ${snapshot.error}");
                  return const Padding(
                    padding: EdgeInsets.all(32.0),
                    child: Text('Something went wrong, try again later.',
                        style: TextStyle(fontSize: 18, color: Colors.red)),
                  );
                } else {
                  final dataPoints = snapshot.data ?? [];
                  return PeopleCountGraph(
                      dataPoints: dataPoints, width: width ?? 600);
                }
              },
            ),
          ],
        ),
      );
    });
  }

  void onPressed() {
    setState(() {
      _dataFuture = _fetchData(_selectedIndex);
    });
  }

  Future<List<FlSpot>> _fetchData(int? index) async {
    if (peopleCountHandler == null) return [];
    if (index == 0) {
      return fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
          DateTime.now().subtract(const Duration(hours: 1)), DateTime.now()));
    } else if (index == 1) {
      return fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
          DateTime.now().subtract(const Duration(days: 1)), DateTime.now()));
    } else if (index == 2) {
      return fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
          DateTime.now().subtract(const Duration(days: 7)), DateTime.now()));
    } else if (index == 3) {
      // Custom range
      final range = await showDateRangePicker(
        context: context,
        confirmText: 'Select',
        saveText: 'Select',
        firstDate: DateTime.now().subtract(const Duration(days: 30)),
        lastDate: DateTime.now(),
      );
      if (range != null) {
        return fromListPeopleCount(await peopleCountHandler!
            .getPeopleCountData(range.start, range.end));
      }
    }
    return [];
  }
}

List<FlSpot> fromListPeopleCount(List<PeopleCount> data) {
  List<FlSpot> dataPoints = [];
  if (data.isEmpty) {
    return dataPoints;
  }
  dataPoints.add(
      FlSpot(data[0].getTimestampInMiliseconds(), data[0].count.toDouble()));
  for (int i = 1; i < data.length; i++) {
    dataPoints.add(FlSpot(
        data[i].getTimestampInMiliseconds(), data[i - 1].count.toDouble()));
    dataPoints.add(
        FlSpot(data[i].getTimestampInMiliseconds(), data[i].count.toDouble()));
  }
  return dataPoints;
}
