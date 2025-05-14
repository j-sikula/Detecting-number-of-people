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

  @override
  initState() {
    super.initState();
    peopleCountHandler = PeopleCountHandler();
    if (peopleCountHandler == null) {
      log("Google Sheets API is not initialized");
      return;
    }
  }

  List<FlSpot> dataPoints = [];

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Container(
              margin: const EdgeInsets.all(10),
              child: DropdownMenu<int>(
                  initialSelection: _selectedIndex,
                  label: const Text('Data source'),
                  width: 180,
                  onSelected: onSelectedIndex,
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
                      label: "Custom range",
                      enabled: true,
                    ),
                  ]),
            ),
            ElevatedButton(onPressed: onPressed, child: const Icon(Icons.refresh)),
          ],
        ),
        PeopleCountGraph(dataPoints: dataPoints),
      ],
    );
  }

  void onPressed() async {
    onSelectedIndex(_selectedIndex);
  }

  void onSelectedIndex(int? newValue) async {
    setState(() {
      _selectedIndex = newValue;
    });
    List<FlSpot> data = [];
    if (newValue == 0) {
      data = fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
          DateTime.now().subtract(const Duration(hours: 1)), DateTime.now()));
    } else if (newValue == 1) {
      data = fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
          DateTime.now().subtract(const Duration(days: 1)), DateTime.now()));
    } else if (newValue == 2) {
      data = fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
          DateTime.now().subtract(const Duration(days: 7)), DateTime.now()));
    } else if (newValue == 3) {
      // Custom range
      final range = await showDateRangePicker(
        context: context,
        firstDate: DateTime.now().subtract(const Duration(days: 30)),
        lastDate: DateTime.now(),
      );
      if (range != null) {
        data = fromListPeopleCount(await peopleCountHandler!.getPeopleCountData(
            range.start, range.end));
      }
    }
    setState(() {
      dataPoints = data;
    });
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
