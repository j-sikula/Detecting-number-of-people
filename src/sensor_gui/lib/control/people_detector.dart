import 'dart:io';

import 'package:flutter/material.dart';
import 'package:sensor_gui/control/data_decoder.dart';
import 'dart:math';
import 'dart:developer' as dev;

const MAX_MOVEMENT_LENGTH = 6.0; // Maximum length of the movement in pixxels
const MIN_LOCAL_MINIMUMS_DISTANCE = 4.0;

class PeopleDetector {
  int peopleCount = 0; // Number of people in the room
  List<PeopleCount> peopleCountHistory = []; // History of people count
  Measurement lastMeasurement = Measurement(
    List<int>.filled(64, 0),
    DateTime.now(),
    List<int>.filled(64, 0),
  ); // Default measurement
  List<PersonMovement> peopleMovements = [];
  List<int> cumsum = List.filled(64, 0); // Cumulative sum of depth data
  List<int> detectionGrid = // Detection grid 5x5
  /*[0,0,1,0,0,
   0,2,3,2,0,
   1,3,5,3,1,
   0,2,3,2,0,
   0,0,1,0,0]; */

    [0,1,1,1,0,
   1,2,2,2,1,
   1,2,3,2,1,
   1,2,2,2,1,
   0,1,1,1,0];

  AlgorithmData processFrame(Measurement measurement) {
    List<int> depthData = List.from(measurement.depthData);
    List<int> weightedData = List.filled(64, 0);
    for (int i = 0; i < depthData.length; i++) {
      if (measurement.statuses[i] == 255) {
        // If target not detected, set depth data to background (2100 mm)
        depthData[i] = 2100;
      }

      int nWeightedCells = 0; // Number of weighted cells
      for (int j = 0; j < detectionGrid.length; j++) {
        int index = i +
            (8 * (j ~/ 5) +
                j % 5 -
                18); // Calculate the index for the depth data grid
        // Check if the index is within bounds and in the same row (overflow is handled)
        if (index >= 0 && index < 64 && index ~/ 8 == i ~/ 8 - 2 + j ~/ 5) {
          weightedData[i] += depthData[index] * detectionGrid[j];
          nWeightedCells++;
        }
      }
      weightedData[i] = (weightedData[i] / nWeightedCells).round().toInt();
    }

    lastMeasurement = Measurement(
      depthData,
      measurement.time,
      measurement.statuses,
    );
    AlgorithmData toReturn = AlgorithmData(
      dataGrid: weightedData,
      textToDisplay: "Data",
    );

    List<int> localMinimums = []; // List to store local minimums
    // find index of the minimum of weightedData
    int minValue = 0;

    for (int i = 1; i < weightedData.length; i++) {
      if (weightedData[i] < weightedData[minValue]) {
        minValue = i;
      }
    }
    if (weightedData[minValue] < 1000) {
      localMinimums = findAllLocalMinimums(weightedData);
    }

    for (int i = 0; i < localMinimums.length; i++) {
      if (weightedData[localMinimums[i]] > 1000) {
        localMinimums
            .removeAt(i); // Remove the local minimums that are below threshold
        i--;
      }
    }

    List<int> locationOfPresentPeople =
        []; // List to store indexes of present people
    List<bool> isPresent = List.filled(
        localMinimums.length, true); // List to store presence status
    for (int i = 0; i < localMinimums.length; i++) {
      // Reduce local minimums too close to each other
      if (!isPresent[i]) continue; // Skip if already marked as not present
      for (int j = i + 1; j < localMinimums.length; j++) {
        if (!isPresent[j]) continue; // Skip if already marked as not present
        if (getDistance(localMinimums[i], localMinimums[j]) <
            MIN_LOCAL_MINIMUMS_DISTANCE) {
          if (weightedData[localMinimums[j]] < weightedData[localMinimums[i]]) {
            isPresent[i] = false;
          } else {
            isPresent[j] = false;
          }
        }
      }
    }
    for (int i = 0; i < localMinimums.length; i++) {
      if (isPresent[i]) {
        locationOfPresentPeople
            .add(localMinimums[i]); // Add the index of the local minimum
        toReturn
            .highlightData(localMinimums[i]); // Highlight the local minimums
      }
    }

    countPeople(locationOfPresentPeople); // Count people in the grid
    return toReturn;
  }

