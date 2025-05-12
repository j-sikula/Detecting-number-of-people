import 'dart:io';

import 'package:flutter/material.dart';
import 'package:sensor_gui/control/data_decoder.dart';
import 'dart:math';
import 'dart:developer' as dev;

const MAX_MOVEMENT_LENGTH = 4.3; // Maximum length of the movement in pixxels
const MIN_LOCAL_MINIMUMS_DISTANCE = 4.0;
const depthThreshold =
    1390; // Threshold for depth data to consider a person present

class PeopleDetector {
  int peopleCount = 0; // Number of people in the room
  List<PeopleCount> peopleCountHistory = []; // History of people count
  Measurement lastMeasurement = Measurement(
    List<int>.filled(64, 0),
    DateTime.now(),
    List<int>.filled(64, 0),
  ); // Default measurement
  List<PersonMovement> peopleMovements = [];
  List<int> weightedData = List.filled(64, 0); // Weighted data
  List<int> cumsum = List.filled(64, 0); // Cumulative sum of depth data
  final List<int> detectionGrid = // List.filled(25, 1); // Detection grid 5x5
      /*    [0,0,1,0,0,
   0,2,3,2,0,
   1,3,5,3,1,
   0,2,3,2,0,
   0,0,1,0,0]; 

    [1,1,1,1,1,
   1,2,2,2,1,
   1,2,2,2,1,
   1,2,2,2,1,
   1,1,1,1,1];
*/
      [1, 1, 1, 1, 2, 1, 1, 1, 1];
  int detectionGridSize = 3; // Size of the detection grid

