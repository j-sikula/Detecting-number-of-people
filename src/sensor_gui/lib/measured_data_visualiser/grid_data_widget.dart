import 'package:flutter/material.dart';
import 'package:sensor_gui/control/data_decoder.dart';

class GridDataWidget extends StatelessWidget {
  final Measurement measurement;
  final bool showTargetStatuses; // Show target statuses
  const GridDataWidget({
    super.key,
    required this.measurement,
    required this.showTargetStatuses,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
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
            color: measurement.statuses[index] == 255? const Color.fromRGBO(5, 5, 5, 1.0): Color.lerp(
                const Color.fromARGB(255, 14, 5, 141),
                const Color.fromARGB(255, 255, 17, 0),
                measurement.depthData[index] / 4000),
            child: Center(
              child: showTargetStatuses? Text(
                '${measurement.statuses[index]}',
                style: const TextStyle(color: Colors.white),
              ): Text(
                '${measurement.depthData[index]}',
                style: const TextStyle(color: Colors.white),
              ),
            ),
          );
        },
      ),
    );
  }
}
