import 'package:flutter/material.dart';
import 'package:sensor_gui/measured_data_visualiser/measured_data_visualiser_screen.dart';
import 'package:sensor_gui/resullts_visualiser/home_screen.dart';
import 'package:sensor_gui/usb_serial_data_visualiser/sensor_data_visualiser_screen.dart';

class NavigationMainScreen extends StatefulWidget {
  const NavigationMainScreen({super.key});

  @override
  State<NavigationMainScreen> createState() => _NavigationMainScreenState();
}

class _NavigationMainScreenState extends State<NavigationMainScreen> {
  int currentPageIndex = 0;

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (BuildContext context, BoxConstraints constraints) {
        return Scaffold(
          appBar: AppBar(
            title: const Text('People counter'),
          ),
          body: Row(
            children: [
              constraints.maxWidth >= constraints.maxHeight
                  ? NavigationRail(
                      selectedIndex: currentPageIndex,
                      onDestinationSelected: (int index) {
                        setState(() {
                          currentPageIndex = index;
                        });
                      },
                      labelType: NavigationRailLabelType.all,
                      destinations: const <NavigationRailDestination>[
                        NavigationRailDestination(
                          selectedIcon: Icon(Icons.home),
                          icon: Icon(Icons.home_outlined),
                          label: Text('Home'),
                        ),
                        NavigationRailDestination(
                          icon: Icon(Icons.usb_sharp),
                          label: Text('USB UART'),
                        ),
                        NavigationRailDestination(
                          icon: Icon(Icons.file_copy_outlined),
                          label: Text('Measured data'),
                        ),
                      ],
                    )
                  : const SizedBox(),
              SizedBox(
                width: constraints.maxWidth < constraints.maxHeight
                    ? constraints.maxWidth
                    : constraints.maxWidth - 120,
                child: <Widget>[
                  const HomeScreen(),
                  const USBSerialDataVisualiserScreen(),
                  const MeasuredDataVisualiserScreen(),
                ][currentPageIndex],
              ),
            ],
          ),
          bottomNavigationBar: constraints.maxWidth < constraints.maxHeight
              ? NavigationBar(
                  onDestinationSelected: (int index) {
                    setState(() {
                      currentPageIndex = index;
                    });
                  },
                  selectedIndex: currentPageIndex,
                  destinations: const <Widget>[
                    NavigationDestination(
                      selectedIcon: Icon(Icons.home),
                      icon: Icon(Icons.home_outlined),
                      label: 'Home',
                    ),
                    NavigationDestination(
                      icon: Icon(Icons.usb_sharp),
                      label: 'USB UART',
                    ),
                    NavigationDestination(
                      icon: Icon(Icons.file_copy_outlined),
                      label: 'Measured data',
                    ),
                  ],
                )
              : const SizedBox(),
        );
      },
    );
  }
}
