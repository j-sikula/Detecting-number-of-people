| Supported Target | ESP32-C6 | 
| ---------------- | -------- | 

# Firmware for ESP32-C6 

This software was developed from blink_example provided by Esspressif.

It is a part of [bachelor thesis]( https://www.vut.cz/en/students/final-thesis/detail/167854)

The `led_strip` library is installed via [component manager](main/idf_component.yml).

## Get started

Add file `main/google_api/keys.h`, which is not tracked by git due to security reasons, with following defines:
```
#define API_KEY "API_KEY"
#define CLIENT_EMAIL "service-account@domain.com"
#define PRIVATE_KEY "-----BEGIN PRIVATE KEY-----\nHERE IS PRIVATE KEY\n-----END PRIVATE KEY-----\n"
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASSWORD"
```

`server_api_cert.pem` can be updated using OpenSSL `openssl s_client -showcerts -connect sheets.googleapis.com:443`

`server_cert.pem` can be updated using `openssl s_client -showcerts -connect oauth2.googleapis.com:443`

## How to Use Example

Before project configuration and build, be sure to set the correct chip target using `idf.py set-target <chip_name>`.

### Hardware Required

* A development board with normal LED or addressable LED on-board (e.g., ESP32-S3-DevKitC, ESP32-C6-DevKitC etc.)
* A USB cable for Power supply and programming

See [Development Boards](https://www.espressif.com/en/products/devkits) for more information about it.

### Configure the Project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Select the LED type in the `Blink LED type` option.
  * Use `GPIO` for regular LED
  * Use `LED strip` for addressable LED
* If the LED type is `LED strip`, select the backend peripheral
  * `RMT` is only available for ESP targets with RMT peripheral supported
  * `SPI` is available for all ESP targets
* Set the GPIO number used for the signal in the `Blink GPIO number` option.
* Set the blinking period in the `Blink period in ms` option.

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.



## Troubleshooting

* If the LED isn't blinking, check the GPIO or the LED type selection in the `Example Configuration` menu.

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
