import 'package:flutter/material.dart';
import 'package:sensor_gui/control/people_detector.dart';

class GridAlgorithmDataWidget extends StatelessWidget {
  final AlgorithmData data;

  const GridAlgorithmDataWidget({super.key, required this.data});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(
          data.textToDisplay,
          
        ),
        SizedBox(
          width: 400,
          height: 400,
          child: GridView.builder(
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 8, // 8 columns for a grid of 64 items
              crossAxisSpacing: 4.0,
              mainAxisSpacing: 4.0,
            ),
            itemCount: data.dataGrid.length,
            itemBuilder: (context, index) {
              return Container(
                alignment: Alignment.center,
                decoration: BoxDecoration(
                  color: data.highlightedData[index] ?? Color.lerp(
                      const Color.fromARGB(255, 14, 5, 141),
                      const Color.fromARGB(255, 255, 17, 0),
                      data.dataGrid[index] / 4000),
                  border: Border.all(color: Colors.black),
                ),
                child: Text(
                  data.dataGrid[index].toString(),
                  style: const TextStyle(color: Colors.white),
                ),
              );
            },
          ),
        ),
      ],
    );
  }
}
