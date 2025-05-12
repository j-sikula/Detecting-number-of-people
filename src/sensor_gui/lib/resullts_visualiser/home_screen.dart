import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:sensor_gui/resullts_visualiser/people_count_graph.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  final List<FlSpot> dataPoints = const [
    FlSpot(0, 5),
    FlSpot(1, 10),
    FlSpot(2, 15),
    FlSpot(3, 20),
    FlSpot(4, 25),
    FlSpot(5, 30),
    FlSpot(6, 35),
    FlSpot(7, 40),
    FlSpot(8, 45),
    FlSpot(9, 50),
  ];

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        PeopleCountGraph(dataPoints: dataPoints)
        
      ],
    );
  }
}