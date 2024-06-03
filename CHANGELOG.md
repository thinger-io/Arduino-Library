## 2.30.0

#### OTA

- **Added** support for defining `THINGER_FIRMWARE_VERSION` to set the firmware version on the device. This build flag allows the VSCode extension (from version 1.1.0) to skip updates on devices with the same firmware version.
- **Improved** compressed/uncompressed checksum verification on OTA updates.

#### Arduino Portenta/Opta Devices

- **Added** OTA implementation for Arduino Portenta/Opta devices, including LZSS compression.
- **Initial support** for Arduino Portenta/Opta Ethernet connectivity.
- **Updated** Arduino Portenta examples for using thinger.io as an Mbed thread by using `thing.start()`.
- **Added** specific Arduino Opta examples, including relay and status LED control.

2.29.0

#### ESP32 Ethernet 
- Improve ESP32 Ethernet TLS support by using stock Arduino WiFiClientSecure
- Fix ESP32 log events related to Ethernet link status
- Update ESP32 Ethernet default example including OTA support
- Allow using THINGER_INSECURE_SSL definition to allow using self-signed certificates on ESP32 Ethernet

2.28.0

- Initial support for ESP32 Ethernet connection over TLS

2.27.0

- Fix compiling issues for AVR
- Avoid trying to send keep-alive/streams on the first connection timeout

2.26.0

- Added CMakeLists for integration with ESP-IDF as component
- Add ThingerClient methods to detect socket connection failures to improve network reconnection mechanisms
- Improve BC66
- Improve MKRNB1500

2.25.2

- Fix Compiling on WiFi devices without WiFi.begin() support after adding the WebConfig compatibility on ESP32.
- Fix PORTENTA H7 compiling issue: does not implement WiFi.begin(SSID).

2.25.1

- Fix ESP32 Webconfig issue
- Improve ESP8266 NTP sync

2.25.0

- Add support for ESP32 Webconfig
- Improve OTA by stopping user flows, handling timeouts, providing callbacks, etc.
