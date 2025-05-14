import 'dart:developer';

import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:sensor_gui/control/google_sheets_api.dart';
import 'package:sensor_gui/control/people_counter.dart';
import 'package:sensor_gui/resullts_visualiser/people_count_graph.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  GoogleSheetsApi? apiPeopleCounter;
  final String spreadsheetID = "1SMUomRFOupgDCK7eLoi8eb6Y_97LJ3NA8j68mztiyTw";
  
  _HomeScreenState() {
    apiPeopleCounter = GoogleSheetsApi(spreadsheetID, false);
    if (apiPeopleCounter == null) {
      log("Google Sheets API is not initialized");
      return;
    }
    apiPeopleCounter!.initGoogleAPI();
  }

  List<FlSpot> dataPoints = const [
    FlSpot(1747044983240, -5),
    FlSpot(1747044999990, 10),
  ];

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        ElevatedButton(onPressed: onPressed, child: const Text("Update")),
        PeopleCountGraph(dataPoints: dataPoints),
      ],
    );
  }

  void onPressed() async {
    List<FlSpot> data = fromListPeopleCount(await apiPeopleCounter!.getPeopleCount("2025-W20"));
    setState(() {
      dataPoints = data;
    });
  }
}

List<FlSpot> fromListPeopleCount(List<PeopleCount> data) {
  List<FlSpot> dataPoints = [];
  for (var element in data) {
    dataPoints.add(FlSpot(element.getTimestampInMiliseconds(), element.count.toDouble()));
  }
  return dataPoints;
}

