import 'package:flutter/material.dart';
import 'package:sensor_gui/control/data_decoder.dart';
import 'dart:math';

class PeopleDetector {
  Measurement lastMeasurement = Measurement(
    List<int>.filled(64, 0),
    DateTime.now(),
    List<int>.filled(64, 0),
  ); // Default measurement
  List<int> cumsum = List.filled(64, 0); // Cumulative sum of depth data
  List<int> detectionGrid = [
    0,
    0,
    1,
    0,
    0,
    0,
    2,
    2,
    2,
    0,
    1,
    2,
    5,
    2,
    1,
    0,
    2,
    2,
    2,
    0,
    0,
    0,
    1,
    0,
    0
  ]; // Detection grid 5x5

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
      int index = localMinimums[i];
      if (weightedData[index] < 1000) {
        toReturn.highlightData(index); // Highlight the local minimums
      }
      
    }
    

    int i = 5;
    for (int j = 0; j < detectionGrid.length; j++) {
      int index = i +
          (8 * (j ~/ 5) +
              j % 5 -
              18); // Calculate the index for the depth data grid
      if (index >= 0 && index < 64 && index ~/ 8 == i ~/ 8 - 2 + j ~/ 5) {
        // Check if the index is within bounds and in the same row (overflow is handled)
        //toReturn.highlightData(index); // Highlight the data
      }
    }
    return toReturn;
  }


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
