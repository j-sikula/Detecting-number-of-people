import 'dart:developer';

import 'package:sensor_gui/control/data_decoder.dart';
import 'package:sensor_gui/control/google_sheets_api.dart';

class PeopleCounter {
  final GoogleSheetsApi apiPeopleCounter;
  int lastPosition = 0;
  int peopleCount = 0;
  final int heightTreshold = 1000;
  final int nZones = 4;
  List<int> background = List.filled(64, 0);

  PeopleCounter(this.apiPeopleCounter);

  List<int> processMeasurement(Measurement measurement) {
    // Checks size of the data
    if (background.length != 64 && measurement.data.length != 64) {
      return [];
    }
    List<int> heightData = List.generate(background.length,
        (index) => background[index] - measurement.data[index]);
    LocalMaximum localMaximum = maxAndPosition(heightData);
    int currentZone = localMaximum.getZone(nZones);
    if (localMaximum.value > heightTreshold) {
      if (lastPosition == 1 && currentZone == 0) {
        peopleCount--;
        log('People count decremented: $peopleCount people');
        apiPeopleCounter.appendDataRow([DateTime.now().toIso8601String(), peopleCount.toString()]);
      }
      if (lastPosition == nZones - 1 && currentZone == nZones) {
        peopleCount++;
        log('People count incremented: $peopleCount people');
        apiPeopleCounter.appendDataRow([DateTime.now().toIso8601String(), peopleCount.toString()]);
      }
      lastPosition = currentZone;
    }

    if (min(heightData) < 0) {
      //return [];
    }
    return heightData;
  }

  LocalMaximum maxAndPosition(List<int> sublist) {
    int maxValue = 0;
    int position = 0;
    for (int i = 0; i < sublist.length; i++) {
      if (sublist[i] > maxValue) {
        maxValue = sublist[i];
        position = i;
      }
    }
    return LocalMaximum(maxValue, position);
  }

  int max(List<int> sublist) {
    return maxAndPosition(sublist).value;
  }

  int min(List<int> sublist) {
    int minValue = 0;
    for (int x in sublist) {
      if (x < minValue) {
        minValue = x;
      }
    }
    return minValue;
  }

  void setSurface(Measurement measurement) {
    background = measurement.data;
  }
}

class LocalMaximum {
  int value = 0;
  int position = 0;
  LocalMaximum(this.value, this.position);

  /// Returns the zone of the local maximum
  ///
  int getZone(int nZones) {
    return position * nZones ~/ 64;
  }
}