  void countPeople(List<int> indexesOfPresentPeople) {
    if (indexesOfPresentPeople.isEmpty) {
      // No people present, clear the list
      peopleMovements.clear();
      return;
    }
    for (int i = 0; i < indexesOfPresentPeople.length; i++) {
      // person on the edge of the grid
      if (isBorderIndex(indexesOfPresentPeople[i])) {
        if (peopleMovements.isEmpty) {
          // If no people are present, add the first person
          peopleMovements.add(PersonMovement(indexesOfPresentPeople[i]));
        } else {
          int indexOfMinDistance =
              -1; // Index of the minimum distance, -1 if not found
          double minDistance = MAX_MOVEMENT_LENGTH; // Minimum distance

          // Find the closest las person position to the edge of the grid
          for (int j = 0; j < peopleMovements.length; j++) {
            double distance = getDistance(
                peopleMovements[j].currentPosition,
                indexesOfPresentPeople[
                    i]); // Distance between the last person position and the grid edge
            if (distance < minDistance) {
              minDistance = distance;
              indexOfMinDistance = j;
            }
          }
          // Person is close to the edge of the grid
          if (minDistance < MAX_MOVEMENT_LENGTH) {
            if (isBorderIndex(peopleMovements[indexOfMinDistance].currentPosition) && (isExitBorderIndex(indexesOfPresentPeople[i]) == isExitBorderIndex(peopleMovements[indexOfMinDistance].currentPosition))) {
              // Person stays in the border
              peopleMovements[indexOfMinDistance]
                  .updatePosition(indexesOfPresentPeople[i]);
            } else {
              // Person moved through the entire grid
              if (isExitBorderIndex(
                      peopleMovements[indexOfMinDistance].startPosition) !=
                  isExitBorderIndex(indexesOfPresentPeople[i] % 8)) {
                if (peopleMovements[indexOfMinDistance].startedExiting) {
                  dev.log("Person exited ${lastMeasurement.time}");
                  peopleCount--;
                  peopleCountHistory
                      .add(PeopleCount(peopleCount, lastMeasurement.time));
                } else {
                  dev.log("Person entered ${lastMeasurement.time}");
                  peopleCount++;
                  peopleCountHistory
                      .add(PeopleCount(peopleCount, lastMeasurement.time));
                }
                peopleMovements[indexOfMinDistance] = PersonMovement(
                    indexesOfPresentPeople[
                        i]); // After finishing the movement, create a new movement for case it returns back immediately
              }
            }
          } else {
            // If presence on the boreder is not recognized as movement from previous actions, create new entry
            peopleMovements.add(PersonMovement(indexesOfPresentPeople[i]));
          }
        }
      } else {
        if (peopleMovements.isEmpty) {
          dev.log("Appeared without entering the grid from the edge");
          break;
        } else {
          int indexOfMinDistance =
              -1; // Index of the minimum distance, -1 if not found
          double minDistance = MAX_MOVEMENT_LENGTH; // Minimum distance

          // Find the closest las person position to the edge of the grid
          for (int j = 0; j < peopleMovements.length; j++) {
            double distance = getDistance(
                peopleMovements[j].currentPosition,
                indexesOfPresentPeople[
                    i]); // Distance between the last person position and the grid edge
            if (distance < minDistance) {
              minDistance = distance;
              indexOfMinDistance = j;
            }
          }
          if (minDistance < MAX_MOVEMENT_LENGTH) {
            peopleMovements[indexOfMinDistance].updatePosition(
                indexesOfPresentPeople[i]); // Update the current position
          }
        }
      }
    }
    for (PersonMovement person in List.from(peopleMovements)) {
      if (person.updatedPosition) {
        // If the position is updated, reset the flag
        person.updatedPosition = false;
      } else {
        // If the position is not updated, remove the person from the list
        peopleMovements.remove(person);
      }
    }

    return;
  }