  AlgorithmData processFrame(Measurement measurement) {
    List<int> depthData = List.from(measurement.depthData);
    for (int i = 0; i < depthData.length; i++) {
      if (measurement.statuses[i] == 255) {
        // If target not detected, set depth data to background (2100 mm)
        depthData[i] = 2100;
      }

      // eliminate the door
      if ((i % 8 == 7 &&
              depthData[i] <= 147 &&
              depthData[i] >= 134 &&
              measurement.depthData[(i + 32) % 64] <= 147 &&
              measurement.depthData[(i + 32) % 64] >= 134) ||
          (i % 8 == 6 &&
              depthData[i] <= 215 &&
              depthData[i] >= 190 &&
              measurement.depthData[(i + 32) % 64] <= 215 &&
              measurement.depthData[(i + 32) % 64] >= 190) ||
          (i % 8 == 5 &&
              depthData[i] <= 340 &&
              depthData[i] >= 280 &&
              measurement.depthData[(i + 32) % 64] <= 340 &&
              measurement.depthData[(i + 32) % 64] >= 280)) {
        depthData[i] = 2100; // Set the depth data to background (2100 mm)
      }
    }
    for (int i = 0; i < depthData.length; i++) {
      int nWeightedCells = 0; // Number of weighted cells
      for (int j = 0; j < detectionGrid.length; j++) {
        int index = i +
            (8 * (j ~/ detectionGridSize) +
                j % detectionGridSize -
                (detectionGridSize - 1) ~/
                    2 *
                    9); // Calculate the index for the depth data grid
        // Check if the index is within bounds and in the same row (overflow is handled)
        if (index >= 0 &&
            index < 64 &&
            index ~/ 8 ==
                i ~/ 8 -
                    (detectionGridSize - 1) ~/ 2 +
                    j ~/ detectionGridSize) {
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

    // finding for optimizing performance
    for (int i = 1; i < weightedData.length; i++) {
      if (weightedData[i] < weightedData[minValue]) {
        minValue = i;
      }
    }
    if (weightedData[minValue] < depthThreshold) {
      localMinimums = findAllLocalMinimums(weightedData);
    }

    for (int i = 0; i < localMinimums.length; i++) {
      if (weightedData[localMinimums[i]] > depthThreshold) {
        localMinimums
            .removeAt(i); // Remove the local minimums that are below threshold
        i--;
      }
    }

    List<int> highlight =
        countPeople(localMinimums); // Count people in the grid
    for (int i in highlight) {
      toReturn.highlightData(i); // Highlight the local minimums
    }
    return toReturn;
  }

  List<int> countPeople(List<int> indexesOfLocalMinimums) {
    if (indexesOfLocalMinimums.isEmpty && peopleMovements.isNotEmpty && peopleMovements[0].deleteMovement) {
      // No people present, clear the list
      peopleMovements.clear();
      return [];
    }

    // Remove multiple people movements in the same position
    for (int i = 0; i < peopleMovements.length; i++) {
      for (int j = i + 1; j < peopleMovements.length; j++) {
        if (peopleMovements[i].currentPosition ==
                peopleMovements[j].currentPosition &&
            i != j &&
            peopleMovements[j].startedExiting ==
                peopleMovements[i].startedExiting) {
          peopleMovements.removeAt(j);
          j--;
        }
      }
    }

    List<int> indexesOfPresentPeople =
        []; // List to store indexes of present people

    for (int i = 0; i < peopleMovements.length; i++) {	
      int destinationIndex = findLocalMinimum(peopleMovements[i].currentPosition);
      if (getDistance(destinationIndex, peopleMovements[i].currentPosition) >
          MAX_MOVEMENT_LENGTH) {
        // If the distance is too big, remove the person from the list
        destinationIndex = peopleMovements[i].currentPosition;
        dev.log("Overflowing distance, banned", level: 2);
      }
      if (weightedData[destinationIndex] < depthThreshold) {
        if (isBorderIndex(destinationIndex)) {
          if (isExitBorderIndex(destinationIndex) ==
              isExitBorderIndex(peopleMovements[i].startPosition)) {
            // Person exits where entered
            peopleMovements[i].updatePosition(
                destinationIndex); // Update the current position
            peopleMovements[i].startPosition =
                destinationIndex; // Update the start position
          } else {
            // Check if there is not multiple detection of the same person
            if (peopleCountHistory.isEmpty ||
                peopleCountHistory.last.time != lastMeasurement.time) {
              if (peopleMovements[i].startedExiting) {
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
            }
            // Note: To modify list in Dart, syntax for(var element in list) cannot be used, use iteration for(int i = 0; i < list.length; i++)
            peopleMovements[i] = PersonMovement(
                destinationIndex); // After finishing the movement, create a new movement for case it returns back immediately
          }
        } else {
          peopleMovements[i]
              .updatePosition(destinationIndex); // Update the current position
        }

        indexesOfPresentPeople.add(destinationIndex);
      }
    }

    for (int i = 0; i < indexesOfLocalMinimums.length; i++) {
      // person on the edge of the grid
      if (isBorderIndex(indexesOfLocalMinimums[i])) {
        if (peopleMovements.isEmpty) {
          // If no people are present, add the first person
          peopleMovements.add(PersonMovement(indexesOfLocalMinimums[i]));
          indexesOfPresentPeople.add(
              indexesOfLocalMinimums[i]); // Add the index of the local minimum
        } else {
          if (indexesOfPresentPeople.contains(indexesOfLocalMinimums[i])) {
            // If the person is already present, skip
            continue;
          } else {
            // If the person is not present in PersonMovement, add the new person
            double minDistance = 8;
            for (int j = 0; j < peopleMovements.length; j++) {
              minDistance = getDistance(peopleMovements[j].currentPosition,
                  indexesOfLocalMinimums[i]);
            
              if (minDistance >= MIN_LOCAL_MINIMUMS_DISTANCE) {
                peopleMovements.add(PersonMovement(indexesOfLocalMinimums[i]));
                indexesOfPresentPeople.add(indexesOfLocalMinimums[
                    i]); // Add the index of the local minimum
              }
            }
          }
        }
      }
    }

    for (PersonMovement person in List.from(peopleMovements)) {
      if (person.updatedPosition) {
        // If the position is updated, reset the flag
        person.updatedPosition = false;
      } else {
        // If the position is not updated twice, remove the person from the list
        if (person.deleteMovement) {
          // If the measurement should be deleted, remove the person from the list
          peopleMovements.remove(person);
        } else {
          // If the measurement should not be deleted, set the flag to true
          person.deleteMovement = true;
        }
      }
    }

    return indexesOfPresentPeople;
  }

  /// Checks if the index is on the border of the grid, where the person can enter or exit.
  bool isBorderIndex(int index) {
    int x = index % 8; // Column index
    int y = index ~/ 8; // Row index
    /*// Check if the index is on the border of the grid
    return x != 3 &&
        x != 4 &&
        (x == 0 ||
            x == 7 ||
            y == 0 ||
            y == 7); // 0-7 are the indexes of the grid*/
    return index % 8 < 2 ||
        index % 8 > 5 ||
        ((x == 2 || x == 5) && (y == 0 || y == 7));
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

  /// Finds the index of the local minimum in the weightedData List concerned as a 8x8 grid
  /// Algorithm: gradient descent
  int findLocalMinimum(int startIndex) {
    bool localMinimumFound = false; // Flag to check if local minimum is found
    while (!localMinimumFound) {
      int indexX = startIndex % 8; // Column index
      int indexY = startIndex ~/ 8; // Row index
      localMinimumFound = true; // Set the flag to true
      int localMinimum = startIndex; // Local minimum value
      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          if (i == 0 && j == 0) continue; // Skip the center element
          int neighborX = indexX + j;
          int neighborY = indexY + i;
          if (neighborX >= 0 &&
              neighborX < 8 &&
              neighborY >= 0 &&
              neighborY < 8) {
            int neighborIndex = neighborY * 8 + neighborX;
            if (weightedData[neighborIndex] < weightedData[localMinimum]) {
              localMinimum =
                  neighborIndex; // Update the index of the probable local minimum
              localMinimumFound =
                  false; // Set the flag to false, found lower value
            }
          }
        }
      }
      if (!localMinimumFound) {
        startIndex =
            localMinimum; // Update the start index to the new local minimum
      }
    }
    return startIndex; // Return the index of the local minimum
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
      peopleCountHistory.clear(); // Clear the history after saving

      dev.log('Data saved to file successfully');
    } catch (e) {
      dev.log('Failed to save data to file: $e');
    }
  }

  /// Resets the people counter and clears the history
  void resetPeopleCounter() {
    peopleCount = 0; // Reset the people count
    peopleCountHistory.clear(); // Clear the history
    peopleMovements.clear(); // Clear the movements
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
  bool deleteMovement = false; // Flag to check if the measurement should be deleted in next frame
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
    deleteMovement = false; // Reset the delete movement flag
  }
}

class PeopleCount {
  /// Number of people in the grid
  int peopleCount = 0;

  /// Time of the change of the count
  DateTime time;
  PeopleCount(this.peopleCount, this.time);
}
