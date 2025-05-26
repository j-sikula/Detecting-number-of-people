import 'dart:developer';
import 'dart:io';

import 'package:sensor_gui/control/google_sheets_api.dart';

import 'people_counter.dart';

class PeopleCountHandler {
  GoogleSheetsApi? apiPeopleCounter;
  final String spreadsheetID = "1SMUomRFOupgDCK7eLoi8eb6Y_97LJ3NA8j68mztiyTw";
  List<PeopleCount>? peopleCountData;

  PeopleCountHandler() {
    apiPeopleCounter = GoogleSheetsApi(spreadsheetID);
    if (apiPeopleCounter == null) {
      log("Google Sheets API is not initialized");
      return;
    }
    apiPeopleCounter!.initGoogleAPI();
  }

  /// Fetches people count data since the specified date.
  /// Returns a list of [PeopleCount] objects.
  Future<List<PeopleCount>> getPeopleCountDataSince(DateTime dataSince) async {
    if (apiPeopleCounter == null) {
      log("Google Sheets API is not initialized");
      return [];
    }
    List<String> weeks = getWeeksSince(dataSince);
    if (weeks.isEmpty) {
      log("No weeks found since the given date");
      return [];
    }

    List<PeopleCount> peopleCountData =
        await apiPeopleCounter!.getPeopleCount(weeks[0]);
    for (int i = 1; i < weeks.length; i++) {
      List<PeopleCount> tempData =
          await apiPeopleCounter!.getPeopleCount(weeks[i]);
      if (tempData.isNotEmpty) {
        peopleCountData.addAll(tempData);
      }
    }
    return peopleCountData;
  }

  /// Fetches people count data between the specified dates.
  /// Returns a list of [PeopleCount] objects filtered by the date range.
  Future<List<PeopleCount>> getPeopleCountData(
      DateTime dateFrom, DateTime dateUntil) async {
    peopleCountData = await getPeopleCountDataSince(dateFrom);
    List<PeopleCount> filteredData = [];
    for (var data in peopleCountData!) {
      if (data.timestamp.isAfter(dateFrom) &&
          data.timestamp.isBefore(dateUntil)) {
        filteredData.add(data);
      }
    }
    if (filteredData.isEmpty) {
      return [];
    }

    if (filteredData.last.timestamp.isBefore(dateUntil)) {
      // If the last data point is before the end date, add a PeopleCount with current date and last count
      filteredData.add(PeopleCount(dateUntil, filteredData.last.count)); // Assuming count 0 for the end date
    }
    return filteredData;
  }
}

/// Returns a list of week strings in the format "YYYY-WW" (for example 2025-W21)
List<String> getWeeksSince(DateTime date) {
  List<String> weeks = [];
  DateTime currentDate = DateTime.now();
  while (date.isBefore(currentDate)) {
    int weekNumber = date.weekOfYear;
    weeks.add("${date.year}-W$weekNumber");
    date = date.add(const Duration(days: 7));
  }
  if (currentDate.weekOfYear.toString() != weeks.last.split('-W')[1]) { // Check if the current week is already added
    weeks.add("${currentDate.year}-W${currentDate.weekOfYear}");
  }
  return weeks;
}

 /// Saves the people count history to a file in CSV format at [path].
  void savePeopleCountHistory(String path, List<PeopleCount> peopleCountHistory) async {
    if (path.contains('.') == false) {
      // Checks if the path contains the file extension
      path = '$path.csv'; // Adds the file extension csv if not present
    }

    final file = File(path);
    try {
      final sink = file.openWrite();
      sink.write('Time;People Count\n');

      // Write the data to the file
      for (PeopleCount count in peopleCountHistory) {
        sink.write('${count.timestamp.toIso8601String()};${count.count}\n');
      }
      await sink.flush();
      await sink.close();
      peopleCountHistory.clear(); // Clear the history after saving

      log('Data saved to file successfully');
    } catch (e) {
      log('Failed to save data to file: $e');
    }
  }

/// https://nonimi-ink.medium.com/calculating-iso-week-numbers-in-dart-7e3891e668c
extension DateTimeExtension on DateTime {
  int get weekOfYear {
    final startOfYear = DateTime(year, 1, 1);
    final weekNumber =
        ((difference(startOfYear).inDays + startOfYear.weekday) / 7).ceil();
    return weekNumber;
  }
}
