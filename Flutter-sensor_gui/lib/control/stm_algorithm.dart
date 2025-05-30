import 'dart:developer';

import 'package:sensor_gui/control/data_decoder.dart';
import 'package:sensor_gui/control/people_counter.dart';

class STMAlgorithmus {
  List<int> exitSequence = [0, 2, 3, 1, 0];
  List<int> enterSequence = [0, 1, 3, 2, 0];
  int positionExit = 0; //position in the exit sequence
  int positionEnter = 0; //position in the enter sequence

  int stateValue = 0; // 0 - no one, 2 - front, 1 - back, 3 - both

  bool rotated =
      true; // Sensor is rotated by 180 degrees, invereses logic of enters and exits
  bool transposed = true; // Sensor is rotated by 90 degrees

  int threshold = 1300;
  List<int> background = List.filled(64, 2100)
    ..[3] = 800
    ..[4] = 800
    ..[59] = 800
    ..[60] = 800;

  int peopleCount = 0; // Current people count
  List<PeopleCount> peopleCountHistory = [];

  List<int> processMeasurement(Measurement currentFrame) {
    List<int> heightData = List.generate(background.length,
        (index) => background[index] - currentFrame.depthData[index]);

    int enterZoneMax = 0;
    int exitZoneMax = 0;
    for (int i = 0; i < heightData.length; i++) {
      // Set the height to 0 if the status is 255 (no target detected == floor)
      if (currentFrame.statuses[i] == 255) {
        heightData[i] = 0;
      }

      if (transposed) {
        if (i % 8 > 1 && i % 8 < 4) {
          if (rotated) {
            if (heightData[i] > enterZoneMax) {
              enterZoneMax = heightData[i];
            }
          } else {
            if (heightData[i] > exitZoneMax) {
              exitZoneMax = heightData[i];
            }
          }
        } else if (i % 8 >3 && i % 8 < 6) {
          if (rotated) {
            if (heightData[i] > exitZoneMax) {
              exitZoneMax = heightData[i];
            }
          } else {
            if (heightData[i] > enterZoneMax) {
              enterZoneMax = heightData[i];
            }
          }
        }
      } else {
        if (i ~/ 8 > 1 && i ~/ 8 < 4) {
          if (rotated) {
            if (heightData[i] > enterZoneMax) {
              enterZoneMax = heightData[i];
            }
          } else {
            if (heightData[i] > exitZoneMax) {
              exitZoneMax = heightData[i];
            }
          }
        } else if (i ~/ 8 > 3 && i ~/ 8 < 6) {
          if (rotated) {
            if (heightData[i] > exitZoneMax) {
              exitZoneMax = heightData[i];
            }
          } else {
            if (heightData[i] > enterZoneMax) {
              enterZoneMax = heightData[i];
            }
          }
        }
      }
    }

    int currentStateValue = 0;
    if (enterZoneMax > threshold) {
      currentStateValue = 2;
    }
    if (exitZoneMax > threshold) {
      currentStateValue += 1;
    } else if (currentStateValue == 0 && stateValue == 0) {
      positionExit =
          0; // reset exit position if no one is detected and in previous state no one was detected
      positionEnter = 0; // reset enter position if no one is detected
    }

    //log( "Current state value: $currentStateValue, Enter zone max: $enterZoneMax, Exit zone max: $exitZoneMax");

    if (currentStateValue != stateValue) {
      // found following state in exit sequence,
      if (currentStateValue == exitSequence[positionExit + 1] &&
          positionEnter == 0) {
        positionExit++;
        if (positionExit >= exitSequence.length - 1) {
          // person exited
          peopleCount--;
          log("person exited - current count: $peopleCount");
          peopleCountHistory.add(PeopleCount(currentFrame.time, peopleCount));
          positionExit = 0;
        }
      } else if (positionExit != 0 &&
          currentStateValue == exitSequence[positionExit - 1] &&
          positionEnter == 0) {
        positionExit--;
      } else if (currentStateValue == enterSequence[positionEnter + 1]) {
        positionEnter++;
        if (positionEnter >= enterSequence.length - 1) {
          // person entered
          peopleCount++;
          log("person entered - current count: $peopleCount");
          peopleCountHistory.add(PeopleCount(currentFrame.time, peopleCount));
          positionEnter = 0;
        }
      } else if (positionEnter != 0 &&
          currentStateValue == enterSequence[positionEnter - 1]) {
        positionEnter--;
      } else {
        positionExit = 0; // reset state machine if the sequence is broken
        positionEnter = 0;
        log("State machine sequence not expected: $currentStateValue != ${exitSequence[positionExit + 1]} or ${enterSequence[positionExit + 1]}");
      }

      if (positionEnter >= enterSequence.length) {
        positionEnter = 0;
        log("position enter reset");
      }

      if (positionExit >= exitSequence.length) {
        positionExit = 0;
        log("position exit reset");
      }
    }
    stateValue = currentStateValue;
    return heightData;
  }

  void resetPeopleCounter() {
    peopleCount = 0;
    peopleCountHistory.clear();
    positionExit = 0;
    positionEnter = 0;
    stateValue = 0;
  }
}
