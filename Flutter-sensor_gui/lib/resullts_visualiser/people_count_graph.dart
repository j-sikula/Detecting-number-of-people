import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:intl/intl.dart'; // Add this import for date formatting

class PeopleCountGraph extends StatelessWidget {
  final List<FlSpot> dataPoints;
  late final double xInterval;
  late final double yInterval;
  final double width;
  final double height;

  PeopleCountGraph(
      {super.key,
      required this.dataPoints,
      this.width = 600,
      this.height = 300}) {
    if (dataPoints.isNotEmpty) {
      xInterval = dataPoints.last.x - dataPoints.first.x;
      if (dataPoints.intervalY > 5) {
        yInterval = (dataPoints.intervalY / 5).round().toDouble(); // Divide by 2 for better readability
      } else {
        yInterval = 1; // Default to 1 if intervalY is zero or negative
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    if (dataPoints.isEmpty) {
      return const Center(
        child: Text(
          'No data available',
          style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
        ),
      );
    }
    return Card(
      elevation: 4,
      margin: const EdgeInsets.all(16),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            const Text(
              'People Count',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: height, // Ensure height is explicitly defined
              width: width, // Add width constraint
              child: LineChart(
                LineChartData(
                  gridData: const FlGridData(show: true),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                          showTitles: true,
                          interval: yInterval,
                          reservedSize: 40,
                          minIncluded: true,
                          maxIncluded: true),
                    ),
                    rightTitles: const AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(
                        interval: getInterval(),
                        showTitles: true,
                        reservedSize: 40,
                        minIncluded: false,
                        maxIncluded: false,
                        getTitlesWidget: (value, meta) {
                          // Convert milliseconds to hh:mm:ss format
                          final date = DateTime.fromMillisecondsSinceEpoch(
                              value.toInt());
                          String formattedTime;
                          if (xInterval > 86400000) {
                            formattedTime =
                                DateFormat('EEE d.M.\nHH:mm:ss').format(date);
                          } else {
                            formattedTime = DateFormat('HH:mm:ss').format(date);
                          }

                          return Text(
                            formattedTime,
                            style: const TextStyle(fontSize: 15),
                            textAlign: TextAlign.center,
                          );
                        },
                      ),
                      axisNameWidget: const Text('Time'),
                    ),
                    topTitles: const AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                  ),
                  borderData: FlBorderData(show: true),
                  lineBarsData: [
                    LineChartBarData(
                      spots: dataPoints,
                      isCurved: false,
                      barWidth: 4,
                      isStrokeCapRound: true,
                      belowBarData: BarAreaData(show: false),
                    ),
                  ],
                  lineTouchData: LineTouchData(
                    touchTooltipData: LineTouchTooltipData(
                      getTooltipItems: (touchedSpots) {
                        return touchedSpots.map((spot) {
                          final date = DateTime.fromMillisecondsSinceEpoch(
                              spot.x.toInt());
                          final formattedDate =
                              DateFormat('EEE, dd/MM/y \nHH:mm:ss.SSS')
                                  .format(date);
                          final present = spot.y > 1 ? "people" : "person";
                          return LineTooltipItem(
                            '$formattedDate\n${spot.y.toStringAsFixed(0)} $present',
                            const TextStyle(color: Colors.white),
                          );
                        }).toList();
                      },
                    ),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  /// Returns the interval in ms for the x-axis based on the total xInterval.
  double getInterval() {
    const List<double> intervals = [
      1000, // 1 second
      10000, // 10 seconds
      15000, // 15 seconds
      20000, // 20 seconds
      30000, // 30 seconds
      60000, // 1 minute
      120000, // 2 minutes
      300000, // 5 minutes
      600000, // 10 minutes
      900000, // 15 minutes
      1200000, // 20 minutes
      1800000, // 30 minutes
      3600000, // 1 hour
      7200000, // 2 hours
      21600000, // 6 hours
      43200000, // 12 hours
      86400000, // 1 day
    ];
    int maxIntervals = (width * 0.01807 - 2.843)
        .toInt(); // Calculate maximum intervals (2-8) based on width (268-600)
    // Find the largest interval that results in 2 to 8 subintervals
    for (double interval in intervals) {
      if (xInterval / interval >= 2 && xInterval / interval <= maxIntervals) {
        return interval;
      }
    }

    // Default to splitting the interval into 5 equal parts if no suitable interval is found
    return xInterval / maxIntervals;
  }
}

extension Maximum on List<FlSpot> {
  double get maxY {
    if (isEmpty) return 0.0;
    double maxYval = this[0].y;
    for (var spot in this) {
      if (spot.y.isNaN || spot.y.isInfinite) {
        throw ArgumentError('Invalid y value in FlSpot: $spot');
      } else {
        maxYval = spot.y > maxYval ? spot.y : maxYval;
      }
    }
    return maxYval;
  }

  double get minY {
    if (isEmpty) return 0.0;
    double minYval = this[0].y;
    for (var spot in this) {
      if (spot.y.isNaN || spot.y.isInfinite) {
        throw ArgumentError('Invalid y value in FlSpot: $spot');
      } else {
        minYval = spot.y < minYval ? spot.y : minYval;
      }
    }
    return minYval;
  }

  double get intervalY {
    if (isEmpty) return 0.0;
    return (maxY - minY);
  }
}
