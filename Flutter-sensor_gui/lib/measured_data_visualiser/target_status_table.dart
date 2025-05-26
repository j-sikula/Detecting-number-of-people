import 'package:flutter/material.dart';

/// A widget that displays a table with target status codes and their descriptions.
class TargetStatusTable extends StatelessWidget {
  const TargetStatusTable({super.key});

  @override
  Widget build(BuildContext context) {
    return Center(
      child: SingleChildScrollView(
        scrollDirection: Axis.horizontal,
        child: DataTable(
          columns: const [
            DataColumn(label: Text('Target status')),
            DataColumn(label: Text('Description')),
          ],
          rows: const [
            DataRow(cells: [
              DataCell(Text('0',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Ranging data are not updated',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('1',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Signal rate too low on SPAD array',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('2',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Target phase',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('3',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Sigma estimator too high',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('4',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Target consistency failed',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('5',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Range valid',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('6',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text(
                  'Wrap around not performed (typically the first range)',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('7',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Rate consistency failed',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('8',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Signal rate too low for the current target',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('9',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text(
                  'Range valid with large pulse (may be due to a merged target)',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('10',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text(
                  'Range valid, but no target detected at previous range',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('11',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Measurement consistency failed',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('12',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Text('Target blurred by another one, due to sharpener',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
            ]),
            DataRow(cells: [
              DataCell(Text('13',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Target detected but inconsistent data.',
                      softWrap: true,
                      overflow: TextOverflow.visible,
                      maxLines: null),
                  Text('Frequently happens for secondary targets.',
                      softWrap: true,
                      overflow: TextOverflow.visible,
                      maxLines: null),
                ],
              )),
            ]),
            DataRow(cells: [
              DataCell(Text('255',
                  softWrap: true,
                  overflow: TextOverflow.visible,
                  maxLines: null)),
              DataCell(Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('No target detected',
                      softWrap: true,
                      overflow: TextOverflow.visible,
                      maxLines: null),
                  Text('(only if number of targets detected is enabled)',
                      softWrap: true,
                      overflow: TextOverflow.visible,
                      maxLines: null),
                ],
              )),
            ]),
          ],
        ),
      ),
    );
  }
}