  /// Checks if the index is on the border of the grid, where the person can enter or exit.
  bool isBorderIndex(int index) {
    int x = index % 8; // Column index
    int y = index ~/ 8; // Row index
    // Check if the index is on the border of the grid
    return x != 3 &&
        x != 4 &&
        (x == 0 ||
            x == 7 ||
            y == 0 ||
            y == 7); // 0-7 are the indexes of the grid
  }

  bool isExitBorderIndex(int index) {
    return index % 8 <=
        3; // Check if the index is on the left border of the grid
  }

  /// Calculates the distance between two indexes in the grid.
  double getDistance(int index1, int index2) {
    int x1 = index1 % 8;
    int y1 = index1 ~/ 8;
    int x2 = index2 % 8;
    int y2 = index2 ~/ 8;
    return sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2)); // Euclidean distance
  }

  /// Finds local minimums in the data grid 8x8.
  /// Algorithm: brute force
  List<int> findAllLocalMinimums(List<int> data) {
    List<int> localMinimums = [];

    if (data.length == 64) {
      for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          bool isLocalMinimum = true;
          for (int k = -1; k <= 1; k++) {
            for (int l = -1; l <= 1; l++) {
              if (k == 0 && l == 0) continue; // Skip the center element
              int neighborX = j + l;
              int neighborY = i + k;
              if (neighborX >= 0 &&
                  neighborX < 8 &&
                  neighborY >= 0 &&
                  neighborY < 8) {
                int neighborIndex = neighborY * 8 + neighborX;
                if (data[neighborIndex] < data[i * 8 + j]) {
                  isLocalMinimum = false;
                  break;
                }
              }
            }
            if (!isLocalMinimum) break;
          }
          if (isLocalMinimum) {
            localMinimums.add(i * 8 + j); // Add the index of the local minimum
          }
        }
      }
    }

    return localMinimums;
  }

  List<int> getSquareMatrix(int startIndex, int size) {
    if (startIndex % 8 + size > 8) {
      return []; // Invalid square matrix size
    }
    List<int> squareMatrix = List.filled(size * size, 0);
    for (int i = 0; i < size; i++) {
      for (int j = 0; j < size; j++) {
        int index = startIndex + i * 8 + j;
        if (index >= 0 && index < 64) {
          squareMatrix[i * size + j] = index;
        }
      }
    }
    return squareMatrix;
  }

  void savePeopleCountHistory(String path) async {
    if (path.contains('.') == false) {
      // Checks if the path contains the file extension
      path = '$path.csv'; // Adds the file extension csv if not present
    }

    final file = File(path);
    try {
      final sink = file.openWrite();
      sink.write('Time;People Count\n');

      // Write the data to the file
      for (var count in peopleCountHistory) {
        sink.write('${count.time.toIso8601String()};${count.peopleCount}\n');
      }
      await sink.flush();
      await sink.close();

      dev.log('Data saved to file successfully');
    } catch (e) {
      dev.log('Failed to save data to file: $e');
    }
  }
}

class AlgorithmData {
  final List<int> dataGrid;
  List<Color?> highlightedData =
      List<Color?>.filled(64, null, growable: false); // Highlighted data
  String textToDisplay;

  AlgorithmData({required this.dataGrid, required this.textToDisplay});

  void highlightData(int index) {
    highlightedData[index] = const Color.fromARGB(255, 1, 255, 9);
  }
}

class PersonMovement {
  int startPosition; // index of the start position
  int currentPosition; // index of the current position
  bool startedExiting;
  bool updatedPosition = true; // Flag to check if the position is updated
  PersonMovement(this.startPosition)
      : currentPosition = startPosition,
        startedExiting =
            startPosition % 8 <= 3; // Person starts exiting the grid

  int getColumn() {
    return currentPosition % 8; // Column index
  }

  void updatePosition(int newPosition) {
    currentPosition = newPosition; // Update the current position
    updatedPosition = true; // Set the flag to true
  }
}

class PeopleCount {
  /// Number of people in the grid
  int peopleCount = 0;

  /// Time of the change of the count
  DateTime time;
  PeopleCount(this.peopleCount, this.time);
}
