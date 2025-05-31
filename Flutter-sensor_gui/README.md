| Supported platforms | Windows | Android |
|---------------------|---------|---------|

# Sensor GUI

The application consists of three screens – the first screen is used for visualisation
of results saved in cloud storage (Google Sheets), the second is used for visualisation
of data received in real time using USB UART and Serial port and performing an
algorithm. The third screen is used for visualisation of data from file and performing
a selected algorithm. Devices with wide screen are using Navigation rail for switching
three screens (items are switched on the left border of the screen). Navigation bar
on the bottom is used for screens with bigger height than width to keep space for
screens.

## Getting Started

### Create Google Sheets credentials

 - Sign into [Google cloud console](https://console.cloud.google.com) and create a new project.
 - Then search and enable in navigation menu in products `APIs & Services` -> `Library Google Sheets API`.
- In navigation menu -> `APIs & Services` -> `Credentials` create a service account  `+ CREATE CREDENTIALS` -> `Service account`, fill Service account ID
 and `CREATE AND CONTINUE`Grant a role of Editor
- In navigation menu -> `APIs & Services` -> `Credentials` click on created service account, in tab `KEYS JSON` create key by clicking `ADD KEY` -> `Create new key` -> selecting JSON -> `CREATE` and that download file with credentials `credentials.json`. In tab DETAILS is email address of service account.

add to `pubspec.yaml`

```
flutter :
    assets :
        − assets / credentials /
```


Run `flutter run`
