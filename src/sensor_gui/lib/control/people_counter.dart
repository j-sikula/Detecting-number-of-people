
import 'dart:developer';

import 'package:sensor_gui/control/data_decoder.dart';


class PeopleCounter {
  int lastPosition = 0;
  int peopleCount = 0;
  final int heightTreshold = 1000;
  final int nZones = 4;
  List<int> background = List.filled(64, 0);

  processMeasurement(Measurement measurement) {
    // Checks size of the data
    if (background.length != 64 && measurement.data.length != 64) {
      return;
    }
    List<int> heightData = List.generate(background.length, (index) => background[index] - measurement.data[index]);
    for (int i = 0; i < nZones; i++) {
      int maxHeight = max(heightData.sublist((i*64/nZones) as int, ((i+1)*64/nZones) as int?));
      if (maxHeight > heightTreshold) {

        if (lastPosition == 1 && i == 0) {
          peopleCount--;
          log('People count decremented: $peopleCount people');
        }
        if (lastPosition == nZones-1 && i == nZones) {
          peopleCount++;
          log('People count incremented: $peopleCount people');
        }
        lastPosition = i;
      } 
    }
    
  }
  
  int max(List<int> sublist) {
    int maxValue = 0;
    for(int x in sublist) {
      if (x > maxValue) {
        maxValue = x;
      }
    }
    return maxValue; 
  }
}