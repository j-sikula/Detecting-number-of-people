import 'dart:developer';

import 'package:flutter/foundation.dart';
import 'package:sensor_gui/control/data_decoder.dart';

import 'package:sensor_gui/control/google_sheets_api.dart';

class PeopleCounter {
  final GoogleSheetsApi apiPeopleCounter;
  int lastPosition = 0;
  int peopleCount = 0;
  final int heightThreshold = 1000;

  /// Number of pixels that must be higer than the threshold
  final int nPixelsToActivateZone = 5;
  static const int nZones = 4;

  List<bool> zoneEntered = List.filled(nZones, false);
  List<bool> zoneExited = List.filled(nZones, false);
  int? positionEntered;

  List<int> background = List.filled(64, 0);

  PeopleCounter(this.apiPeopleCounter);

  /// Notifier for updating PeopleCount in the UI
  final ValueNotifier<int> peopleCountNotifier = ValueNotifier<int>(0);

  List<int> processMeasurement(Measurement measurement) {
    // Checks size of the data
    if (background.length != 64 && measurement.depthData.length != 64) {
      return [];
    }
    List<int> heightData = List.generate(background.length,
        (index) => background[index] - measurement.depthData[index]);
    
    // Set the height to 0 if the status is 255 (no target detected == floor)
    for (int i = 0; i < heightData.length; i++) {
      if (measurement.statuses[i] == 255) {
        heightData[i] = 0;
      }
    }

    for (int i = 0; i < nZones; i++) {
      int nPixelsAboveThreshold = 0;
      for (int j = 0; j < 64 / nZones; j++) {
        if (heightData[i * 64 ~/ nZones + j] > heightThreshold) {
          nPixelsAboveThreshold++;
        }
      }
      // Check if the zone is entered or exited
      if (nPixelsAboveThreshold > nPixelsToActivateZone) {
        zoneEntered[i] = true;
        zoneExited[i] = false;
        if (positionEntered == null && (i == 0 || i == nZones - 1)) {
          positionEntered = i;
        }
      } else {
        // If the zone is exited, the zone is exited only if the zone was entered
        if (zoneEntered[i]) {
          zoneExited[i] = true;
        }

        // If all the zones are exited, the people count is updated
        if (zoneExited.every((element) => element)) {
          if (positionEntered == 0) {
            peopleCount--;
            peopleCountNotifier.value = peopleCount;
            log('People count decremented: $peopleCount people');
            apiPeopleCounter.appendDataRow(
                [DateTime.now().toIso8601String(), peopleCount.toString()]);
          }
          if (positionEntered == nZones - 1) {
            peopleCount++;
            peopleCountNotifier.value = peopleCount;
            log('People count incremented: $peopleCount people');
            apiPeopleCounter.appendDataRow(
                [DateTime.now().toIso8601String(), peopleCount.toString()]);
          }
          positionEntered = null;
          zoneEntered = List.filled(nZones, false);
          zoneExited = List.filled(nZones, false);
        } else {
          // when person exits in position entered
          if (zoneExited.equals(zoneEntered)) {
            positionEntered = null;
            zoneEntered = List.filled(nZones, false);
            zoneExited = List.filled(nZones, false);
          }
        }
      }
    }

    return heightData;
  }

  void setSurface(Measurement measurement) {
    background = measurement.depthData;
  }
}

extension on List<bool> {
  bool equals(List<bool> zoneEntered) {
    for (int i = 0; i < length; i++) {
      if (this[i] != zoneEntered[i]) {
        return false;
      }
    }
    return true;
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


class PeopleCount {
  int count;
  DateTime timestamp;
  PeopleCount(this.timestamp, this.count);

  double getTimestampInMiliseconds() {
    return timestamp.millisecondsSinceEpoch.toDouble();
  }
}
