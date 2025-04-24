import 'package:flutter/material.dart';

class TargetStatusTable extends StatelessWidget {
  const TargetStatusTable({super.key});

  @override
  Widget build(BuildContext context) {
    return Center(
        child: DataTable(
          columns: const [
            DataColumn(label: Text('Target status')),
            DataColumn(label: Text('Description')),
          ],
          rows: const [
            DataRow(cells: [
              DataCell(Text('0')),
              DataCell(Text('Ranging data are not updated')),
            ]),
            DataRow(cells: [
              DataCell(Text('1')),
              DataCell(Text('Signal rate too low on SPAD array')),
            ]),
            DataRow(cells: [
              DataCell(Text('2')),
              DataCell(Text('Target phase')),
            ]),
            DataRow(cells: [
              DataCell(Text('3')),
              DataCell(Text('Sigma estimator too high')),
            ]),
            DataRow(cells: [
              DataCell(Text('4')),
              DataCell(Text('Target consistency failed')),
            ]),
            DataRow(cells: [
              DataCell(Text('5')),
              DataCell(Text('Range valid')),
            ]),
            DataRow(cells: [
              DataCell(Text('6')),
              DataCell(Text(
                  'Wrap around not performed (typically the first range)')),
            ]),
            DataRow(cells: [
              DataCell(Text('7')),
              DataCell(Text('Rate consistency failed')),
            ]),
            DataRow(cells: [
              DataCell(Text('8')),
              DataCell(Text('Signal rate too low for the current target')),
            ]),
            DataRow(cells: [
              DataCell(Text('9')),
              DataCell(Text(
                  'Range valid with large pulse (may be due to a merged target)')),
            ]),
            DataRow(cells: [
              DataCell(Text('10')),
              DataCell(Text(
                  'Range valid, but no target detected at previous range')),
            ]),
            DataRow(cells: [
              DataCell(Text('11')),
              DataCell(Text('Measurement consistency failed')),
            ]),
            DataRow(cells: [
              DataCell(Text('12')),
              DataCell(Text('Target blurred by another one, due to sharpener')),
            ]),
            DataRow(cells: [
              DataCell(Text('13')),
              DataCell(Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Target detected but inconsistent data.'),
                  Text('Frequently happens for secondary targets.'),
                ],
              )),
            ]),
            DataRow(cells: [
              DataCell(Text('255')),
              DataCell(Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('No target detected'),
                  Text('(only if number of targets detected is enabled)'),
                ],
              )),
            ]),
          ],
        ),
      );
  }
}
